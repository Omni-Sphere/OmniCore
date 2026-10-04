#pragma once

#include <OmniUtils/Http/Request.hpp>
#include <OmniUtils/Http/Response.hpp>
#include <boost/json.hpp>
#include <string>

namespace omnisphere::security
{
    /**
     * @brief Plantilla y Middleware Universal de Seguridad de API para el ecosistema OmniSphere.
     * Centraliza las políticas OWASP, inyección de cabeceras seguras y el manejo de cookies
     * HttpOnly para cualquier API (OmniRouteAPI, OmniCafeAPI, OmniERP, etc.).
     */
    class ApiSecurity
    {
    public:
        static constexpr const char* SESSION_COOKIE_NAME = "authToken";
        static constexpr int SESSION_MAX_AGE_SECONDS = 86400; // 24 horas

        /**
         * @brief Aplica las cabeceras estándar de Hardening OWASP a cualquier respuesta HTTP.
         */
        static void ApplySecurityHeaders(::omnisphere::net::Response& resp)
        {
            resp.Header("X-Content-Type-Options", "nosniff");
            resp.Header("X-Frame-Options", "DENY");
            resp.Header("Referrer-Policy", "strict-origin-when-cross-origin");
            resp.Header("X-XSS-Protection", "1; mode=block");
        }

        /**
         * @brief Adjunta la cookie segura HttpOnly de sesión a la respuesta HTTP.
         */
        static void AttachSessionCookie(const ::omnisphere::net::Request& req, ::omnisphere::net::Response& resp, const std::string& token)
        {
            bool isSecure = (req.Header("X-Forwarded-Proto") == "https" || req.Header("x-forwarded-proto") == "https");
            resp.SetCookie(SESSION_COOKIE_NAME, token, SESSION_MAX_AGE_SECONDS, "/", true, "Lax", isSecure);
        }

        /**
         * @brief Limpia / revoca la cookie de sesión en el navegador.
         */
        static void ClearSessionCookie(::omnisphere::net::Response& resp)
        {
            resp.ClearCookie(SESSION_COOKIE_NAME, "/");
        }

        /**
         * @brief Inspecciona el payload devuelto por GraphQL y gestiona automáticamente
         * el ciclo de vida de la cookie HttpOnly de sesión (Login / Logout).
         */
        static void ProcessGraphQLAuth(const ::omnisphere::net::Request& req, ::omnisphere::net::Response& resp, const boost::json::value& result)
        {
            ApplySecurityHeaders(resp);

            if (!result.is_object()) return;
            const auto& root = result.as_object();
            if (!root.contains("data") || !root.at("data").is_object()) return;

            const auto& data = root.at("data").as_object();
            if (!data.contains("Session") || !data.at("Session").is_object()) return;

            const auto& session = data.at("Session").as_object();

            // 1. Detectar Login exitoso y emitir Set-Cookie HttpOnly
            if (session.contains("Login") && session.at("Login").is_object())
            {
                const auto& loginObj = session.at("Login").as_object();
                if (loginObj.contains("AccessToken") && loginObj.at("AccessToken").is_string())
                {
                    std::string token = std::string(loginObj.at("AccessToken").as_string());
                    if (!token.empty())
                    {
                        AttachSessionCookie(req, resp, token);
                    }
                }
            }
            // 2. Detectar Logout y revocar Set-Cookie (Max-Age=0)
            else if (session.contains("Logout"))
            {
                ClearSessionCookie(resp);
            }
        }
    };
} // namespace omnisphere::security
