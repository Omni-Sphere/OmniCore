#pragma once
#include <string>
#include <OmniData/SQLParams.hpp>

namespace omnisphere::enums
{
    enum class PermissionType
    {
        // Overview
        ROUTE_OVERVIEW_EXPORT,

        // Reservaciones y Taquilla
        ROUTE_RESERVATION_CREATE,
        ROUTE_RESERVATION_UPDATE,
        ROUTE_RESERVATION_STATUS,
        ROUTE_RESERVATION_PAY,
        ROUTE_RESERVATION_CANCEL,
        ROUTE_RESERVATION_DELETE,
        ROUTE_RESERVATION_READ,

        // Horarios y Salidas
        ROUTE_SCHEDULE_CREATE,
        ROUTE_SCHEDULE_UPDATE,
        ROUTE_SCHEDULE_DELETE,

        // Eventos
        ROUTE_EVENT_CREATE,
        ROUTE_EVENT_UPDATE,
        ROUTE_EVENT_DELETE,

        // Recintos / Venues
        ROUTE_VENUE_CREATE,
        ROUTE_VENUE_UPDATE,
        ROUTE_VENUE_DELETE,

        // Puntos de Abordaje / Destino
        ROUTE_POINT_CREATE,
        ROUTE_POINT_UPDATE,
        ROUTE_POINT_DELETE,

        // Rutas
        ROUTE_ROUTE_CREATE,
        ROUTE_ROUTE_UPDATE,
        ROUTE_ROUTE_DELETE,

        // Métodos de Pago y Bancos
        PAYMENTS_METHOD_CREATE,
        PAYMENTS_METHOD_UPDATE,
        PAYMENTS_BANK_MANAGE,
        PAYMENTS_METHOD_DELETE,

        // Integraciones
        INTEGRATION_STRIPE_MANAGE,
        INTEGRATION_WHATSAPP_MANAGE,
        INTEGRATION_TEST,
        NOTIF_STAFF_MANAGE,

        // Configuración General
        ROUTE_CONFIG_UPDATE,

        // Usuarios, Roles y Seguridad
        CORE_USER_CREATE,
        CORE_USER_UPDATE,
        CORE_USER_DELETE,
        CORE_ROLE_MANAGE,
        CORE_PERM_MANAGE,

        // Membresías
        MEMBERSHIPS_MANAGE,

        // Sentinel
        UNKNOWN
    };

