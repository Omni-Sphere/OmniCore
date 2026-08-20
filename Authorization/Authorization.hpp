#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Models/AuditLog.hpp"
#include "Authorization/Models/AuthorizationResult.hpp"
#include "Authorization/DTOs/GrantPermission.hpp"
#include "Authorization/DTOs/RevokePermission.hpp"
#include "Authorization/DTOs/GrantRolePermission.hpp"
#include "Authorization/DTOs/RevokeRolePermission.hpp"
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
        void Authorize(const omnisphere::models::SecurityContext& ctx, const std::string& requiredPermission) const;
        void AuthorizeRoles(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& allowedRoles) const;
        bool HasPermission(const omnisphere::models::SecurityContext& ctx, const std::string& permission) const;

        void LogAudit(const omnisphere::models::SecurityContext& ctx, const std::string& module, const std::string& permission, const std::string& resourceCode, bool isGranted, const std::string& reason = "") const;

        omnisphere::models::AuthorizationResult GrantUserPermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantPermissionInput& input) const;
        omnisphere::models::AuthorizationResult RevokeUserPermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokePermissionInput& input) const;

        omnisphere::models::AuthorizationResult GrantRolePermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::GrantRolePermissionInput& input) const;
        omnisphere::models::AuthorizationResult RevokeRolePermission(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::RevokeRolePermissionInput& input) const;
    };
} // namespace omnisphere::services
