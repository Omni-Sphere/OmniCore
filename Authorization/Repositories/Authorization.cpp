#include "Authorization/Repositories/Authorization.hpp"
#include <iostream>
#include <OmniData/Database.hpp>

namespace omnisphere::repositories
{
    Authorization::Authorization(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    bool Authorization::CheckPermission(const std::string& userCode, const std::string& permission) const
    {
        if (!m_dbPool) return true;

        try
        {
            auto conn = m_dbPool->Acquire();

            // 1. Obtener el RoleCode del usuario
            std::string userQuery = "SELECT \"RoleCode\" FROM \"Users\" WHERE \"Code\" = ?";
            std::vector<omnisphere::types::SQLParam> userParams = {
                omnisphere::types::MakeSQLParam(userCode)
            };
            auto userDt = conn->FetchPrepared(userQuery, userParams);

            std::string roleCode = "";
            if (userDt.RowsCount() > 0 && !userDt[0]["RoleCode"].IsNull())
            {
                roleCode = std::string(userDt[0]["RoleCode"]);
            }

            // CASO 1: Si el usuario TIENE un Rol asignado, consultar ÚNICAMENTE RolePermissions
            if (!roleCode.empty())
            {
                std::string roleQuery = "SELECT COUNT(1) AS \"Allowed\" FROM \"RolePermissions\" WHERE \"RoleCode\" = ? AND \"PermissionCode\" = ? AND \"IsAllowed\" = true AND \"IsActive\" = true";
                std::vector<omnisphere::types::SQLParam> roleParams = {
                    omnisphere::types::MakeSQLParam(roleCode),
                    omnisphere::types::MakeSQLParam(permission)
                };
                auto dt = conn->FetchPrepared(roleQuery, roleParams);
                if (dt.RowsCount() > 0)
                {
                    int count = dt[0]["Allowed"];
                    return count > 0;
                }
                return false;
            }
            else
            {
                // CASO 2: Si el usuario NO TIENE Rol asignado, consultar ÚNICAMENTE UserPermissions
                std::string userPermQuery = "SELECT COUNT(1) AS \"Allowed\" FROM \"UserPermissions\" WHERE \"UserCode\" = ? AND \"PermissionCode\" = ? AND \"IsAllowed\" = true AND \"IsActive\" = true";
                std::vector<omnisphere::types::SQLParam> userPermParams = {
                    omnisphere::types::MakeSQLParam(userCode),
                    omnisphere::types::MakeSQLParam(permission)
                };
                auto dt = conn->FetchPrepared(userPermQuery, userPermParams);
                if (dt.RowsCount() > 0)
                {
                    int count = dt[0]["Allowed"];
                    return count > 0;
                }
                return false;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[Authorization Repository SQL Error] " << ex.what() << std::endl;
        }

        return false;
    }

    bool Authorization::CheckRole(const std::string& userRole, const std::vector<std::string>& allowedRoles) const
    {
        for (const auto& role : allowedRoles)
        {
            if (role == userRole) return true;
        }
        return false;
    }

    void Authorization::LogAudit(const omnisphere::models::SecurityContext& ctx, const omnisphere::models::AuditLogEntry& entry) const
    {
        if (m_dbPool)
        {
            try
            {
                auto conn = m_dbPool->Acquire();
                std::string sql = "INSERT INTO \"AuthorizationAuditLog\" (\"UserCode\", \"GrantedByCode\", \"Module\", \"Permission\", \"ResourceCode\", \"Status\", \"Reason\") "
                                  "VALUES (?, ?, ?, ?, ?, ?, ?)";

                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(ctx.userCode),
                    omnisphere::types::MakeSQLParam(ctx.grantedByCode),
                    omnisphere::types::MakeSQLParam(entry.module),
                    omnisphere::types::MakeSQLParam(entry.permission),
                    omnisphere::types::MakeSQLParam(entry.resourceCode),
                    omnisphere::types::MakeSQLParam(entry.status),
                    omnisphere::types::MakeSQLParam(entry.reason)
                };

                conn->RunPrepared(sql, params);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "[Audit Service SQL Error] " << ex.what() << std::endl;
            }
        }

        std::cout << "[OmniCore::Audit] LOG -> User: '" << ctx.userCode
                  << "' | GrantedBy: '" << ctx.grantedByCode
                  << "' | Module: '" << entry.module
                  << "' | Permission: '" << entry.permission
                  << "' | Status: " << entry.status << std::endl;
    }

    std::vector<omnisphere::models::PermissionModule> Authorization::GetPermissionsCatalog() const
    {
        std::vector<omnisphere::models::PermissionModule> catalog;
        if (!m_dbPool) return catalog;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string modSql = "SELECT \"Code\", \"Name\", \"Icon\" FROM \"Modules\" WHERE \"IsActive\" = true ORDER BY \"SortOrder\" ASC";
            auto modDt = conn->FetchResults(modSql);

            std::string permSql = "SELECT \"Code\", \"Name\", \"Description\", \"ModuleCode\" FROM \"Permissions\" WHERE \"IsActive\" = true ORDER BY \"Entry\" ASC";
            auto permDt = conn->FetchResults(permSql);

            for (size_t m = 0; m < modDt.RowsCount(); ++m)
            {
                omnisphere::models::PermissionModule module;
                module.code = std::string(modDt[m]["Code"]);
                module.name = std::string(modDt[m]["Name"]);
                if (!modDt[m]["Icon"].IsNull()) module.icon = std::string(modDt[m]["Icon"]);

                for (size_t p = 0; p < permDt.RowsCount(); ++p)
                {
                    std::string pModCode = std::string(permDt[p]["ModuleCode"]);
                    if (pModCode == module.code)
                    {
                        omnisphere::models::PermissionItem item;
                        item.code = std::string(permDt[p]["Code"]);
                        item.name = std::string(permDt[p]["Name"]);
                        if (!permDt[p]["Description"].IsNull()) item.description = std::string(permDt[p]["Description"]);
                        item.moduleCode = pModCode;
                        module.permissions.push_back(item);
                    }
                }
                catalog.push_back(module);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[GetPermissionsCatalog SQL Error] " << ex.what() << std::endl;
        }

        return catalog;
    }

    std::vector<std::string> Authorization::GetUserPermissions(const std::string& userCode) const
    {
        std::vector<std::string> perms;
        if (!m_dbPool) return perms;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string userSql = "SELECT \"PermissionCode\" FROM \"UserPermissions\" WHERE \"UserCode\" = ? AND \"IsAllowed\" = true AND \"IsActive\" = true";
            auto userDt = conn->FetchPrepared(userSql, { omnisphere::types::MakeSQLParam(userCode) });
            if (userDt.RowsCount() > 0)
            {
                for (size_t i = 0; i < userDt.RowsCount(); ++i)
                {
                    perms.push_back(std::string(userDt[i]["PermissionCode"]));
                }
                return perms;
            }

            // Si no tiene permisos explícitos en UserPermissions, consultar su RoleCode
            std::string roleSql = "SELECT \"RoleCode\" FROM \"Users\" WHERE \"Code\" = ?";
            auto roleDt = conn->FetchPrepared(roleSql, { omnisphere::types::MakeSQLParam(userCode) });
            if (roleDt.RowsCount() > 0 && !roleDt[0]["RoleCode"].IsNull())
            {
                std::string roleCode = std::string(roleDt[0]["RoleCode"]);
                if (!roleCode.empty())
                {
                    return GetRolePermissions(roleCode);
                }
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[GetUserPermissions SQL Error] " << ex.what() << std::endl;
        }

        return perms;
    }

    std::vector<std::string> Authorization::GetRolePermissions(const std::string& roleCode) const
    {
        std::vector<std::string> perms;
        if (!m_dbPool) return perms;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "SELECT \"PermissionCode\" FROM \"RolePermissions\" WHERE \"RoleCode\" = ? AND \"IsAllowed\" = true AND \"IsActive\" = true";
            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(roleCode) });
            for (size_t i = 0; i < dt.RowsCount(); ++i)
            {
                perms.push_back(std::string(dt[i]["PermissionCode"]));
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[GetRolePermissions SQL Error] " << ex.what() << std::endl;
        }

