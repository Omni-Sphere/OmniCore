#include "License/Services/LicenseService.hpp"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <boost/json.hpp>
#include <iostream>
#include <sstream>
#include <ctime>
#include <cstring>
#include <stdexcept>
#include <cstdlib>

namespace omnisphere::services
{
    static std::shared_ptr<LicenseService> s_sharedLicenseService;
    static std::mutex s_sharedMutex;

    std::shared_ptr<LicenseService> LicenseService::GetSharedInstance()
    {
        std::lock_guard<std::mutex> lock(s_sharedMutex);
        return s_sharedLicenseService;
    }

    void LicenseService::SetSharedInstance(std::shared_ptr<LicenseService> instance)
    {
        std::lock_guard<std::mutex> lock(s_sharedMutex);
        s_sharedLicenseService = std::move(instance);
    }

    // =========================================================================
    // Constructor
    // =========================================================================

    LicenseService::LicenseService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repository(std::make_shared<omnisphere::repositories::LicenseRepository>(std::move(dbPool)))
    {
        std::lock_guard<std::mutex> lock(s_sharedMutex);
        if (!s_sharedLicenseService)
        {
            s_sharedLicenseService = std::shared_ptr<LicenseService>(this, [](LicenseService*){});
        }
    }

    LicenseService::LicenseService(std::shared_ptr<omnisphere::repositories::LicenseRepository> repository)
        : m_repository(std::move(repository))
    {
        std::lock_guard<std::mutex> lock(s_sharedMutex);
        if (!s_sharedLicenseService)
        {
            s_sharedLicenseService = std::shared_ptr<LicenseService>(this, [](LicenseService*){});
        }
    }

    // =========================================================================
    // Criptografía — Helpers internos
    // =========================================================================

    std::string LicenseService::Base64UrlEncode(const std::vector<unsigned char>& data)
    {
        // Codificar en Base64 estándar primero
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO* mem = BIO_new(BIO_s_mem());
        b64 = BIO_push(b64, mem);
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(b64, data.data(), static_cast<int>(data.size()));
        BIO_flush(b64);

        BUF_MEM* bptr = nullptr;
        BIO_get_mem_ptr(b64, &bptr);
        std::string result(bptr->data, bptr->length);
        BIO_free_all(b64);

        // Convertir a Base64URL: + → -, / → _, quitar =
        for (auto& c : result) {
            if (c == '+') c = '-';
            else if (c == '/') c = '_';
        }
        while (!result.empty() && result.back() == '=')
            result.pop_back();

        return result;
    }

    std::string LicenseService::Base64UrlEncode(const std::string& data)
    {
        return Base64UrlEncode(std::vector<unsigned char>(data.begin(), data.end()));
    }

    std::string LicenseService::Base64UrlDecode(const std::string& encoded)
    {
        // Restaurar Base64 estándar desde Base64URL
        std::string b64 = encoded;
        for (auto& c : b64) {
            if (c == '-') c = '+';
            else if (c == '_') c = '/';
        }
        // Agregar padding
        while (b64.size() % 4 != 0) b64 += '=';

        BIO* b64bio = BIO_new(BIO_f_base64());
        BIO* mem    = BIO_new_mem_buf(b64.data(), static_cast<int>(b64.size()));
        b64bio = BIO_push(b64bio, mem);
        BIO_set_flags(b64bio, BIO_FLAGS_BASE64_NO_NL);

        std::string decoded(b64.size(), '\0');
        int len = BIO_read(b64bio, decoded.data(), static_cast<int>(decoded.size()));
        BIO_free_all(b64bio);

        if (len <= 0) return "";
        decoded.resize(static_cast<size_t>(len));
        return decoded;
    }

    std::string LicenseService::ComputeHmac(const std::string& data, const std::string& secret)
    {
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digestLen = 0;

        HMAC(EVP_sha256(),
             secret.c_str(),
             static_cast<int>(secret.size()),
             reinterpret_cast<const unsigned char*>(data.c_str()),
             data.size(),
             digest,
             &digestLen);

        return Base64UrlEncode(std::vector<unsigned char>(digest, digest + digestLen));
    }

    // =========================================================================
    // Generación de API Key
    // =========================================================================

    std::string LicenseService::GenerateKey(const LicenseParams& params, const std::string& masterSecret)
    {
        if (masterSecret.empty())
            throw LicenseException("OMNI_LICENSE_SECRET no configurada. No se puede generar la API Key.");

        if (params.expiresAt.empty())
            throw LicenseException("Se requiere fecha de expiración (expiresAt) para generar la API Key.");

        if (params.clientName.empty())
            throw LicenseException("Se requiere nombre del cliente (clientName) para generar la API Key.");

        // Fecha de emisión: hoy si no se especifica
        std::string issuedAt = params.issuedAt;
        if (issuedAt.empty())
        {
            std::time_t t = std::time(nullptr);
            char buf[16];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
            issuedAt = buf;
        }

        // Construir JSON de módulos
        boost::json::array modulesArr;
        for (const auto& mod : params.modules)
            modulesArr.push_back(boost::json::value(boost::json::string_view(mod)));

        boost::json::object payload;
        payload["client"]  = params.clientName;
        payload["issuer"]  = params.issuer;
        payload["iat"]     = issuedAt;
        payload["exp"]     = params.expiresAt;
        payload["modules"] = modulesArr;

        std::string payloadJson = boost::json::serialize(payload);
        std::string encodedPayload = Base64UrlEncode(payloadJson);
        std::string signature      = ComputeHmac(encodedPayload, masterSecret);

        return "OMNI-" + encodedPayload + "." + signature;
    }

