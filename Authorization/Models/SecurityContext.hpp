#pragma once

#include <string>
#include <boost/json.hpp>

namespace omnisphere::models
{
    struct SecurityContext
    {
        std::string userCode;        // Código único del usuario actuante (CurrentUser)
        std::string userRole;        // Rol principal (ej: "ADMIN", "OPERATOR")
        std::string grantedByCode;   // Código del supervisor/autorizador (si aplica a la transacción)
        std::string delegationToken; // Token de delegación opcional
        boost::json::object rawClaims;

        bool isAuthenticated() const
        {
            return !userCode.empty();
        }

        bool isSuperAdmin() const
        {
            return userRole == "ADMIN" || userRole == "SUPERADMIN";
        }
    };
} // namespace omnisphere::models
