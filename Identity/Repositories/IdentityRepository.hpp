#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <OmniData/DatabasePool.hpp>
#include "Identity/Models/Identity.hpp"
#include "Identity/DTOs/CreateIdentity.hpp"

namespace omnisphere::repositories
{
    class IdentityRepository
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

    public:
        explicit IdentityRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~IdentityRepository() = default;

        bool Create(const omnisphere::dtos::CreateIdentityInput& input) const;
        std::string GetNextCode(const std::string& domain, const std::string& defaultPrefix = "", int prefixIndex = 1) const;
        omnisphere::types::DataTable ReadAll(const std::vector<std::string>& fields = {}) const;
        omnisphere::types::DataTable GetByDomain(const std::string& domain, const std::vector<std::string>& fields = {}) const;
    };
} // namespace omnisphere::repositories