    std::string LicenseService::GenerateKey(const LicenseParams& params) const
    {
        return GenerateKey(params, GetMasterSecret());
    }

    // =========================================================================
    // Parsear payload JSON
    // =========================================================================

    omnisphere::models::SystemLicense LicenseService::ParsePayload(const std::string& payloadJson)
    {
        omnisphere::models::SystemLicense lic;
        try
        {
            auto val = boost::json::parse(payloadJson);
            auto& obj = val.as_object();

            if (obj.contains("client"))  lic.clientName = obj.at("client").as_string().c_str();
            if (obj.contains("issuer"))  lic.issuer     = obj.at("issuer").as_string().c_str();
            if (obj.contains("iat"))     lic.issuedAt   = obj.at("iat").as_string().c_str();
            if (obj.contains("exp"))     lic.expiresAt  = obj.at("exp").as_string().c_str();

            if (obj.contains("modules") && obj.at("modules").is_array())
            {
                for (const auto& m : obj.at("modules").as_array())
                {
                    if (m.is_string())
                        lic.modules.insert(std::string(m.as_string().c_str()));
                }
            }
        }
        catch (const std::exception& ex)
        {
            throw LicenseException(std::string("Error al parsear payload de licencia: ") + ex.what());
        }
        return lic;
    }

    // =========================================================================
    // Validación de fecha
    // =========================================================================

    bool LicenseService::IsDateValid(const std::string& expiresAt)
    {
        if (expiresAt.size() < 10) return false;
        std::tm exp{};
        if (std::sscanf(expiresAt.c_str(), "%d-%d-%d", &exp.tm_year, &exp.tm_mon, &exp.tm_mday) != 3)
            return false;
        exp.tm_year -= 1900;
        exp.tm_mon  -= 1;
        exp.tm_hour = 23; exp.tm_min = 59; exp.tm_sec = 59;

        std::time_t expTime = std::mktime(&exp);
        std::time_t now     = std::time(nullptr);
        return expTime > now;
    }

    int LicenseService::DaysRemaining(const std::string& expiresAt)
    {
        if (expiresAt.size() < 10) return 0;
        std::tm exp{};
        if (std::sscanf(expiresAt.c_str(), "%d-%d-%d", &exp.tm_year, &exp.tm_mon, &exp.tm_mday) != 3)
            return 0;
        exp.tm_year -= 1900;
        exp.tm_mon  -= 1;
        exp.tm_hour = 23; exp.tm_min = 59; exp.tm_sec = 59;

        std::time_t expTime = std::mktime(&exp);
        std::time_t now     = std::time(nullptr);
        double diff = std::difftime(expTime, now);
        return diff > 0 ? static_cast<int>(diff / 86400) : 0;
    }

    std::string LicenseService::GetMasterSecret() const
    {
        const char* envSecret = std::getenv("OMNI_LICENSE_SECRET");
        if (envSecret && *envSecret) return std::string(envSecret);

        if (m_repository) return m_repository->GetMasterSecret();
        return "_.:0mn15ph3r3L1c3n53:._";
    }

    // =========================================================================
    // Activación — ValidateAndLoad
    // =========================================================================

    omnisphere::models::SystemLicense LicenseService::ActivateKey(const std::string& apiKey)
    {
        // Verificar formato: debe comenzar con "OMNI-"
        if (apiKey.rfind("OMNI-", 0) != 0)
            throw LicenseException("Formato de API Key inválido. Se espera el prefijo 'OMNI-'.");

        std::string body = apiKey.substr(5); // Quitar "OMNI-"
        auto dotPos = body.rfind('.');
        if (dotPos == std::string::npos)
            throw LicenseException("Formato de API Key inválido. Falta separador de firma '.'.");

        std::string encodedPayload = body.substr(0, dotPos);
        std::string providedSig    = body.substr(dotPos + 1);

        // Verificar firma HMAC-SHA256
        std::string secret = GetMasterSecret();
        std::string expectedSig = ComputeHmac(encodedPayload, secret);

        if (expectedSig != providedSig)
            throw LicenseException("Firma de API Key inválida. La clave no fue emitida por OmniSphere Authority o ha sido modificada.");

        // Decodificar payload
        std::string payloadJson = Base64UrlDecode(encodedPayload);
        omnisphere::models::SystemLicense lic = ParsePayload(payloadJson);
        lic.apiKey = apiKey;

        // Verificar expiración
        if (!IsDateValid(lic.expiresAt))
            throw LicenseException("La API Key de licencia ha expirado el " + lic.expiresAt + ". Contacta a OmniSphere Authority para renovar.");

        // Generar código único (LIC1, LIC2, etc.)
        // Para simplificar, usamos timestamp si el repositorio no tiene identity
        if (lic.code.empty())
        {
            auto now = std::time(nullptr);
            lic.code = "LIC" + std::to_string(now % 100000);
        }

        lic.isActive      = true;
        lic.daysRemaining = DaysRemaining(lic.expiresAt);

        // Actualizar caché en RAM (thread-safe)
        {
            std::unique_lock lock(m_mutex);
            m_license = lic;
            m_loaded  = true;
        }

        // Persistir en BD (fire-and-forget; la RAM ya está actualizada)
        if (m_repository) m_repository->Save(lic);

        std::cout << "[LicenseService] Licencia activada. Cliente: " << lic.clientName
                  << " | Módulos: " << lic.modules.size()
                  << " | Vigente hasta: " << lic.expiresAt << std::endl;

        return lic;
    }