    inline std::string PermissionTypeToString(PermissionType perm)
    {
        switch (perm)
        {
            case PermissionType::ROUTE_OVERVIEW_EXPORT: return "ROUTE_OVERVIEW_EXPORT";
            case PermissionType::ROUTE_RESERVATION_CREATE: return "ROUTE_RESERVATION_CREATE";
            case PermissionType::ROUTE_RESERVATION_UPDATE: return "ROUTE_RESERVATION_UPDATE";
            case PermissionType::ROUTE_RESERVATION_STATUS: return "ROUTE_RESERVATION_STATUS";
            case PermissionType::ROUTE_RESERVATION_PAY: return "ROUTE_RESERVATION_PAY";
            case PermissionType::ROUTE_RESERVATION_CANCEL: return "ROUTE_RESERVATION_CANCEL";
            case PermissionType::ROUTE_RESERVATION_DELETE: return "ROUTE_RESERVATION_DELETE";
            case PermissionType::ROUTE_RESERVATION_READ: return "ROUTE_RESERVATION_READ";
            case PermissionType::ROUTE_SCHEDULE_CREATE: return "ROUTE_SCHEDULE_CREATE";
            case PermissionType::ROUTE_SCHEDULE_UPDATE: return "ROUTE_SCHEDULE_UPDATE";
            case PermissionType::ROUTE_SCHEDULE_DELETE: return "ROUTE_SCHEDULE_DELETE";
            case PermissionType::ROUTE_EVENT_CREATE: return "ROUTE_EVENT_CREATE";
            case PermissionType::ROUTE_EVENT_UPDATE: return "ROUTE_EVENT_UPDATE";
            case PermissionType::ROUTE_EVENT_DELETE: return "ROUTE_EVENT_DELETE";
            case PermissionType::ROUTE_VENUE_CREATE: return "ROUTE_VENUE_CREATE";
            case PermissionType::ROUTE_VENUE_UPDATE: return "ROUTE_VENUE_UPDATE";
            case PermissionType::ROUTE_VENUE_DELETE: return "ROUTE_VENUE_DELETE";
            case PermissionType::ROUTE_POINT_CREATE: return "ROUTE_POINT_CREATE";
            case PermissionType::ROUTE_POINT_UPDATE: return "ROUTE_POINT_UPDATE";
            case PermissionType::ROUTE_POINT_DELETE: return "ROUTE_POINT_DELETE";
            case PermissionType::ROUTE_ROUTE_CREATE: return "ROUTE_ROUTE_CREATE";
            case PermissionType::ROUTE_ROUTE_UPDATE: return "ROUTE_ROUTE_UPDATE";
            case PermissionType::ROUTE_ROUTE_DELETE: return "ROUTE_ROUTE_DELETE";
            case PermissionType::PAYMENTS_METHOD_CREATE: return "PAYMENTS_METHOD_CREATE";
            case PermissionType::PAYMENTS_METHOD_UPDATE: return "PAYMENTS_METHOD_UPDATE";
            case PermissionType::PAYMENTS_BANK_MANAGE: return "PAYMENTS_BANK_MANAGE";
            case PermissionType::PAYMENTS_METHOD_DELETE: return "PAYMENTS_METHOD_DELETE";
            case PermissionType::INTEGRATION_STRIPE_MANAGE: return "INTEGRATION_STRIPE_MANAGE";
            case PermissionType::INTEGRATION_WHATSAPP_MANAGE: return "INTEGRATION_WHATSAPP_MANAGE";
            case PermissionType::INTEGRATION_TEST: return "INTEGRATION_TEST";
            case PermissionType::NOTIF_STAFF_MANAGE: return "NOTIF_STAFF_MANAGE";
            case PermissionType::ROUTE_CONFIG_UPDATE: return "ROUTE_CONFIG_UPDATE";
            case PermissionType::CORE_USER_CREATE: return "CORE_USER_CREATE";
            case PermissionType::CORE_USER_UPDATE: return "CORE_USER_UPDATE";
            case PermissionType::CORE_USER_DELETE: return "CORE_USER_DELETE";
            case PermissionType::CORE_ROLE_MANAGE: return "CORE_ROLE_MANAGE";
            case PermissionType::CORE_PERM_MANAGE: return "CORE_PERM_MANAGE";
            case PermissionType::MEMBERSHIPS_MANAGE: return "MEMBERSHIPS_MANAGE";
            default: return "UNKNOWN";
        }
    }

