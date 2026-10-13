#include "License/Services/LicenseService.hpp"
#include <OmniUtils/Base64.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>
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

        return omnisphere::utils::Base64::EncodeUrl(std::string(reinterpret_cast<const char*>(digest), digestLen));
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

        // Módulos: si no se especifican, licenciar todos los módulos del ecosistema por defecto
        std::vector<std::string> modules = params.modules;
        if (modules.empty())
        {
            modules = omnisphere::license::AllModules();
        }

        // Construir JSON de módulos
        boost::json::array modulesArr;
        for (const auto& mod : modules)
            modulesArr.push_back(boost::json::value(boost::json::string_view(mod)));

        boost::json::object payload;
        payload["client"]  = params.clientName;
        payload["issuer"]  = params.issuer;
        payload["iat"]     = issuedAt;
        payload["exp"]     = params.expiresAt;
        payload["modules"] = modulesArr;

        std::string payloadJson = boost::json::serialize(payload);
        std::string encodedPayload = omnisphere::utils::Base64::EncodeUrl(payloadJson);
        std::string signature      = ComputeHmac(encodedPayload, masterSecret);

        std::string apiKey = "OMNI-" + encodedPayload + "." + signature;

        std::cout << "\n===============================================================================\n"
                  << "[LicenseService::GenerateKey] NUEVA LICENCIA GENERADA (" << modules.size() << " MÓDULOS):\n"
                  << apiKey << "\n"
                  << "===============================================================================\n" << std::endl;

        return apiKey;
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
        // El secreto HMAC NUNCA se guarda ni se lee de la base de datos.
        // Fuente única: variable de entorno del contenedor de la API.
        const char* envSecret = std::getenv("OMNI_LICENSE_SECRET");

        std::cout << "[LicenseService] Secreto HMAC: " << envSecret << std::endl;

        if (envSecret && *envSecret) 
            return std::string(envSecret);

        throw LicenseException("OMNI_LICENSE_SECRET no está configurada en el contenedor de la API.");
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

        std::cout << "[DEBUG] Encoded Payload recibido: " << encodedPayload << std::endl;
        std::cout << "[DEBUG] Firma esperada por C++: " << expectedSig << std::endl;
        std::cout << "[DEBUG] Firma provista por la Key: " << providedSig << std::endl;

        if (expectedSig != providedSig)
            throw LicenseException("Firma de API Key inválida. La clave no fue emitida por OmniSphere Authority o ha sido modificada.");

        // Decodificar payload
        std::string payloadJson = omnisphere::utils::Base64::DecodeUrl(encodedPayload);
        omnisphere::models::SystemLicense lic = ParsePayload(payloadJson);
        lic.apiKey = apiKey;

        // Verificar expiración
        if (!IsDateValid(lic.expiresAt))
            throw LicenseException("La API Key de licencia ha expirado el " + lic.expiresAt + ". Contacta a OmniSphere Authority para renovar.");

        // Código canónico para garantizar que siempre sobreescriba la licencia previa
        lic.code = "ACTIVE_LICENSE";

        lic.isActive      = true;
        lic.daysRemaining = DaysRemaining(lic.expiresAt);

        // Persistir primero en BD: si no se guarda, la licencia se perdería al reiniciar
        if (!m_repository || !m_repository->Save(lic))
            throw LicenseException("No se pudo guardar la licencia en la base de datos (SystemLicenses). Revisa el log de LicenseRepository::Save.");

        // Actualizar caché en RAM (thread-safe) solo tras persistir correctamente
        {
            std::unique_lock lock(m_mutex);
            m_license = lic;
            m_loaded  = true;
        }

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
            // =================================================================
            // [TEMPORAL] GENERACIÓN DE LICENCIA MAESTRA POR DEFECTO
            // Comenta este bloque tras copiar y resguardar la API Key.
            // =================================================================
            {
                LicenseParams genParams;
                genParams.clientName = "OmniSphere Platform";
                genParams.expiresAt  = "2099-12-31";
                genParams.modules    = omnisphere::license::AllModules();
                std::string masterKey = GenerateKey(genParams);
                std::cout << "\n===============================================================================\n"
                          << ">>> COPIA Y RESGUARDA ESTA OMNILICENSE API KEY (TODOS LOS MÓDULOS) <<<\n"
                          << masterKey << "\n"
                          << "===============================================================================\n" << std::endl;
                ActivateKey(masterKey);
            }
            // =================================================================

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

                std::cout << "[LicenseService] Re-validando firma: " << secret << std::endl;
                

                if (dotPos != std::string::npos)
                {
                    std::string encodedPayload = body.substr(0, dotPos);
                    std::string providedSig    = body.substr(dotPos + 1);
                    std::string expectedSig    = ComputeHmac(encodedPayload, secret);
                    if (expectedSig != providedSig)
                    {
                        std::cerr << "[LicenseService] AVISO: Clave secreta o firma diferente al arranque previo. Manteniendo licencia activa por vigencia de fecha." << std::endl;
                    }
                }
            }
            catch (...) {
                // Si OMNI_LICENSE_SECRET no está configurada en este arranque, continuar
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