    // =========================================================================
    // Carga inicial desde BD (al arrancar el servidor)
    // =========================================================================

    bool LicenseService::LoadFromDatabase()
    {
        if (!m_repository) return false;
        try
        {
            auto active = m_repository->GetActive();
            if (!active.has_value())
            {
                std::cout << "[LicenseService] No hay licencia activa en BD. Sistema en modo sin licencia." << std::endl;
                return false;
            }

            const auto& lic = active.value();

            // Re-validar la key guardada (puede haber expirado desde el último arranque)
            if (!IsDateValid(lic.expiresAt))
            {
                std::cout << "[LicenseService] Licencia en BD expirada el " << lic.expiresAt << ". Desactivando." << std::endl;
                m_repository->Deactivate(lic.code);
                return false;
            }

            // Re-validar firma para detectar manipulación en BD
            try {
                std::string secret = GetMasterSecret();
                std::string body = lic.apiKey.substr(5);
                auto dotPos = body.rfind('.');
                if (dotPos != std::string::npos)
                {
                    std::string encodedPayload = body.substr(0, dotPos);
                    std::string providedSig    = body.substr(dotPos + 1);
                    std::string expectedSig    = ComputeHmac(encodedPayload, secret);
                    if (expectedSig != providedSig)
                    {
                        std::cerr << "[LicenseService] ADVERTENCIA: Firma de licencia en BD inválida. Posible manipulación. Desactivando." << std::endl;
                        m_repository->Deactivate(lic.code);
                        return false;
                    }
                }
            }
            catch (...) {
                // Si OMNI_LICENSE_SECRET no está configurada en este arranque, no podemos validar
                std::cerr << "[LicenseService] No se pudo re-validar la firma (OMNI_LICENSE_SECRET no disponible). Cargando de todas formas." << std::endl;
            }

            omnisphere::models::SystemLicense loaded = lic;
            loaded.isActive      = true;
            loaded.daysRemaining = DaysRemaining(lic.expiresAt);

            {
                std::unique_lock lock(m_mutex);
                m_license = loaded;
                m_loaded  = true;
            }

            std::cout << "[LicenseService] Licencia cargada desde BD. Cliente: " << loaded.clientName
                      << " | Módulos activos: " << loaded.modules.size()
                      << " | Días restantes: " << loaded.daysRemaining << std::endl;
            return true;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[LicenseService::LoadFromDatabase] " << ex.what() << std::endl;
            return false;
        }
    }

    // =========================================================================
    // Revocar licencia activa
    // =========================================================================

    bool LicenseService::RevokeActive()
    {
        std::unique_lock lock(m_mutex);
        std::string code = m_license.code;
        m_license = omnisphere::models::SystemLicense{};
        m_loaded  = false;

        if (m_repository && !code.empty())
            return m_repository->Deactivate(code);
        return true;
    }

    // =========================================================================
    // Consulta de módulos (O(1) — RAM)
    // =========================================================================

    bool LicenseService::IsModuleLicensed(const std::string& moduleCode) const
    {
        std::shared_lock lock(m_mutex);
        if (!m_loaded || !m_license.isActive) return false;
        return m_license.modules.count(moduleCode) > 0;
    }

    void LicenseService::RequireModule(const std::string& moduleCode) const
    {
        if (!IsModuleLicensed(moduleCode))
        {
            throw LicenseException(
                "Acceso denegado: El módulo '" + moduleCode +
                "' no está incluido en la licencia activa de OmniSphere. "
                "Contacta a tu administrador para actualizar la licencia."
            );
        }
    }

    std::optional<omnisphere::models::SystemLicense> LicenseService::GetActiveLicense() const
    {
        std::shared_lock lock(m_mutex);
        if (!m_loaded || !m_license.isActive) return std::nullopt;
        return m_license;
    }

    bool LicenseService::IsLicenseActive() const
    {
        std::shared_lock lock(m_mutex);
        return m_loaded && m_license.isActive;
    }

} // namespace omnisphere::services