    inline PermissionType StringToPermissionType(const std::string& str)
    {
        if (str == "ROUTE_OVERVIEW_EXPORT") return PermissionType::ROUTE_OVERVIEW_EXPORT;
        if (str == "ROUTE_RESERVATION_CREATE") return PermissionType::ROUTE_RESERVATION_CREATE;
        if (str == "ROUTE_RESERVATION_UPDATE") return PermissionType::ROUTE_RESERVATION_UPDATE;
        if (str == "ROUTE_RESERVATION_STATUS") return PermissionType::ROUTE_RESERVATION_STATUS;
        if (str == "ROUTE_RESERVATION_PAY") return PermissionType::ROUTE_RESERVATION_PAY;
        if (str == "ROUTE_RESERVATION_CANCEL") return PermissionType::ROUTE_RESERVATION_CANCEL;
        if (str == "ROUTE_RESERVATION_DELETE") return PermissionType::ROUTE_RESERVATION_DELETE;
        if (str == "ROUTE_RESERVATION_READ") return PermissionType::ROUTE_RESERVATION_READ;
        if (str == "ROUTE_SCHEDULE_CREATE") return PermissionType::ROUTE_SCHEDULE_CREATE;
        if (str == "ROUTE_SCHEDULE_UPDATE") return PermissionType::ROUTE_SCHEDULE_UPDATE;
        if (str == "ROUTE_SCHEDULE_DELETE") return PermissionType::ROUTE_SCHEDULE_DELETE;
        if (str == "ROUTE_EVENT_CREATE") return PermissionType::ROUTE_EVENT_CREATE;
        if (str == "ROUTE_EVENT_UPDATE") return PermissionType::ROUTE_EVENT_UPDATE;
        if (str == "ROUTE_EVENT_DELETE") return PermissionType::ROUTE_EVENT_DELETE;
        if (str == "ROUTE_VENUE_CREATE") return PermissionType::ROUTE_VENUE_CREATE;
        if (str == "ROUTE_VENUE_UPDATE") return PermissionType::ROUTE_VENUE_UPDATE;
        if (str == "ROUTE_VENUE_DELETE") return PermissionType::ROUTE_VENUE_DELETE;
        if (str == "ROUTE_POINT_CREATE") return PermissionType::ROUTE_POINT_CREATE;
        if (str == "ROUTE_POINT_UPDATE") return PermissionType::ROUTE_POINT_UPDATE;
        if (str == "ROUTE_POINT_DELETE") return PermissionType::ROUTE_POINT_DELETE;
        if (str == "ROUTE_ROUTE_CREATE") return PermissionType::ROUTE_ROUTE_CREATE;
        if (str == "ROUTE_ROUTE_UPDATE") return PermissionType::ROUTE_ROUTE_UPDATE;
        if (str == "ROUTE_ROUTE_DELETE") return PermissionType::ROUTE_ROUTE_DELETE;
        if (str == "PAYMENTS_METHOD_CREATE") return PermissionType::PAYMENTS_METHOD_CREATE;
        if (str == "PAYMENTS_METHOD_UPDATE") return PermissionType::PAYMENTS_METHOD_UPDATE;
        if (str == "PAYMENTS_BANK_MANAGE") return PermissionType::PAYMENTS_BANK_MANAGE;
        if (str == "PAYMENTS_METHOD_DELETE") return PermissionType::PAYMENTS_METHOD_DELETE;
        if (str == "INTEGRATION_STRIPE_MANAGE") return PermissionType::INTEGRATION_STRIPE_MANAGE;
        if (str == "INTEGRATION_WHATSAPP_MANAGE") return PermissionType::INTEGRATION_WHATSAPP_MANAGE;
        if (str == "INTEGRATION_TEST") return PermissionType::INTEGRATION_TEST;
        if (str == "NOTIF_STAFF_MANAGE") return PermissionType::NOTIF_STAFF_MANAGE;
        if (str == "ROUTE_CONFIG_UPDATE") return PermissionType::ROUTE_CONFIG_UPDATE;
        if (str == "CORE_USER_CREATE") return PermissionType::CORE_USER_CREATE;
        if (str == "CORE_USER_UPDATE") return PermissionType::CORE_USER_UPDATE;
        if (str == "CORE_USER_DELETE") return PermissionType::CORE_USER_DELETE;
        if (str == "CORE_ROLE_MANAGE") return PermissionType::CORE_ROLE_MANAGE;
        if (str == "CORE_PERM_MANAGE") return PermissionType::CORE_PERM_MANAGE;
        if (str == "MEMBERSHIPS_MANAGE") return PermissionType::MEMBERSHIPS_MANAGE;
        return PermissionType::UNKNOWN;
    }
}

namespace omnisphere::types
{
    inline SQLParam MakeSQLParam(omnisphere::enums::PermissionType val)
    {
        return SQLParam{omnisphere::enums::PermissionTypeToString(val)};
    }
}
