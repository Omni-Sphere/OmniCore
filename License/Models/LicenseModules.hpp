#pragma once
#include <string>

// =============================================================================
// LicenseModules.hpp
// Códigos de módulo para el sistema de licenciamiento OmniSphere.
// Cada constante representa una capacidad del sistema que puede habilitarse
// o deshabilitarse mediante una OmniLicense API Key firmada.
// =============================================================================

namespace omnisphere::license
{
    // -------------------------------------------------------------------------
    // Módulos de Notificación
    // -------------------------------------------------------------------------

    /// WhatsApp Meta Cloud API — Envío de mensajes, plantillas y webhooks
    constexpr const char* MODULE_WHATSAPP       = "MODULE_WHATSAPP";

    /// Motor de Notificaciones Personalizadas (mensajes internos del sistema)
    constexpr const char* MODULE_CUSTOM_NOTIF   = "MODULE_CUSTOM_NOTIF";

    // -------------------------------------------------------------------------
    // Módulos de Pasarela de Pago
    // -------------------------------------------------------------------------

    /// Stripe Multi-Currency Checkout — Sesiones, PaymentIntents, SPEI/CLABE
    constexpr const char* MODULE_STRIPE         = "MODULE_STRIPE";

    /// OpenPay — Pasarela de pago alternativa
    constexpr const char* MODULE_OPENPAY        = "MODULE_OPENPAY";

    /// MercadoPago — Pasarela de pago alternativa
    constexpr const char* MODULE_MERCADOPAGO    = "MODULE_MERCADOPAGO";

    // -------------------------------------------------------------------------
    // Módulos de Analítica y Reportes
    // -------------------------------------------------------------------------

    /// Dashboard de Analítica Avanzada — Métricas, reportes e insights
    constexpr const char* MODULE_ANALYTICS      = "MODULE_ANALYTICS";

    // -------------------------------------------------------------------------
    // Helper: lista de todos los módulos disponibles en la plataforma
    // -------------------------------------------------------------------------
    inline std::vector<std::string> AllModules()
    {
        return {
            MODULE_WHATSAPP,
            MODULE_CUSTOM_NOTIF,
            MODULE_STRIPE,
            MODULE_OPENPAY,
            MODULE_MERCADOPAGO,
            MODULE_ANALYTICS
        };
    }

} // namespace omnisphere::license
