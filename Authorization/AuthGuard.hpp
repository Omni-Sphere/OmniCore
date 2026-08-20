#pragma once

#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Authorization.hpp"
#include <memory>
#include <vector>
#include <string>

namespace omnisphere::core
{
    inline void PerformAuthorization(
        const std::shared_ptr<omnisphere::services::Authorization>& authService,
        const omnisphere::models::SecurityContext& ctx,
        const std::string& module,
        const std::string& permission,
        const std::string& resourceCode = "")
    {
        if (!authService) return;

        try
        {
            authService->Authorize(ctx, permission);
            authService->LogAudit(ctx, module, permission, resourceCode, true, "AUTHORIZED");
        }
        catch (const omnisphere::services::AccessDeniedException& ex)
        {
            authService->LogAudit(ctx, module, permission, resourceCode, false, ex.what());
            throw;
        }
    }

    inline void PerformRolesAuthorization(
        const std::shared_ptr<omnisphere::services::Authorization>& authService,
        const omnisphere::models::SecurityContext& ctx,
        const std::string& module,
        const std::vector<std::string>& roles)
    {
        if (!authService) return;

        try
        {
            authService->AuthorizeRoles(ctx, roles);
            authService->LogAudit(ctx, module, "ROLE_CHECK", "", true, "AUTHORIZED");
        }
        catch (const omnisphere::services::AccessDeniedException& ex)
        {
            authService->LogAudit(ctx, module, "ROLE_CHECK", "", false, ex.what());
            throw;
        }
    }
} // namespace omnisphere::core

#define AUTHORIZE(ctx, module, permission) \
    ::omnisphere::core::PerformAuthorization(m_authService, ctx, module, permission)

#define AUTHORIZE_ROLES(ctx, module, ...) \
    ::omnisphere::core::PerformRolesAuthorization(m_authService, ctx, module, {__VA_ARGS__})
