#pragma once

#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Authorization.hpp"
#include "Authorization/Enums/ModuleType.hpp"
#include "Authorization/Enums/PermissionType.hpp"
#include <memory>
#include <vector>
#include <string>

namespace omnisphere::core
{
    // 1. Autorización de Acceso a Pantalla / Módulo
    inline void PerformModuleAuthorization(
        const std::shared_ptr<omnisphere::services::Authorization>& authService,
        const omnisphere::models::SecurityContext& ctx,
        omnisphere::enums::ModuleType module,
        const std::string& resourceCode = "")
    {
        if (!authService) return;

        std::string modStr = omnisphere::enums::ModuleTypeToString(module);
        try
        {
            authService->AuthorizeModule(ctx, module);
            authService->LogAudit(ctx, modStr, "MODULE_ACCESS", resourceCode, true, "AUTHORIZED");
        }
        catch (const omnisphere::services::AccessDeniedException& ex)
        {
            authService->LogAudit(ctx, modStr, "MODULE_ACCESS", resourceCode, false, ex.what());
            throw;
        }
    }

    // 2. Autorización de Operación Atómica
    inline void PerformActionAuthorization(
        const std::shared_ptr<omnisphere::services::Authorization>& authService,
        const omnisphere::models::SecurityContext& ctx,
        omnisphere::enums::PermissionType permission,
        const std::string& resourceCode = "")
    {
        if (!authService) return;

        std::string permStr = omnisphere::enums::PermissionTypeToString(permission);
        try
        {
            authService->AuthorizeAction(ctx, permission);
            authService->LogAudit(ctx, "ACTION", permStr, resourceCode, true, "AUTHORIZED");
        }
        catch (const omnisphere::services::AccessDeniedException& ex)
        {
            authService->LogAudit(ctx, "ACTION", permStr, resourceCode, false, ex.what());
            throw;
        }
    }

    // Overload de compatibilidad string-based
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

#define AUTHORIZE_MODULE(ctx, module) \
    ::omnisphere::core::PerformModuleAuthorization(m_authService, ctx, module)

#define AUTHORIZE_ACTION(ctx, action) \
    ::omnisphere::core::PerformActionAuthorization(m_authService, ctx, action)

#define AUTHORIZE(ctx, module, permission) \
    ::omnisphere::core::PerformAuthorization(m_authService, ctx, module, permission)

#define AUTHORIZE_ROLES(ctx, module, ...) \
    ::omnisphere::core::PerformRolesAuthorization(m_authService, ctx, module, {__VA_ARGS__})
