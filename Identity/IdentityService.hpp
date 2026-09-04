#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "Identity/Repositories/IdentityRepository.hpp"
#include "Identity/DTOs/CreateIdentity.hpp"
#include "Identity/Models/Identity.hpp"
#include <OmniCore/Authorization/Models/SecurityContext.hpp>

namespace omnisphere::services
{
    class IdentityService
    {
    private:
        std::shared_ptr<omnisphere::repositories::IdentityRepository> m_identityRepo;

    public:
        explicit IdentityService(std::shared_ptr<omnisphere::repositories::IdentityRepository> identityRepo);
        explicit IdentityService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~IdentityService() = default;

        bool Create(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreateIdentityInput& input) const;
        std::string GetNextCode(const omnisphere::models::SecurityContext& ctx, const std::string& domain, const std::string& defaultPrefix = "", int prefixIndex = 1) const;
        omnisphere::types::DataTable ReadAll(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields = {}) const;
        omnisphere::types::DataTable GetByDomain(const omnisphere::models::SecurityContext& ctx, const std::string& domain, const std::vector<std::string>& fields = {}) const;
    };
} // namespace omnisphere::services
