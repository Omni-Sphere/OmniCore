#pragma once

#include <memory>
#include <string>
#include <vector>
#include <OmniData/DatabasePool.hpp>
#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Models/AuditLog.hpp"
#include "Authorization/DTOs/GrantPermission.hpp"
#include "Authorization/DTOs/RevokePermission.hpp"
#include "Authorization/DTOs/GrantRolePermission.hpp"
#include "Authorization/DTOs/RevokeRolePermission.hpp"

namespace omnisphere::repositories
{
    class Authorization
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

    public:
        explicit Authorization(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~Authorization() = default;

        bool CheckPermission(const std::string& userCode, const std::string& permission) const;
        bool CheckRole(const std::string& userRole, const std::vector<std::string>& allowedRoles) const;
        void LogAudit(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::AuditLogEntry& entry) const;

        bool GrantUserPermission(const omnisphere::dtos::GrantPermissionInput& input) const;
        bool RevokeUserPermission(const omnisphere::dtos::RevokePermissionInput& input) const;

        bool GrantRolePermission(const omnisphere::dtos::GrantRolePermissionInput& input) const;
        bool RevokeRolePermission(const omnisphere::dtos::RevokeRolePermissionInput& input) const;
    };
} // namespace omnisphere::repositories
