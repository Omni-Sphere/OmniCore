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
            std::string userQuery = "SELECT RoleCode FROM Users WHERE Code = ?";
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
                std::string roleQuery = "SELECT COUNT(1) AS Allowed FROM RolePermissions WHERE RoleCode = ? AND Permission = ? AND State = 'ENABLE'";
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
                std::string userPermQuery = "SELECT COUNT(1) AS Allowed FROM UserPermissions WHERE UserCode = ? AND Permission = ? AND State = 'ENABLE'";
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
                std::string sql = "INSERT INTO AuthorizationAuditLog (UserCode, GrantedByCode, Module, Permission, ResourceCode, Status, Reason) "
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

    bool Authorization::GrantUserPermission(const omnisphere::dtos::GrantPermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::string sql = "INSERT INTO UserPermissions (UserCode, Module, Permission, State, GrantedByCode) "
                          "VALUES (?, ?, ?, 'ENABLE', ?) "
                          "ON DUPLICATE KEY UPDATE State = 'ENABLE', GrantedByCode = VALUES(GrantedByCode)";

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
        std::string sql = "UPDATE UserPermissions SET State = 'DISABLE' WHERE UserCode = ? AND Module = ? AND Permission = ?";

        std::vector<omnisphere::types::SQLParam> params = {
            omnisphere::types::MakeSQLParam(input.userCode),
            omnisphere::types::MakeSQLParam(input.module),
            omnisphere::types::MakeSQLParam(input.permission)
        };
        return conn->RunPrepared(sql, params);
    }

    bool Authorization::GrantRolePermission(const omnisphere::dtos::GrantRolePermissionInput& input) const
    {
        if (!m_dbPool) return true;

        auto conn = m_dbPool->Acquire();
        std::string sql = "INSERT INTO RolePermissions (RoleCode, Module, Permission, State) "
                          "VALUES (?, ?, ?, 'ENABLE') "
                          "ON DUPLICATE KEY UPDATE State = 'ENABLE'";

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
        std::string sql = "UPDATE RolePermissions SET State = 'DISABLE' WHERE RoleCode = ? AND Module = ? AND Permission = ?";

        std::vector<omnisphere::types::SQLParam> params = {
            omnisphere::types::MakeSQLParam(input.roleCode),
            omnisphere::types::MakeSQLParam(input.module),
            omnisphere::types::MakeSQLParam(input.permission)
        };
        return conn->RunPrepared(sql, params);
    }
} // namespace omnisphere::repositories
