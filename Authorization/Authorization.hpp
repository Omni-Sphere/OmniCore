#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Models/AuditLog.hpp"
#include "Authorization/Models/AuthorizationResult.hpp"
#include "Authorization/Enums/ModuleType.hpp"
#include "Authorization/Enums/PermissionType.hpp"
#include "Authorization/DTOs/GrantPermission.hpp"
#include "Authorization/DTOs/RevokePermission.hpp"
#include "Authorization/DTOs/GrantRolePermission.hpp"
#include "Authorization/DTOs/RevokeRolePermission.hpp"
#include "Authorization/DTOs/GrantModule.hpp"
#include "Authorization/DTOs/RevokeModule.hpp"
#include "Authorization/DTOs/GrantRoleModule.hpp"
#include "Authorization/DTOs/RevokeRoleModule.hpp"
#include "Authorization/DTOs/SetUserModules.hpp"
#include "Authorization/DTOs/SetRoleModules.hpp"
#include "Authorization/DTOs/SetUserPermissions.hpp"
#include "Authorization/DTOs/SetRolePermissions.hpp"
#include "Authorization/Repositories/Authorization.hpp"

namespace omnisphere::services
{
    class AccessDeniedException : public std::runtime_error
    {
    public:
        explicit AccessDeniedException(const std::string& message)
            : std::runtime_error(message) {}
    };

    class Authorization
    {
    private:
        std::shared_ptr<omnisphere::repositories::Authorization> m_repository;

    public:
        explicit Authorization(std::shared_ptr<omnisphere::repositories::Authorization> repository);
        explicit Authorization(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~Authorization() = default;

        void RequireAuthenticated(const omnisphere::models::SecurityContext& ctx) const;

        // Autorizaciones en 2 Niveles Tipados
        void AuthorizeModule(const omnisphere::models::SecurityContext& ctx, omnisphere::enums::ModuleType module) const;
        void AuthorizeAction(const omnisphere::models::SecurityContext& ctx, omnisphere::enums::PermissionType permission) const;
        bool HasModule(const omnisphere::models::SecurityContext& ctx, omnisphere::enums::ModuleType module) const;
        bool HasAction(const omnisphere::models::SecurityContext& ctx, omnisphere::enums::PermissionType permission) const;

        // Métodos de compatibilidad
        void Authorize(const omnisphere::models::SecurityContext& ctx, const std::string& requiredPermission) const;
        void AuthorizeRoles(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& allowedRoles) const;
        bool HasPermission(const omnisphere::models::SecurityContext& ctx, const std::string& permission) const;

        void LogAudit(const omnisphere::models::SecurityContext& ctx, const std::string& module, const std::string& permission, const std::string& resourceCode, bool isGranted, const std::string& reason = "") const;

        // Gestión de Módulos (Nivel 1)
        omnisphere::models::AuthorizationResult GrantUserModule(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantUserModuleInput& input) const;
        omnisphere::models::AuthorizationResult RevokeUserModule(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokeUserModuleInput& input) const;
        omnisphere::models::AuthorizationResult GrantRoleModule(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantRoleModuleInput& input) const;
        omnisphere::models::AuthorizationResult RevokeRoleModule(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokeRoleModuleInput& input) const;
        omnisphere::models::AuthorizationResult SetUserModules(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetUserModulesInput& input) const;
        omnisphere::models::AuthorizationResult SetRoleModules(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetRoleModulesInput& input) const;
        std::vector<std::string> GetUserModules(const std::string& userCode) const;
        std::vector<std::string> GetRoleModules(const std::string& roleCode) const;

        // Gestión de Permisos por Operación (Nivel 2)
        omnisphere::models::AuthorizationResult GrantUserPermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantPermissionInput& input) const;
        omnisphere::models::AuthorizationResult RevokeUserPermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokePermissionInput& input) const;
        omnisphere::models::AuthorizationResult GrantRolePermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantRolePermissionInput& input) const;
        omnisphere::models::AuthorizationResult RevokeRolePermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokeRolePermissionInput& input) const;
        omnisphere::models::AuthorizationResult SetUserPermissions(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetUserPermissionsInput& input) const;
        omnisphere::models::AuthorizationResult SetRolePermissions(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::SetRolePermissionsInput& input) const;

        std::vector<omnisphere::models::PermissionModule> GetPermissionsCatalog() const;
        std::vector<std::string> GetUserPermissions(const std::string& userCode) const;
        std::vector<std::string> GetRolePermissions(const std::string& roleCode) const;
        std::vector<std::string> GetUserOverridePermissions(const std::string& userCode) const;
        std::vector<std::string> GetRoleOverridePermissions(const std::string& roleCode) const;
        bool CanRequestOverride(const omnisphere::models::SecurityContext& ctx, const std::string& permission) const;
        std::vector<omnisphere::models::Role> GetAllRoles(const std::vector<std::string>& fields = {}) const;

        bool CreateRole(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::Role& role) const;
        bool UpdateRole(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::Role& role) const;
        bool DeleteRole(const omnisphere::models::SecurityContext& ctx, const std::string& roleCode) const;
    };
} // namespace omnisphere::services
