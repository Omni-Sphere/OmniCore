#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <atomic>
#include <shared_mutex>
#include "License/Models/SystemLicense.hpp"
#include "License/Models/LicenseModules.hpp"
#include "License/Repositories/LicenseRepository.hpp"
#include <OmniData/DatabasePool.hpp>

// =============================================================================
// LicenseService.hpp
// Motor de licenciamiento OmniSphere.
//
// Formato de API Key:
//   OMNI-<Base64URL(payload_json)>.<Base64URL(HMAC-SHA256(payload, secret))>
//
// Payload JSON:
//   {"client":"Acme","issuer":"OmniSphere Authority","iat":"2026-09-15",
//    "exp":"2027-12-31","modules":["MODULE_WHATSAPP","MODULE_STRIPE"]}
//
// La validación es 100% en memoria RAM (OpenSSL HMAC).
// La BD solo se usa para persistencia entre reinicios y auditoría.
//
// Variable de entorno requerida: OMNI_LICENSE_SECRET
// =============================================================================

namespace omnisphere::services
{
    /// Excepción lanzada cuando un módulo no está licenciado o la key es inválida.
    class LicenseException : public std::runtime_error
    {
    public:
        explicit LicenseException(const std::string& message)
            : std::runtime_error(message) {}
    };

    /// Parámetros para generar una nueva API Key
    struct LicenseParams
    {
        std::string clientName;
        std::string issuer = "OmniSphere Authority";
        std::string issuedAt;           // "YYYY-MM-DD" — vacío = hoy
        std::string expiresAt;          // "YYYY-MM-DD" requerido
        std::vector<std::string> modules;
    };

    class LicenseService
    {
    public:
        explicit LicenseService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        explicit LicenseService(std::shared_ptr<omnisphere::repositories::LicenseRepository> repository);
        ~LicenseService() = default;

        // -----------------------------------------------------------------------
        // Instancia Compartida Global (para reactividad en todo el proceso)
        // -----------------------------------------------------------------------
        static std::shared_ptr<LicenseService> GetSharedInstance();
        static void SetSharedInstance(std::shared_ptr<LicenseService> instance);

        // -----------------------------------------------------------------------
        // Generación (uso interno del sistema OmniSphere)
        // -----------------------------------------------------------------------

        /// Genera una nueva OmniLicense API Key firmada con HMAC-SHA256 usando la clave de BD.
        /// @return La API Key completa: "OMNI-<payload>.<signature>"
        std::string GenerateKey(const LicenseParams& params) const;

        /// Genera una nueva OmniLicense API Key firmada con un secreto explícito.
        static std::string GenerateKey(const LicenseParams& params, const std::string& masterSecret);

        // -----------------------------------------------------------------------
        // Activación y validación
        // -----------------------------------------------------------------------

        /// Valida la firma y expiración de la API Key, carga módulos en RAM y
        /// persiste la key en BD. Si la key es inválida lanza LicenseException.
        /// Debe llamarse desde el resolver GraphQL "applyActivateLicense".
        /// @return La licencia decodificada y activa
        omnisphere::models::SystemLicense ActivateKey(const std::string& apiKey);

        /// Carga la licencia activa desde la BD al arrancar el servidor.
        /// Si la key guardada ya expiró, la desactiva automáticamente.
        /// Se llama desde ServiceInjector al inicializar.
        bool LoadFromDatabase();

        /// Revoca la licencia activa — todos los módulos quedan bloqueados.
        bool RevokeActive();

        // -----------------------------------------------------------------------
        // Consulta de estado (O(1) — RAM, sin BD)
        // -----------------------------------------------------------------------

        /// true si el módulo está incluido en la licencia activa y vigente.
        /// Lanza LicenseException si se llama como RequireModule() en servicios.
        bool IsModuleLicensed(const std::string& moduleCode) const;

        /// Igual que IsModuleLicensed pero lanza LicenseException automáticamente.
        /// Úsalo al inicio de cada operación protegida en los servicios C++.
        void RequireModule(const std::string& moduleCode) const;

        /// Devuelve la licencia activa en memoria (copia thread-safe).
        std::optional<omnisphere::models::SystemLicense> GetActiveLicense() const;

        /// true si hay alguna licencia activa y no expirada cargada en RAM.
        bool IsLicenseActive() const;

    private:
        std::shared_ptr<omnisphere::repositories::LicenseRepository> m_repository;
        mutable std::shared_mutex m_mutex;
        omnisphere::models::SystemLicense m_license;
        bool m_loaded = false;

        // -----------------------------------------------------------------------
        // Helpers criptográficos internos
        // -----------------------------------------------------------------------

        /// Calcula HMAC-SHA256 con OpenSSL y retorna el resultado en Base64URL
        static std::string ComputeHmac(const std::string& data, const std::string& secret);

        /// Codifica bytes en Base64URL (sin padding '=')
        static std::string Base64UrlEncode(const std::vector<unsigned char>& data);
        static std::string Base64UrlEncode(const std::string& data);

        /// Decodifica Base64URL → string
        static std::string Base64UrlDecode(const std::string& encoded);

        /// Parsea el payload JSON y rellena un SystemLicense (sin validar firma)
        static omnisphere::models::SystemLicense ParsePayload(const std::string& payloadJson);

        /// Verifica que la fecha de expiración sea posterior a hoy
        static bool IsDateValid(const std::string& expiresAt);

        /// Calcula días restantes desde hoy hasta expiresAt
        static int DaysRemaining(const std::string& expiresAt);

        /// Lee la clave maestra HMAC-SHA256 desde GlobalConfiguration (BD) o fallback.
        std::string GetMasterSecret() const;
    };

} // namespace omnisphere::services
