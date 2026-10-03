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
            throw AccessDeniedException("Se requiere autenticación de usuario para acceder a esta función.");
        }
    }

    void Authorization::AuthorizeModule(const omnisphere::models::SecurityContext& ctx, omnisphere::enums::ModuleType module) const
    {
        RequireAuthenticated(ctx);
        if (!HasModule(ctx, module))
        {
            throw AccessDeniedException("No se tiene acceso al módulo solicitado: " + omnisphere::enums::ModuleTypeToString(module));
        }
    }

    void Authorization::AuthorizeAction(const omnisphere::models::SecurityContext& ctx, omnisphere::enums::PermissionType permission) const
    {
        RequireAuthenticated(ctx);
        if (!HasAction(ctx, permission))
        {
            if (!ctx.grantedByCode.empty() && m_repository && m_repository->CheckAction(ctx.grantedByCode, permission))
            {
                return;
            }
            throw AccessDeniedException("No se tienen los permisos necesarios para ejecutar la acción: " + omnisphere::enums::PermissionTypeToString(permission));
        }
    }

    bool Authorization::HasModule(const omnisphere::models::SecurityContext& ctx, omnisphere::enums::ModuleType module) const
    {
        if (!ctx.isAuthenticated()) return false;
        if (!m_repository) return true;
        return m_repository->CheckModule(ctx.userCode, module);
    }

    bool Authorization::HasAction(const omnisphere::models::SecurityContext& ctx, omnisphere::enums::PermissionType permission) const
    {
        if (!ctx.isAuthenticated()) return false;
        if (!m_repository) return true;
        return m_repository->CheckAction(ctx.userCode, permission);
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

            throw AccessDeniedException("No se tienen los permisos necesarios para ejecutar la función solicitada.");
        }
    }

    void Authorization::AuthorizeRoles(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& allowedRoles) const
    {
        RequireAuthenticated(ctx);

        if (m_repository && m_repository->CheckRole(ctx.userCode, allowedRoles))
        {
            return;
        }

        throw AccessDeniedException("No se tienen los permisos necesarios para ejecutar la función solicitada.");
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
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_PERM_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            omnisphere::dtos::GrantPermissionInput finalInput = input;
            if (finalInput.grantedByCode.empty())
            {
                finalInput.grantedByCode = ctx.userCode;
            }
            ok = m_repository->GrantUserPermission(finalInput);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Permiso otorgado correctamente al usuario" : "Error al otorgar permiso al usuario";
        result.userCode = input.userCode;
        result.permission = omnisphere::enums::PermissionTypeToString(input.permission);

        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::RevokeUserPermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokePermissionInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_PERM_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            ok = m_repository->RevokeUserPermission(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Permiso revocado correctamente del usuario" : "Error al revocar permiso del usuario";
        result.userCode = input.userCode;
        result.permission = omnisphere::enums::PermissionTypeToString(input.permission);

        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::GrantRolePermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantRolePermissionInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            ok = m_repository->GrantRolePermission(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Permiso otorgado correctamente al rol" : "Error al otorgar permiso al rol";
        result.userCode = input.roleCode;
        result.permission = omnisphere::enums::PermissionTypeToString(input.permission);

        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::RevokeRolePermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokeRolePermissionInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            ok = m_repository->RevokeRolePermission(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Permiso revocado correctamente del rol" : "Error al revocar permiso del rol";
        result.userCode = input.roleCode;
        result.permission = omnisphere::enums::PermissionTypeToString(input.permission);

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

    std::vector<std::string> Authorization::GetUserOverridePermissions(const std::string& userCode) const
    {
        if (m_repository)
        {
            return m_repository->GetUserOverridePermissions(userCode);
        }
        return {};
    }

    std::vector<std::string> Authorization::GetRoleOverridePermissions(const std::string& roleCode) const
    {
        if (m_repository)
        {
            return m_repository->GetRoleOverridePermissions(roleCode);
        }
        return {};
    }

    bool Authorization::CanRequestOverride(const omnisphere::models::SecurityContext& ctx, const std::string& permission) const
    {
        if (!ctx.isAuthenticated() || ctx.userCode.empty()) return false;
        if (m_repository)
        {
            return m_repository->CheckPermissionOverride(ctx.userCode, permission);
        }
        return false;
    }

    std::vector<omnisphere::models::Role> Authorization::GetAllRoles(const std::vector<std::string>& fields) const
    {
        if (m_repository)
        {
            return m_repository->GetAllRoles(fields);
        }
        return {};
    }

    omnisphere::models::AuthorizationResult Authorization::GrantUserModule(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantUserModuleInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_PERM_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            omnisphere::dtos::GrantUserModuleInput finalInput = input;
            if (finalInput.grantedByCode.empty())
            {
                finalInput.grantedByCode = ctx.userCode;
            }
            ok = m_repository->GrantUserModule(finalInput);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Módulo otorgado correctamente al usuario" : "Error al otorgar módulo al usuario";
        result.userCode = input.userCode;
        result.permission = omnisphere::enums::ModuleTypeToString(input.module);
        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::RevokeUserModule(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokeUserModuleInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_PERM_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            ok = m_repository->RevokeUserModule(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Módulo revocado correctamente del usuario" : "Error al revocar módulo del usuario";
        result.userCode = input.userCode;
        result.permission = omnisphere::enums::ModuleTypeToString(input.module);
        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::GrantRoleModule(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantRoleModuleInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            ok = m_repository->GrantRoleModule(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Módulo otorgado correctamente al rol" : "Error al otorgar módulo al rol";
        result.userCode = input.roleCode;
        result.permission = omnisphere::enums::ModuleTypeToString(input.module);
        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::RevokeRoleModule(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokeRoleModuleInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            ok = m_repository->RevokeRoleModule(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Módulo revocado correctamente del rol" : "Error al revocar módulo del rol";
        result.userCode = input.roleCode;
        result.permission = omnisphere::enums::ModuleTypeToString(input.module);
        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::SetUserModules(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetUserModulesInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_PERM_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            omnisphere::dtos::SetUserModulesInput finalInput = input;
            if (finalInput.grantedByCode.empty())
            {
                finalInput.grantedByCode = ctx.userCode;
            }
            ok = m_repository->SetUserModules(finalInput);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Módulos de usuario actualizados correctamente" : "Error al actualizar módulos de usuario";
        result.userCode = input.userCode;
        return result;
    }

    omnisphere::models::AuthorizationResult Authorization::SetRoleModules(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetRoleModulesInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);

        bool ok = false;
        if (m_repository)
        {
            ok = m_repository->SetRoleModules(input);
        }

        omnisphere::models::AuthorizationResult result;
        result.success = ok;
        result.message = ok ? "Módulos de rol actualizados correctamente" : "Error al actualizar módulos de rol";
        result.userCode = input.roleCode;
        return result;
    }

    std::vector<std::string> Authorization::GetUserModules(const std::string& userCode) const
    {
        if (m_repository)
        {
            return m_repository->GetUserModules(userCode);
        }
        return {};
    }

    std::vector<std::string> Authorization::GetRoleModules(const std::string& roleCode) const
    {
        if (m_repository)
        {
            return m_repository->GetRoleModules(roleCode);
        }
        return {};
    }

    omnisphere::models::AuthorizationResult Authorization::SetUserPermissions(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetUserPermissionsInput& input) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_PERM_MANAGE);

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
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);

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
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);
        if (m_repository)
        {
            return m_repository->CreateRole(role);
        }
        return false;
    }

    bool Authorization::UpdateRole(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::Role& role) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);
        if (m_repository)
        {
            return m_repository->UpdateRole(role);
        }
        return false;
    }

    bool Authorization::DeleteRole(const omnisphere::models::SecurityContext& ctx, const std::string& roleCode) const
    {
        AuthorizeAction(ctx, omnisphere::enums::PermissionType::CORE_ROLE_MANAGE);
        if (m_repository)
        {
            return m_repository->DeleteRole(roleCode);
        }
        return false;
    }
} // namespace omnisphere::services

