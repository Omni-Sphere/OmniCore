#pragma once
#include <string>
#include <unordered_set>
#include <vector>

// =============================================================================
// SystemLicense.hpp
// Modelo de datos para una OmniLicense API Key decodificada.
// El payload se extrae de la API Key firmada — nunca se almacena en claro
// más que en memoria RAM durante el ciclo de vida del servidor.
// =============================================================================

namespace omnisphere::models
{
    /// Representa una licencia activa decodificada desde una OmniLicense API Key.
    /// Campos cargados en RAM al arrancar el servidor o al activar una nueva key.
    struct SystemLicense
    {
        /// Código único de la licencia en BD (ej. "LIC1")
        std::string code;

        /// La API Key completa: OMNI-<Base64URL_payload>.<Base64URL_sig>
        std::string apiKey;

        /// Nombre del cliente/organización licenciado
        std::string clientName;

        /// Emisor de la licencia (por defecto "OmniSphere Authority")
        std::string issuer;

        /// Fecha de emisión en formato ISO "YYYY-MM-DD"
        std::string issuedAt;

        /// Fecha de expiración en formato ISO "YYYY-MM-DD"
        std::string expiresAt;

        /// Conjunto de códigos de módulo activos (lookup O(1))
        /// Ejemplo: {"MODULE_WHATSAPP", "MODULE_STRIPE", "MODULE_ANALYTICS"}
        std::unordered_set<std::string> modules;

        /// true si la licencia está activa y no ha expirado
        bool isActive = false;

        /// Días restantes de vigencia (calculado al momento de la carga)
        int daysRemaining = 0;

        /// Convierte los módulos activos a vector para serialización
        std::vector<std::string> GetModuleList() const
        {
            return std::vector<std::string>(modules.begin(), modules.end());
        }

        /// Verifica si un módulo específico está licenciado
        bool HasModule(const std::string& moduleCode) const
        {
            return modules.count(moduleCode) > 0;
        }
    };

} // namespace omnisphere::models
