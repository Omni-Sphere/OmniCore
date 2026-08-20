#include "Authorization/Authorization.hpp"
#include "Authorization/DTOs/GrantRolePermission.hpp"
#include "Authorization/DTOs/RevokeRolePermission.hpp"
#include <iostream>

namespace omnisphere::services
{
    Authorization::Authorization(std::shared_ptr<omnisphere::repositories::Authorization> repository)
        : m_repository(std::move(repository)) {}

    Authorization::Authorization(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repository(std::make_shared<omnisphere::repositories::Authorization>(std::move(dbPool))) {}

    void Authorization::RequireAuthenticated(const omnisphere::models::SecurityContext& ctx) const
    {
        if (!ctx.isAuthenticated())
        {
            throw AccessDeniedException("401 Unauthorized: User authentication required.");
        }
    }

    bool Authorization::HasPermission(const omnisphere::models::SecurityContext& ctx, const std::string& permission) const
    {
        if (!ctx.isAuthenticated()) return false;
        if (ctx.isSuperAdmin()) return true;
        if (!m_repository) return true;

        return m_repository->CheckPermission(ctx.userCode, permission);
    }

    void Authorization::Authorize(const omnisphere::models::SecurityContext& ctx, const std::string& requiredPermission) const
    {
        RequireAuthenticated(ctx);

        if (!HasPermission(ctx, requiredPermission))
        {
            if (!ctx.grantedByCode.empty())
            {
                if (m_repository && m_repository->CheckPermission(ctx.grantedByCode, requiredPermission))
                {
                    return;
                }
            }

            throw AccessDeniedException("403 Forbidden: Missing required permission '" + requiredPermission + "'.");
        }
    }

    void Authorization::AuthorizeRoles(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& allowedRoles) const
    {
        RequireAuthenticated(ctx);
        if (ctx.isSuperAdmin()) return;

        if (m_repository && m_repository->CheckRole(ctx.userRole, allowedRoles))
        {
            return;
        }

        throw AccessDeniedException("403 Forbidden: Insufficient role privileges.");
    }

    void Authorization::LogAudit(const omnisphere::models::SecurityContext& ctx, const std::string& module, const std::string& permission, const std::string& resourceCode, bool isGranted, const std::string& reason) const
    {
        if (m_repository)
        {
            omnisphere::models::AuditLogEntry entry;
            entry.userCode = ctx.userCode;
            entry.grantedByCode = ctx.grantedByCode;
            entry.module = module;
            entry.permission = permission;
            entry.resourceCode = resourceCode;
            entry.status = isGranted ? "GRANTED" : "DENIED";
            entry.reason = reason;

            m_repository->LogAudit(ctx, entry);
        }
    }

    omnisphere::models::AuthorizationResult Authorization::GrantUserPermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantPermissionInput& input) const
    {
        Authorize(ctx, "PERMISSION_GRANT");

        if (m_repository)
        {
            m_repository->GrantUserPermission(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = true;
        result.message = "Permission '" + input.permission + "' successfully granted to user '" + input.userCode + "'";
        result.userCode = input.userCode;
        result.permission = input.permission;

        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::RevokeUserPermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokePermissionInput& input) const
    {
        Authorize(ctx, "PERMISSION_REVOKE");

        if (m_repository)
        {
            m_repository->RevokeUserPermission(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = true;
        result.message = "Permission '" + input.permission + "' successfully revoked from user '" + input.userCode + "'";
        result.userCode = input.userCode;
        result.permission = input.permission;

        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::GrantRolePermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantRolePermissionInput& input) const
    {
        Authorize(ctx, "ROLE_PERMISSION_GRANT");

        if (m_repository)
        {
            m_repository->GrantRolePermission(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = true;
        result.message = "Permission '" + input.permission + "' successfully granted to role '" + input.roleCode + "'";
        result.userCode = input.roleCode;
        result.permission = input.permission;

        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::RevokeRolePermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokeRolePermissionInput& input) const
    {
        Authorize(ctx, "ROLE_PERMISSION_REVOKE");

        if (m_repository)
        {
            m_repository->RevokeRolePermission(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = true;
        result.message = "Permission '" + input.permission + "' successfully revoked from role '" + input.roleCode + "'";
        result.userCode = input.roleCode;
        result.permission = input.permission;

        return result;
    }
} // namespace omnisphere::services
