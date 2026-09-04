#include "Identity/IdentityService.hpp"
#include <iostream>

namespace omnisphere::services
{
    IdentityService::IdentityService(std::shared_ptr<omnisphere::repositories::IdentityRepository> identityRepo)
        : m_identityRepo(std::move(identityRepo)) {}

    IdentityService::IdentityService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_identityRepo(std::make_shared<omnisphere::repositories::IdentityRepository>(std::move(dbPool))) {}

    bool IdentityService::Create(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreateIdentityInput& input) const
    {
        return m_identityRepo ? m_identityRepo->Create(input) : false;
    }

    std::string IdentityService::GetNextCode(const omnisphere::models::SecurityContext& ctx, const std::string& domain, const std::string& defaultPrefix, int prefixIndex) const
    {
        return m_identityRepo ? m_identityRepo->GetNextCode(domain, defaultPrefix, prefixIndex) : "";
    }

    omnisphere::types::DataTable IdentityService::ReadAll(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields) const
    {
        return m_identityRepo ? m_identityRepo->ReadAll(fields) : omnisphere::types::DataTable{};
    }

    omnisphere::types::DataTable IdentityService::GetByDomain(const omnisphere::models::SecurityContext& ctx, const std::string& domain, const std::vector<std::string>& fields) const
    {
        return m_identityRepo ? m_identityRepo->GetByDomain(domain, fields) : omnisphere::types::DataTable{};
    }
} // namespace omnisphere::services
