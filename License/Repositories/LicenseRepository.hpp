#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <stdexcept>
#include "License/Models/SystemLicense.hpp"
#include <OmniData/DatabasePool.hpp>

// =============================================================================
// LicenseRepository.hpp
// Acceso a base de datos exclusivamente para persistir y recuperar la
// OmniLicense API Key activa. La VALIDACIÓN criptográfica se realiza en
// LicenseService (en memoria RAM), no aquí.
// =============================================================================

namespace omnisphere::repositories
{
    class LicenseRepository
    {
    public:
        explicit LicenseRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~LicenseRepository() = default;

        /// Persiste o actualiza la licencia activa en "SystemLicenses".
        /// Desactiva cualquier licencia anterior antes de insertar la nueva.
        bool Save(const omnisphere::models::SystemLicense& license) const;

        /// Recupera la licencia activa actual (si existe) para restaurarla al reiniciar.
        std::optional<omnisphere::models::SystemLicense> GetActive() const;

        /// Desactiva la licencia actual (para revocar acceso inmediatamente).
        bool Deactivate(const std::string& code) const;

        /// Lista todas las licencias históricas registradas.
        std::vector<omnisphere::models::SystemLicense> GetAll() const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };

} // namespace omnisphere::repositories