        return perms;
    }

    std::vector<omnisphere::models::Role> Authorization::GetAllRoles() const
    {
        std::vector<omnisphere::models::Role> roles;
        if (!m_dbPool) return roles;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "SELECT \"Entry\", \"Code\", \"Name\", \"Description\", \"IsActive\" FROM \"Roles\" WHERE \"IsCanceled\" = false ORDER BY \"Entry\" ASC";
            auto dt = conn->FetchResults(sql);
            for (size_t i = 0; i < dt.RowsCount(); ++i)
            {
                omnisphere::models::Role r;
                r.entry = dt[i]["Entry"];
                r.code = std::string(dt[i]["Code"]);
                r.name = std::string(dt[i]["Name"]);
                if (!dt[i]["Description"].IsNull()) r.description = std::string(dt[i]["Description"]);
                r.isActive = dt[i]["IsActive"];
                roles.push_back(r);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[GetAllRoles SQL Error] " << ex.what() << std::endl;
        }

        return roles;
    }

    bool Authorization::SetUserPermissions(const omnisphere::dtos::SetUserPermissionsInput& input) const
    {
        if (!m_dbPool) return false;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string delSql = "DELETE FROM \"UserPermissions\" WHERE \"UserCode\" = ?";
            conn->RunPrepared(delSql, { omnisphere::types::MakeSQLParam(input.userCode) });

            for (const auto& perm : input.permissions)
            {
                std::string insSql = "INSERT INTO \"UserPermissions\" (\"UserCode\", \"PermissionCode\", \"ModuleCode\", \"IsAllowed\", \"GrantedByCode\") "
                                     "SELECT ?, ?, \"ModuleCode\", true, ? FROM \"Permissions\" WHERE \"Code\" = ? "
                                     "ON CONFLICT (\"UserCode\", \"PermissionCode\") DO UPDATE SET \"IsAllowed\" = true, \"IsActive\" = true";
                conn->RunPrepared(insSql, {
                    omnisphere::types::MakeSQLParam(input.userCode),
                    omnisphere::types::MakeSQLParam(perm),
                    omnisphere::types::MakeSQLParam(input.grantedByCode),
                    omnisphere::types::MakeSQLParam(perm)
                });
            }
            return true;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SetUserPermissions SQL Error] " << ex.what() << std::endl;
        }

        return false;
    }

    bool Authorization::SetRolePermissions(const omnisphere::dtos::SetRolePermissionsInput& input) const
    {
        if (!m_dbPool) return false;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string delSql = "DELETE FROM \"RolePermissions\" WHERE \"RoleCode\" = ?";
            conn->RunPrepared(delSql, { omnisphere::types::MakeSQLParam(input.roleCode) });

            for (const auto& perm : input.permissions)
            {
                std::string insSql = "INSERT INTO \"RolePermissions\" (\"RoleCode\", \"PermissionCode\", \"ModuleCode\", \"IsAllowed\") "
                                     "SELECT ?, ?, \"ModuleCode\", true FROM \"Permissions\" WHERE \"Code\" = ? "
                                     "ON CONFLICT (\"RoleCode\", \"PermissionCode\") DO UPDATE SET \"IsAllowed\" = true, \"IsActive\" = true";
                conn->RunPrepared(insSql, {
                    omnisphere::types::MakeSQLParam(input.roleCode),
                    omnisphere::types::MakeSQLParam(perm),
                    omnisphere::types::MakeSQLParam(perm)
                });
            }
            return true;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SetRolePermissions SQL Error] " << ex.what() << std::endl;
        }

        return false;
    }

    bool Authorization::CreateRole(const omnisphere::models::Role& role) const
    {
        if (!m_dbPool) return false;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "INSERT INTO \"Roles\" (\"Code\", \"Name\", \"Description\", \"IsActive\") VALUES (?, ?, ?, ?) "
                              "ON CONFLICT (\"Code\") DO UPDATE SET \"Name\" = EXCLUDED.\"Name\", \"Description\" = EXCLUDED.\"Description\", \"IsCanceled\" = false, \"IsActive\" = true";
            return conn->RunPrepared(sql, {
                omnisphere::types::MakeSQLParam(role.code),
                omnisphere::types::MakeSQLParam(role.name),
                omnisphere::types::MakeSQLParam(role.description),
                omnisphere::types::MakeSQLParam(role.isActive)
            });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[CreateRole SQL Error] " << ex.what() << std::endl;
        }

        return false;
    }

    bool Authorization::UpdateRole(const omnisphere::models::Role& role) const
    {
        if (!m_dbPool) return false;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "UPDATE \"Roles\" SET \"Name\" = ?, \"Description\" = ?, \"IsActive\" = ?, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"Code\" = ?";
            return conn->RunPrepared(sql, {
                omnisphere::types::MakeSQLParam(role.name),
                omnisphere::types::MakeSQLParam(role.description),
                omnisphere::types::MakeSQLParam(role.isActive),
                omnisphere::types::MakeSQLParam(role.code)
            });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[UpdateRole SQL Error] " << ex.what() << std::endl;
        }

        return false;
    }

    bool Authorization::DeleteRole(const std::string& roleCode) const
    {
        if (!m_dbPool) return false;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "UPDATE \"Roles\" SET \"IsCanceled\" = true, \"IsActive\" = false, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"Code\" = ?";
            return conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(roleCode) });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[DeleteRole SQL Error] " << ex.what() << std::endl;
        }

        return false;
    }

    bool Authorization::GrantUserPermission(const omnisphere::dtos::GrantPermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::string sql = "INSERT INTO \"UserPermissions\" (\"UserCode\", \"ModuleCode\", \"PermissionCode\", \"IsAllowed\", \"GrantedByCode\") "
                          "VALUES (?, ?, ?, true, ?) "
                          "ON CONFLICT (\"UserCode\", \"PermissionCode\") DO UPDATE SET \"IsAllowed\" = true, \"IsActive\" = true";

        std::vector<omnisphere::types::SQLParam> params = {
            omnisphere::types::MakeSQLParam(input.userCode),
            omnisphere::types::MakeSQLParam(input.module),
            omnisphere::types::MakeSQLParam(input.permission),
            omnisphere::types::MakeSQLParam(input.grantedByCode)
        };
        return conn->RunPrepared(sql, params);
    }

    bool Authorization::RevokeUserPermission(const omnisphere::dtos::RevokePermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::string sql = "UPDATE \"UserPermissions\" SET \"IsAllowed\" = false WHERE \"UserCode\" = ? AND \"PermissionCode\" = ?";

        std::vector<omnisphere::types::SQLParam> params = {
            omnisphere::types::MakeSQLParam(input.userCode),
            omnisphere::types::MakeSQLParam(input.permission)
        };
        return conn->RunPrepared(sql, params);
    }

    bool Authorization::GrantRolePermission(const omnisphere::dtos::GrantRolePermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::string sql = "INSERT INTO \"RolePermissions\" (\"RoleCode\", \"ModuleCode\", \"PermissionCode\", \"IsAllowed\") "
                          "VALUES (?, ?, ?, true) "
                          "ON CONFLICT (\"RoleCode\", \"PermissionCode\") DO UPDATE SET \"IsAllowed\" = true, \"IsActive\" = true";

        std::vector<omnisphere::types::SQLParam> params = {
            omnisphere::types::MakeSQLParam(input.roleCode),
            omnisphere::types::MakeSQLParam(input.module),
            omnisphere::types::MakeSQLParam(input.permission)
        };
        return conn->RunPrepared(sql, params);
    }

    bool Authorization::RevokeRolePermission(const omnisphere::dtos::RevokeRolePermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::string sql = "UPDATE \"RolePermissions\" SET \"IsAllowed\" = false WHERE \"RoleCode\" = ? AND \"PermissionCode\" = ?";

        std::vector<omnisphere::types::SQLParam> params = {
            omnisphere::types::MakeSQLParam(input.roleCode),
            omnisphere::types::MakeSQLParam(input.permission)
        };
        return conn->RunPrepared(sql, params);
    }
} // namespace omnisphere::repositories

