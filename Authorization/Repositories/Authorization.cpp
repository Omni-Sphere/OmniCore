#include "Authorization/Repositories/Authorization.hpp"
#include <iostream>
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <OmniData/DataMapper.hpp>

namespace omnisphere::repositories
{
    Authorization::Authorization(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    bool Authorization::CheckPermission(const std::string& userCode, const std::string& permission) const
    {
        if (!m_dbPool) return true;
        if (userCode.empty()) return false;
        if (userCode == "system") return true;

        try
        {
            auto conn = m_dbPool->Acquire();

            // 1. Validar directamente del usuario en la base de datos (SuperUser, RoleCode y PermissionMode)
            std::vector<omnisphere::types::Condition> userConditions = {
                {"", "\"Code\"", "=", "?"},
                {"", "\"IsActive\"", "=", "true"},
                {"", "\"IsCanceled\"", "=", "false"}
            };
            auto userQp = omnisphere::types::BuildQueryParts({"\"RoleCode\"", "\"SuperUser\"", "\"PermissionMode\""}, userConditions);
            std::string userQuery = "SELECT " + userQp.SelectClause + " FROM \"Users\" WHERE " + userQp.WhereClause;
            std::vector<omnisphere::types::SQLParam> userParams = {
                omnisphere::types::MakeSQLParam(userCode)
            };
            auto userDt = conn->FetchPrepared(userQuery, userParams);

            if (userDt.RowsCount() == 0)
            {
                return false;
            }

            // Si es SuperUser directamente en la tabla Users, tiene acceso total
            if (!userDt[0]["SuperUser"].IsNull())
            {
                bool isSuper = userDt[0]["SuperUser"];
                if (isSuper) return true;
            }

            std::string roleCode = "";
            if (!userDt[0]["RoleCode"].IsNull())
            {
                roleCode = std::string(userDt[0]["RoleCode"]);
            }

            // Si el rol es de administración total
            if (roleCode == "ADMIN" || roleCode == "SUPERADMIN")
            {
                return true;
            }

            std::string permMode = "P";
            if (!userDt[0]["PermissionMode"].IsNull())
            {
                permMode = std::string(userDt[0]["PermissionMode"]);
            }

            // MODO R (Role-based): Valida única y estrictamente contra RolePermissions
            if (permMode == "R")
            {
                if (roleCode.empty()) return false;

                std::vector<omnisphere::types::Condition> roleConditions = {
                    {"", "\"RoleCode\"", "=", "?"},
                    {"", "\"PermissionCode\"", "=", "?"},
                    {"", "\"IsActive\"", "=", "true"}
                };
                auto roleQp = omnisphere::types::BuildQueryParts({"COUNT(1) AS \"Allowed\""}, roleConditions);
                std::string roleQuery = "SELECT " + roleQp.SelectClause + " FROM \"RolePermissions\" WHERE " + roleQp.WhereClause;
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

            // MODO P (Permissions-based): Valida única y estrictamente contra UserPermissions activos
            std::vector<omnisphere::types::Condition> permConditions = {
                {"", "\"UserCode\"", "=", "?"},
                {"", "\"PermissionCode\"", "=", "?"},
                {"", "\"IsActive\"", "=", "true"}
            };
            auto permQp = omnisphere::types::BuildQueryParts({"COUNT(1) AS \"Allowed\""}, permConditions);
            std::string userPermQuery = "SELECT " + permQp.SelectClause + " FROM \"UserPermissions\" WHERE " + permQp.WhereClause;
            auto userPermDt = conn->FetchPrepared(userPermQuery, {
                omnisphere::types::MakeSQLParam(userCode),
                omnisphere::types::MakeSQLParam(permission)
            });
            if (userPermDt.RowsCount() > 0)
            {
                int count = userPermDt[0]["Allowed"];
                return count > 0;
            }
            return false;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[Authorization Repository SQL Error] " << ex.what() << std::endl;
        }

        return false;
    }

    bool Authorization::CheckRole(const std::string& userCode, const std::vector<std::string>& allowedRoles) const
    {
        if (!m_dbPool) return true;
        if (userCode == "system") return true;

        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Code\"", "=", "?"},
                {"", "\"IsActive\"", "=", "true"},
                {"", "\"IsCanceled\"", "=", "false"}
            };
            auto qp = omnisphere::types::BuildQueryParts({"\"RoleCode\"", "\"SuperUser\""}, conditions);
            std::string userQuery = "SELECT " + qp.SelectClause + " FROM \"Users\" WHERE " + qp.WhereClause;
            auto dt = conn->FetchPrepared(userQuery, { omnisphere::types::MakeSQLParam(userCode) });
            if (dt.RowsCount() > 0)
            {
                if (!dt[0]["SuperUser"].IsNull() && (bool)dt[0]["SuperUser"]) return true;
                if (!dt[0]["RoleCode"].IsNull())
                {
                    std::string role = std::string(dt[0]["RoleCode"]);
                    if (role == "ADMIN" || role == "SUPERADMIN") return true;
                    for (const auto& r : allowedRoles)
                    {
                        if (r == role) return true;
                    }
                }
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[CheckRole SQL Error] " << ex.what() << std::endl;
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
                std::vector<std::string> columns = {
                    "\"UserCode\"", "\"GrantedByCode\"", "\"Module\"", "\"Permission\"",
                    "\"ResourceCode\"", "\"Status\"", "\"Reason\""
                };
                std::string sql = omnisphere::types::BuildInsertQuery("\"AuthorizationAuditLog\"", columns);

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
            std::vector<omnisphere::types::Condition> modConditions = {
                {"", "\"IsActive\"", "=", "true"}
            };
            auto modQp = omnisphere::types::BuildQueryParts({"\"Code\"", "\"Name\"", "\"Icon\""}, modConditions);
            std::string modSql = "SELECT " + modQp.SelectClause + " FROM \"Modules\" WHERE " + modQp.WhereClause + " ORDER BY \"SortOrder\" ASC";
            auto modDt = conn->FetchResults(modSql);

            std::vector<omnisphere::types::Condition> permConditions = {
                {"", "\"IsActive\"", "=", "true"}
            };
            auto permQp = omnisphere::types::BuildQueryParts({"\"Code\"", "\"Name\"", "\"Description\"", "\"ModuleCode\""}, permConditions);
            std::string permSql = "SELECT " + permQp.SelectClause + " FROM \"Permissions\" WHERE " + permQp.WhereClause + " ORDER BY \"Entry\" ASC";
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

            // 1. Consultar directamente del usuario en Users (RoleCode, SuperUser y PermissionMode)
            std::vector<omnisphere::types::Condition> userConds = {
                {"", "\"Code\"", "=", "?"},
                {"", "\"IsActive\"", "=", "true"},
                {"", "\"IsCanceled\"", "=", "false"}
            };
            auto userQp = omnisphere::types::BuildQueryParts({"\"RoleCode\"", "\"SuperUser\"", "\"PermissionMode\""}, userConds);
            std::string userSql = "SELECT " + userQp.SelectClause + " FROM \"Users\" WHERE " + userQp.WhereClause;
            auto userDt = conn->FetchPrepared(userSql, { omnisphere::types::MakeSQLParam(userCode) });

            bool isSuper = false;
            std::string roleCode = "";
            std::string permMode = "P";
            if (userDt.RowsCount() > 0)
            {
                if (!userDt[0]["SuperUser"].IsNull() && (bool)userDt[0]["SuperUser"]) isSuper = true;
                if (!userDt[0]["RoleCode"].IsNull()) roleCode = std::string(userDt[0]["RoleCode"]);
                if (!userDt[0]["PermissionMode"].IsNull()) permMode = std::string(userDt[0]["PermissionMode"]);
            }

            // Si es SuperUser o tiene rol ADMIN/SUPERADMIN, devolver todos los permisos del catálogo
            if (isSuper || roleCode == "ADMIN" || roleCode == "SUPERADMIN" || userCode == "system")
            {
                std::vector<omnisphere::types::Condition> allConds = {
                    {"", "\"IsActive\"", "=", "true"}
                };
                auto allQp = omnisphere::types::BuildQueryParts({"\"Code\""}, allConds);
                std::string allSql = "SELECT " + allQp.SelectClause + " FROM \"Permissions\" WHERE " + allQp.WhereClause;
                auto allDt = conn->FetchResults(allSql);
                for (size_t i = 0; i < allDt.RowsCount(); ++i)
                {
                    perms.push_back(std::string(allDt[i]["Code"]));
                }
                return perms;
            }

            // Si el modo es R (Basado en Rol):
            if (permMode == "R")
            {
                if (!roleCode.empty())
                {
                    return GetRolePermissions(roleCode);
                }
                return perms;
            }

            // Si el modo es P (Permisos Personalizados):
            std::vector<omnisphere::types::Condition> permConds = {
                {"", "\"UserCode\"", "=", "?"},
                {"", "\"IsActive\"", "=", "true"}
            };
            auto permQp = omnisphere::types::BuildQueryParts({"\"PermissionCode\""}, permConds);
            std::string userPermSql = "SELECT " + permQp.SelectClause + " FROM \"UserPermissions\" WHERE " + permQp.WhereClause;
            auto userPermDt = conn->FetchPrepared(userPermSql, { omnisphere::types::MakeSQLParam(userCode) });
            if (userPermDt.RowsCount() > 0)
            {
                for (size_t i = 0; i < userPermDt.RowsCount(); ++i)
                {
                    perms.push_back(std::string(userPermDt[i]["PermissionCode"]));
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
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"RoleCode\"", "=", "?"},
                {"", "\"IsAllowed\"", "=", "true"},
                {"", "\"IsActive\"", "=", "true"}
            };
            auto qp = omnisphere::types::BuildQueryParts({"\"PermissionCode\""}, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"RolePermissions\" WHERE " + qp.WhereClause;
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

    std::vector<omnisphere::models::Role> Authorization::GetAllRoles(const std::vector<std::string>& fields) const
    {
        std::vector<omnisphere::models::Role> roles;
        if (!m_dbPool) return roles;

        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::Role>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"IsCanceled\"", "=", "false"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"Roles\" WHERE " + qp.WhereClause + " ORDER BY \"Entry\" ASC";
            auto dt = conn->FetchResults(sql);
            for (size_t i = 0; i < dt.RowsCount(); ++i)
            {
                roles.push_back(omnisphere::data::MapFromRow<omnisphere::models::Role>(dt[i]));
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
            // Desactivar todos los permisos actuales del usuario en lugar de eliminarlos físicamente
            std::string deactSql = "UPDATE \"UserPermissions\" SET \"IsActive\" = false, \"IsAllowed\" = false, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"UserCode\" = ?";
            conn->RunPrepared(deactSql, { omnisphere::types::MakeSQLParam(input.userCode) });

            // Al configurar permisos personalizados, el usuario pasa automáticamente a PermissionMode = 'P'
            std::string updateModeSql = "UPDATE \"Users\" SET \"PermissionMode\" = 'P', \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"Code\" = ?";
            conn->RunPrepared(updateModeSql, { omnisphere::types::MakeSQLParam(input.userCode) });

            for (const auto& perm : input.permissions)
            {
                std::string insSql = "INSERT INTO \"UserPermissions\" (\"UserCode\", \"PermissionCode\", \"ModuleCode\", \"IsAllowed\", \"GrantedByCode\", \"IsActive\", \"UpdateDate\") "
                                     "VALUES (?, ?, COALESCE((SELECT \"ModuleCode\" FROM \"Permissions\" WHERE \"Code\" = ? LIMIT 1), ''), true, ?, true, CURRENT_TIMESTAMP) "
                                     "ON CONFLICT (\"UserCode\", \"PermissionCode\") DO UPDATE SET \"IsAllowed\" = true, \"IsActive\" = true, \"GrantedByCode\" = EXCLUDED.\"GrantedByCode\", \"UpdateDate\" = CURRENT_TIMESTAMP";
                conn->RunPrepared(insSql, {
                    omnisphere::types::MakeSQLParam(input.userCode),
                    omnisphere::types::MakeSQLParam(perm),
                    omnisphere::types::MakeSQLParam(perm),
                    omnisphere::types::MakeSQLParam(input.grantedByCode)
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
            // Desactivar todos los permisos actuales del rol en lugar de eliminarlos físicamente
            std::string deactSql = "UPDATE \"RolePermissions\" SET \"IsActive\" = false, \"IsAllowed\" = false, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"RoleCode\" = ?";
            conn->RunPrepared(deactSql, { omnisphere::types::MakeSQLParam(input.roleCode) });

            for (const auto& perm : input.permissions)
            {
                std::string insSql = "INSERT INTO \"RolePermissions\" (\"RoleCode\", \"PermissionCode\", \"ModuleCode\", \"IsAllowed\", \"IsActive\", \"UpdateDate\") "
                                     "VALUES (?, ?, COALESCE((SELECT \"ModuleCode\" FROM \"Permissions\" WHERE \"Code\" = ? LIMIT 1), ''), true, true, CURRENT_TIMESTAMP) "
                                     "ON CONFLICT (\"RoleCode\", \"PermissionCode\") DO UPDATE SET \"IsAllowed\" = true, \"IsActive\" = true, \"UpdateDate\" = CURRENT_TIMESTAMP";
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
            std::vector<std::string> cols = {"\"Code\"", "\"Name\"", "\"Description\"", "\"IsActive\""};
            std::string baseInsert = omnisphere::types::BuildInsertQuery("\"Roles\"", cols);
            std::string sql = baseInsert + " ON CONFLICT (\"Code\") DO UPDATE SET \"Name\" = EXCLUDED.\"Name\", \"Description\" = EXCLUDED.\"Description\", \"IsCanceled\" = false, \"IsActive\" = true";
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
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"Name\"", omnisphere::types::MakeSQLParam(role.name)},
                {"\"Description\"", omnisphere::types::MakeSQLParam(role.description)},
                {"\"IsActive\"", omnisphere::types::MakeSQLParam(role.isActive)}
            };
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"Roles\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(role.code)
            );
            return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
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
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"IsCanceled\"", omnisphere::types::MakeSQLParam(true)},
                {"\"IsActive\"", omnisphere::types::MakeSQLParam(false)}
            };
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"Roles\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(roleCode)
            );
            return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
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
        std::vector<std::string> cols = {
            "\"UserCode\"", "\"ModuleCode\"", "\"PermissionCode\"", "\"IsAllowed\"", "\"GrantedByCode\""
        };
        std::string baseInsert = omnisphere::types::BuildInsertQuery("\"UserPermissions\"", cols);
        std::string sql = baseInsert + " ON CONFLICT (\"UserCode\", \"PermissionCode\") DO UPDATE SET \"IsAllowed\" = true, \"IsActive\" = true";

        std::vector<omnisphere::types::SQLParam> params = {
            omnisphere::types::MakeSQLParam(input.userCode),
            omnisphere::types::MakeSQLParam(input.module),
            omnisphere::types::MakeSQLParam(input.permission),
            omnisphere::types::MakeSQLParam(true),
            omnisphere::types::MakeSQLParam(input.grantedByCode)
        };
        return conn->RunPrepared(sql, params);
    }

    bool Authorization::RevokeUserPermission(const omnisphere::dtos::RevokePermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::string sql = "UPDATE \"UserPermissions\" SET \"IsActive\" = false, \"IsAllowed\" = false, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"UserCode\" = ? AND \"PermissionCode\" = ?";
        return conn->RunPrepared(sql, {
            omnisphere::types::MakeSQLParam(input.userCode),
            omnisphere::types::MakeSQLParam(input.permission)
        });
    }

    bool Authorization::GrantRolePermission(const omnisphere::dtos::GrantRolePermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::vector<std::string> cols = {
            "\"RoleCode\"", "\"ModuleCode\"", "\"PermissionCode\"", "\"IsAllowed\""
        };
        std::string baseInsert = omnisphere::types::BuildInsertQuery("\"RolePermissions\"", cols);
        std::string sql = baseInsert + " ON CONFLICT (\"RoleCode\", \"PermissionCode\") DO UPDATE SET \"IsAllowed\" = true, \"IsActive\" = true, \"UpdateDate\" = CURRENT_TIMESTAMP";

        std::vector<omnisphere::types::SQLParam> params = {
            omnisphere::types::MakeSQLParam(input.roleCode),
            omnisphere::types::MakeSQLParam(input.module),
            omnisphere::types::MakeSQLParam(input.permission),
            omnisphere::types::MakeSQLParam(true)
        };
        return conn->RunPrepared(sql, params);
    }

    bool Authorization::RevokeRolePermission(const omnisphere::dtos::RevokeRolePermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::string sql = "UPDATE \"RolePermissions\" SET \"IsActive\" = false, \"IsAllowed\" = false, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"RoleCode\" = ? AND \"PermissionCode\" = ?";
        return conn->RunPrepared(sql, {
            omnisphere::types::MakeSQLParam(input.roleCode),
            omnisphere::types::MakeSQLParam(input.permission)
        });
    }
} // namespace omnisphere::repositories


