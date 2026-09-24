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

        if (m_repository && m_repository->CheckRole(ctx.userCode, allowedRoles))
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

    std::vector<omnisphere::models::PermissionModule> Authorization::GetPermissionsCatalog() const
    {
        if (m_repository)
        {
            return m_repository->GetPermissionsCatalog();
        }
        return {};
    }

    std::vector<std::string> Authorization::GetUserPermissions(const std::string& userCode) const
    {
        if (m_repository)
        {
            return m_repository->GetUserPermissions(userCode);
        }
        return {};
    }

    std::vector<std::string> Authorization::GetRolePermissions(const std::string& roleCode) const
    {
        if (m_repository)
        {
            return m_repository->GetRolePermissions(roleCode);
        }
        return {};
    }

    std::vector<omnisphere::models::Role> Authorization::GetAllRoles(const std::vector<std::string>& fields) const
    {
        if (m_repository)
        {
            return m_repository->GetAllRoles(fields);
        }
        return {};
    }

    omnisphere::models::AuthorizationResult Authorization::SetUserPermissions(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetUserPermissionsInput& input) const
    {
        Authorize(ctx, "CORE_PERM_MANAGE");

        bool ok = false;
        if (m_repository)
        {
            omnisphere::dtos::SetUserPermissionsInput finalInput = input;
            if (finalInput.grantedByCode.empty())
            {
                finalInput.grantedByCode = ctx.userCode;
            }
            ok = m_repository->SetUserPermissions(finalInput);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Permisos de usuario actualizados correctamente" : "Error al actualizar permisos de usuario";
        result.userCode = input.userCode;
        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::SetRolePermissions(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetRolePermissionsInput& input) const
    {
        Authorize(ctx, "CORE_ROLE_MANAGE");

        bool ok = false;
        if (m_repository)
        {
            ok = m_repository->SetRolePermissions(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Permisos de rol actualizados correctamente" : "Error al actualizar permisos de rol";
        result.userCode = input.roleCode;
        return result;
    }

    bool Authorization::CreateRole(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::Role& role) const
    {
        Authorize(ctx, "CORE_ROLE_MANAGE");
        if (m_repository)
        {
            return m_repository->CreateRole(role);
        }
        return false;
    }

    bool Authorization::UpdateRole(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::Role& role) const
    {
        Authorize(ctx, "CORE_ROLE_MANAGE");
        if (m_repository)
        {
            return m_repository->UpdateRole(role);
        }
        return false;
    }

    bool Authorization::DeleteRole(const omnisphere::models::SecurityContext& ctx, const std::string& roleCode) const
    {
        Authorize(ctx, "CORE_ROLE_MANAGE");
        if (m_repository)
        {
            return m_repository->DeleteRole(roleCode);
        }
        return false;
    }
} // namespace omnisphere::services

