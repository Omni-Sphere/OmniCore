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

#include "Authorization/Models/Role.hpp"
#include "Authorization/Models/Permission.hpp"
#include "Authorization/DTOs/SetUserPermissions.hpp"
#include "Authorization/DTOs/SetRolePermissions.hpp"

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
        bool CheckRole(const std::string& userCode, const std::vector<std::string>& allowedRoles) const;
        void LogAudit(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::AuditLogEntry& entry) const;

        std::vector<omnisphere::models::PermissionModule> GetPermissionsCatalog() const;
        std::vector<std::string> GetUserPermissions(const std::string& userCode) const;
        std::vector<std::string> GetRolePermissions(const std::string& roleCode) const;
        std::vector<omnisphere::models::Role> GetAllRoles(const std::vector<std::string>& fields = {}) const;

        bool SetUserPermissions(const omnisphere::dtos::SetUserPermissionsInput& input) const;
        bool SetRolePermissions(const omnisphere::dtos::SetRolePermissionsInput& input) const;

        bool CreateRole(const omnisphere::models::Role& role) const;
        bool UpdateRole(const omnisphere::models::Role& role) const;
        bool DeleteRole(const std::string& roleCode) const;

        bool GrantUserPermission(const omnisphere::dtos::GrantPermissionInput& input) const;
        bool RevokeUserPermission(const omnisphere::dtos::RevokePermissionInput& input) const;

        bool GrantRolePermission(const omnisphere::dtos::GrantRolePermissionInput& input) const;
        bool RevokeRolePermission(const omnisphere::dtos::RevokeRolePermissionInput& input) const;
    };
} // namespace omnisphere::repositories

