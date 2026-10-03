#pragma once

#include <memory>
#include <string>
#include <vector>
#include <OmniData/DatabasePool.hpp>
#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Models/AuditLog.hpp"
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

        // Validaciones en 2 Niveles
        bool CheckModule(const std::string& userCode, omnisphere::enums::ModuleType module) const;
        bool CheckAction(const std::string& userCode, omnisphere::enums::PermissionType permission) const;

        // Compatibilidad con cadenas
        bool CheckPermission(const std::string& userCode, const std::string& permission) const;
        bool CheckRole(const std::string& userCode, const std::vector<std::string>& allowedRoles) const;
        void LogAudit(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::AuditLogEntry& entry) const;

        std::vector<omnisphere::models::PermissionModule> GetPermissionsCatalog() const;
        std::vector<std::string> GetUserPermissions(const std::string& userCode) const;
        std::vector<std::string> GetRolePermissions(const std::string& roleCode) const;
        std::vector<std::string> GetUserOverridePermissions(const std::string& userCode) const;
        std::vector<std::string> GetRoleOverridePermissions(const std::string& roleCode) const;
        bool CheckPermissionOverride(const std::string& userCode, const std::string& permission) const;
        std::vector<omnisphere::models::Role> GetAllRoles(const std::vector<std::string>& fields = {}) const;

        // Gestión Masiva de Permisos
        bool SetUserPermissions(const omnisphere::dtos::SetUserPermissionsInput& input) const;
        bool SetRolePermissions(const omnisphere::dtos::SetRolePermissionsInput& input) const;

        // Gestión Masiva de Módulos (Nivel 1)
        bool SetUserModules(const omnisphere::dtos::SetUserModulesInput& input) const;
        bool SetRoleModules(const omnisphere::dtos::SetRoleModulesInput& input) const;
        std::vector<std::string> GetUserModules(const std::string& userCode) const;
        std::vector<std::string> GetRoleModules(const std::string& roleCode) const;

        bool CreateRole(const omnisphere::models::Role& role) const;
        bool UpdateRole(const omnisphere::models::Role& role) const;
        bool DeleteRole(const std::string& roleCode) const;

        // Permisos Unitarios
        bool GrantUserPermission(const omnisphere::dtos::GrantPermissionInput& input) const;
        bool RevokeUserPermission(const omnisphere::dtos::RevokePermissionInput& input) const;
        bool GrantRolePermission(const omnisphere::dtos::GrantRolePermissionInput& input) const;
        bool RevokeRolePermission(const omnisphere::dtos::RevokeRolePermissionInput& input) const;

        // Módulos Unitarios
        bool GrantUserModule(const omnisphere::dtos::GrantUserModuleInput& input) const;
        bool RevokeUserModule(const omnisphere::dtos::RevokeUserModuleInput& input) const;
        bool GrantRoleModule(const omnisphere::dtos::GrantRoleModuleInput& input) const;
        bool RevokeRoleModule(const omnisphere::dtos::RevokeRoleModuleInput& input) const;
    };
} // namespace omnisphere::repositories
