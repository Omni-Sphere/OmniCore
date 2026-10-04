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
        std::string clientIp;        // Dirección IP real del cliente conectado
        boost::json::object rawClaims;

        bool isAuthenticated() const
        {
            return !userCode.empty();
        }
    };
} // namespace omnisphere::models
