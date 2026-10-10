#pragma once
#include <string>
#include <OmniData/SQLParams.hpp>

namespace omnisphere::enums
{
    enum class PermissionType
    {
        // ==========================================
        // 1. CORE DOMAIN (OmniCore - Plataforma Transversal)
        // ==========================================
        // Usuarios y Accesos
        CORE_USER_CREATE,
        CORE_USER_UPDATE,
        CORE_USER_DELETE,

        // Personal / Empleados
        CORE_EMPLOYEE_CREATE,
        CORE_EMPLOYEE_UPDATE,
        CORE_EMPLOYEE_DELETE,

        // Clientes
        CORE_CUSTOMER_CREATE,
        CORE_CUSTOMER_UPDATE,
        CORE_CUSTOMER_DELETE,

        // Roles y Permisos
        CORE_ROLE_CREATE,
        CORE_ROLE_UPDATE,
        CORE_ROLE_DELETE,
        CORE_ROLE_MANAGE,
        CORE_PERM_MANAGE,

        // Configuración General
        CORE_CONFIG_UPDATE,

        // Métodos de Pago y Cuentas Bancarias
        CORE_PAYMENT_CREATE,
        CORE_PAYMENT_UPDATE,
        CORE_PAYMENT_DELETE,
        CORE_PAYMENT_BANK_MANAGE,

        // Integraciones y Alertas (Stripe, WhatsApp Cloud, etc.)
        CORE_INTEGRATION_STRIPE,
        CORE_INTEGRATION_WHATSAPP,
        CORE_INTEGRATION_TEST,
        CORE_NOTIF_STAFF,

        // Membresías y Planes
        CORE_MEMBERSHIP_MANAGE,

        // ==========================================
        // 2. ROUTE DOMAIN (OmniRoute - Transporte & Eventos)
        // ==========================================
        // Overview
        ROUTE_OVERVIEW_EXPORT,

        // Reservaciones
        ROUTE_RESERVATION_CREATE,
        ROUTE_RESERVATION_UPDATE,
        ROUTE_RESERVATION_STATUS,
        ROUTE_RESERVATION_PAY,
        ROUTE_RESERVATION_CANCEL,
        ROUTE_RESERVATION_DELETE,

        // Horarios y Salidas
        ROUTE_SCHEDULE_CREATE,
        ROUTE_SCHEDULE_UPDATE,
        ROUTE_SCHEDULE_DELETE,

        // Eventos
        ROUTE_EVENT_CREATE,
        ROUTE_EVENT_UPDATE,
        ROUTE_EVENT_DELETE,

        // Rutas de Viaje
        ROUTE_ROUTE_CREATE,
        ROUTE_ROUTE_UPDATE,
        ROUTE_ROUTE_DELETE,

        // Catálogos Base (Recintos y Paradas)
        ROUTE_CATALOG_CREATE,
        ROUTE_CATALOG_UPDATE,
        ROUTE_CATALOG_DELETE,
        ROUTE_VENUE_CREATE,
        ROUTE_VENUE_UPDATE,
        ROUTE_VENUE_DELETE,
        ROUTE_POINT_CREATE,
        ROUTE_POINT_UPDATE,
        ROUTE_POINT_DELETE,

        // ==========================================
        // 3. ERP DOMAIN (OmniERP - Administración y Operaciones)
        // ==========================================
        ERP_INVENTORY_MANAGE,
        ERP_PURCHASE_MANAGE,
        ERP_INVOICE_MANAGE,

        // ==========================================
        // 4. POS DOMAIN (OmniPOS / OmniCafe - Punto de Venta)
        // ==========================================
        POS_ORDER_MANAGE,
        POS_CASH_MANAGE,
        POS_TICKET_MANAGE,

        // ==========================================
        // Compatibility Aliases
        // ==========================================
        ROUTE_USER_CREATE,
        ROUTE_USER_UPDATE,
        ROUTE_USER_DELETE,
        ROUTE_ROLE_CREATE_ALIAS,
        ROUTE_ROLE_UPDATE_ALIAS,
        ROUTE_ROLE_DELETE_ALIAS,
        ROUTE_EMPLOYEE_CREATE,
        ROUTE_EMPLOYEE_UPDATE,
        ROUTE_EMPLOYEE_DELETE,
        ROUTE_CUSTOMER_CREATE,
        ROUTE_CUSTOMER_UPDATE,
        ROUTE_CUSTOMER_DELETE,
        ROUTE_SETTING_UPDATE,
        ROUTE_SETTING_PAYMENT_UPDATE,
        ROUTE_SETTING_WHATSAPP_UPDATE,
        ROUTE_CONFIG_UPDATE,
        PAYMENTS_METHOD_CREATE,
        PAYMENTS_METHOD_UPDATE,
        PAYMENTS_BANK_MANAGE,
        PAYMENTS_METHOD_DELETE,
        INTEGRATION_STRIPE_MANAGE,
        INTEGRATION_WHATSAPP_MANAGE,
        INTEGRATION_TEST,
        NOTIF_STAFF_MANAGE,
        MEMBERSHIPS_MANAGE,

        // Sentinel
        UNKNOWN
    };

    inline std::string PermissionTypeToString(PermissionType perm)
    {
        switch (perm)
        {
            // CORE
            case PermissionType::CORE_USER_CREATE: return "CORE_USER_CREATE";
            case PermissionType::CORE_USER_UPDATE: return "CORE_USER_UPDATE";
            case PermissionType::CORE_USER_DELETE: return "CORE_USER_DELETE";
            case PermissionType::CORE_EMPLOYEE_CREATE: return "CORE_EMPLOYEE_CREATE";
            case PermissionType::CORE_EMPLOYEE_UPDATE: return "CORE_EMPLOYEE_UPDATE";
            case PermissionType::CORE_EMPLOYEE_DELETE: return "CORE_EMPLOYEE_DELETE";
            case PermissionType::CORE_CUSTOMER_CREATE: return "CORE_CUSTOMER_CREATE";
            case PermissionType::CORE_CUSTOMER_UPDATE: return "CORE_CUSTOMER_UPDATE";
            case PermissionType::CORE_CUSTOMER_DELETE: return "CORE_CUSTOMER_DELETE";
            case PermissionType::CORE_ROLE_CREATE: return "CORE_ROLE_CREATE";
            case PermissionType::CORE_ROLE_UPDATE: return "CORE_ROLE_UPDATE";
            case PermissionType::CORE_ROLE_DELETE: return "CORE_ROLE_DELETE";
            case PermissionType::CORE_ROLE_MANAGE: return "CORE_ROLE_MANAGE";
            case PermissionType::CORE_PERM_MANAGE: return "CORE_PERM_MANAGE";
            case PermissionType::CORE_CONFIG_UPDATE: return "CORE_CONFIG_UPDATE";
            case PermissionType::CORE_PAYMENT_CREATE: return "CORE_PAYMENT_CREATE";
            case PermissionType::CORE_PAYMENT_UPDATE: return "CORE_PAYMENT_UPDATE";
            case PermissionType::CORE_PAYMENT_DELETE: return "CORE_PAYMENT_DELETE";
            case PermissionType::CORE_PAYMENT_BANK_MANAGE: return "CORE_PAYMENT_BANK_MANAGE";
            case PermissionType::CORE_INTEGRATION_STRIPE: return "CORE_INTEGRATION_STRIPE";
            case PermissionType::CORE_INTEGRATION_WHATSAPP: return "CORE_INTEGRATION_WHATSAPP";
            case PermissionType::CORE_INTEGRATION_TEST: return "CORE_INTEGRATION_TEST";
            case PermissionType::CORE_NOTIF_STAFF: return "CORE_NOTIF_STAFF";
            case PermissionType::CORE_MEMBERSHIP_MANAGE: return "CORE_MEMBERSHIP_MANAGE";

            // ROUTE
            case PermissionType::ROUTE_OVERVIEW_EXPORT: return "ROUTE_OVERVIEW_EXPORT";
            case PermissionType::ROUTE_RESERVATION_CREATE: return "ROUTE_RESERVATION_CREATE";
            case PermissionType::ROUTE_RESERVATION_UPDATE: return "ROUTE_RESERVATION_UPDATE";
            case PermissionType::ROUTE_RESERVATION_STATUS: return "ROUTE_RESERVATION_STATUS";
            case PermissionType::ROUTE_RESERVATION_PAY: return "ROUTE_RESERVATION_PAY";
            case PermissionType::ROUTE_RESERVATION_CANCEL: return "ROUTE_RESERVATION_CANCEL";
            case PermissionType::ROUTE_RESERVATION_DELETE: return "ROUTE_RESERVATION_DELETE";
            case PermissionType::ROUTE_SCHEDULE_CREATE: return "ROUTE_SCHEDULE_CREATE";
            case PermissionType::ROUTE_SCHEDULE_UPDATE: return "ROUTE_SCHEDULE_UPDATE";
            case PermissionType::ROUTE_SCHEDULE_DELETE: return "ROUTE_SCHEDULE_DELETE";
            case PermissionType::ROUTE_EVENT_CREATE: return "ROUTE_EVENT_CREATE";
            case PermissionType::ROUTE_EVENT_UPDATE: return "ROUTE_EVENT_UPDATE";
            case PermissionType::ROUTE_EVENT_DELETE: return "ROUTE_EVENT_DELETE";
            case PermissionType::ROUTE_ROUTE_CREATE: return "ROUTE_ROUTE_CREATE";
            case PermissionType::ROUTE_ROUTE_UPDATE: return "ROUTE_ROUTE_UPDATE";
            case PermissionType::ROUTE_ROUTE_DELETE: return "ROUTE_ROUTE_DELETE";
            case PermissionType::ROUTE_CATALOG_CREATE: return "ROUTE_CATALOG_CREATE";
            case PermissionType::ROUTE_CATALOG_UPDATE: return "ROUTE_CATALOG_UPDATE";
            case PermissionType::ROUTE_CATALOG_DELETE: return "ROUTE_CATALOG_DELETE";
            case PermissionType::ROUTE_VENUE_CREATE: return "ROUTE_VENUE_CREATE";
            case PermissionType::ROUTE_VENUE_UPDATE: return "ROUTE_VENUE_UPDATE";
            case PermissionType::ROUTE_VENUE_DELETE: return "ROUTE_VENUE_DELETE";
            case PermissionType::ROUTE_POINT_CREATE: return "ROUTE_POINT_CREATE";
            case PermissionType::ROUTE_POINT_UPDATE: return "ROUTE_POINT_UPDATE";
            case PermissionType::ROUTE_POINT_DELETE: return "ROUTE_POINT_DELETE";

            // ERP
            case PermissionType::ERP_INVENTORY_MANAGE: return "ERP_INVENTORY_MANAGE";
            case PermissionType::ERP_PURCHASE_MANAGE: return "ERP_PURCHASE_MANAGE";
            case PermissionType::ERP_INVOICE_MANAGE: return "ERP_INVOICE_MANAGE";

            // POS
            case PermissionType::POS_ORDER_MANAGE: return "POS_ORDER_MANAGE";
            case PermissionType::POS_CASH_MANAGE: return "POS_CASH_MANAGE";
            case PermissionType::POS_TICKET_MANAGE: return "POS_TICKET_MANAGE";

            // Aliases map to official names
            case PermissionType::ROUTE_CONFIG_UPDATE:
            case PermissionType::ROUTE_SETTING_UPDATE: return "CORE_CONFIG_UPDATE";
            case PermissionType::PAYMENTS_METHOD_CREATE: return "CORE_PAYMENT_CREATE";
            case PermissionType::PAYMENTS_METHOD_UPDATE:
            case PermissionType::ROUTE_SETTING_PAYMENT_UPDATE: return "CORE_PAYMENT_UPDATE";
            case PermissionType::PAYMENTS_METHOD_DELETE: return "CORE_PAYMENT_DELETE";
            case PermissionType::PAYMENTS_BANK_MANAGE: return "CORE_PAYMENT_BANK_MANAGE";
            case PermissionType::INTEGRATION_STRIPE_MANAGE: return "CORE_INTEGRATION_STRIPE";
            case PermissionType::INTEGRATION_WHATSAPP_MANAGE:
            case PermissionType::ROUTE_SETTING_WHATSAPP_UPDATE: return "CORE_INTEGRATION_WHATSAPP";
            case PermissionType::INTEGRATION_TEST: return "CORE_INTEGRATION_TEST";
            case PermissionType::NOTIF_STAFF_MANAGE: return "CORE_NOTIF_STAFF";
            case PermissionType::MEMBERSHIPS_MANAGE: return "CORE_MEMBERSHIP_MANAGE";
            case PermissionType::ROUTE_USER_CREATE: return "CORE_USER_CREATE";
            case PermissionType::ROUTE_USER_UPDATE: return "CORE_USER_UPDATE";
            case PermissionType::ROUTE_USER_DELETE: return "CORE_USER_DELETE";
            case PermissionType::ROUTE_ROLE_CREATE_ALIAS: return "CORE_ROLE_CREATE";
            case PermissionType::ROUTE_ROLE_UPDATE_ALIAS: return "CORE_ROLE_UPDATE";
            case PermissionType::ROUTE_ROLE_DELETE_ALIAS: return "CORE_ROLE_DELETE";
            case PermissionType::ROUTE_EMPLOYEE_CREATE: return "CORE_EMPLOYEE_CREATE";
            case PermissionType::ROUTE_EMPLOYEE_UPDATE: return "CORE_EMPLOYEE_UPDATE";
            case PermissionType::ROUTE_EMPLOYEE_DELETE: return "CORE_EMPLOYEE_DELETE";
            case PermissionType::ROUTE_CUSTOMER_CREATE: return "CORE_CUSTOMER_CREATE";
            case PermissionType::ROUTE_CUSTOMER_UPDATE: return "CORE_CUSTOMER_UPDATE";
            case PermissionType::ROUTE_CUSTOMER_DELETE: return "CORE_CUSTOMER_DELETE";

            default: return "UNKNOWN";
        }
    }

    inline PermissionType StringToPermissionType(const std::string& str)
    {
        // CORE
        if (str == "CORE_USER_CREATE") return PermissionType::CORE_USER_CREATE;
        if (str == "CORE_USER_UPDATE") return PermissionType::CORE_USER_UPDATE;
        if (str == "CORE_USER_DELETE") return PermissionType::CORE_USER_DELETE;
        if (str == "CORE_EMPLOYEE_CREATE") return PermissionType::CORE_EMPLOYEE_CREATE;
        if (str == "CORE_EMPLOYEE_UPDATE") return PermissionType::CORE_EMPLOYEE_UPDATE;
        if (str == "CORE_EMPLOYEE_DELETE") return PermissionType::CORE_EMPLOYEE_DELETE;
        if (str == "CORE_CUSTOMER_CREATE") return PermissionType::CORE_CUSTOMER_CREATE;
        if (str == "CORE_CUSTOMER_UPDATE") return PermissionType::CORE_CUSTOMER_UPDATE;
        if (str == "CORE_CUSTOMER_DELETE") return PermissionType::CORE_CUSTOMER_DELETE;
        if (str == "CORE_ROLE_CREATE") return PermissionType::CORE_ROLE_CREATE;
        if (str == "CORE_ROLE_UPDATE") return PermissionType::CORE_ROLE_UPDATE;
        if (str == "CORE_ROLE_DELETE") return PermissionType::CORE_ROLE_DELETE;
        if (str == "CORE_ROLE_MANAGE") return PermissionType::CORE_ROLE_MANAGE;
        if (str == "CORE_PERM_MANAGE") return PermissionType::CORE_PERM_MANAGE;
        if (str == "CORE_CONFIG_UPDATE") return PermissionType::CORE_CONFIG_UPDATE;
        if (str == "CORE_PAYMENT_CREATE") return PermissionType::CORE_PAYMENT_CREATE;
        if (str == "CORE_PAYMENT_UPDATE") return PermissionType::CORE_PAYMENT_UPDATE;
        if (str == "CORE_PAYMENT_DELETE") return PermissionType::CORE_PAYMENT_DELETE;
        if (str == "CORE_PAYMENT_BANK_MANAGE") return PermissionType::CORE_PAYMENT_BANK_MANAGE;
        if (str == "CORE_INTEGRATION_STRIPE") return PermissionType::CORE_INTEGRATION_STRIPE;
        if (str == "CORE_INTEGRATION_WHATSAPP") return PermissionType::CORE_INTEGRATION_WHATSAPP;
        if (str == "CORE_INTEGRATION_TEST") return PermissionType::CORE_INTEGRATION_TEST;
        if (str == "CORE_NOTIF_STAFF") return PermissionType::CORE_NOTIF_STAFF;
        if (str == "CORE_MEMBERSHIP_MANAGE") return PermissionType::CORE_MEMBERSHIP_MANAGE;

        // ROUTE
        if (str == "ROUTE_OVERVIEW_EXPORT") return PermissionType::ROUTE_OVERVIEW_EXPORT;
        if (str == "ROUTE_RESERVATION_CREATE") return PermissionType::ROUTE_RESERVATION_CREATE;
        if (str == "ROUTE_RESERVATION_UPDATE") return PermissionType::ROUTE_RESERVATION_UPDATE;
        if (str == "ROUTE_RESERVATION_STATUS") return PermissionType::ROUTE_RESERVATION_STATUS;
        if (str == "ROUTE_RESERVATION_PAY") return PermissionType::ROUTE_RESERVATION_PAY;
        if (str == "ROUTE_RESERVATION_CANCEL") return PermissionType::ROUTE_RESERVATION_CANCEL;
        if (str == "ROUTE_RESERVATION_DELETE") return PermissionType::ROUTE_RESERVATION_DELETE;
        if (str == "ROUTE_SCHEDULE_CREATE") return PermissionType::ROUTE_SCHEDULE_CREATE;
        if (str == "ROUTE_SCHEDULE_UPDATE") return PermissionType::ROUTE_SCHEDULE_UPDATE;
        if (str == "ROUTE_SCHEDULE_DELETE") return PermissionType::ROUTE_SCHEDULE_DELETE;
        if (str == "ROUTE_EVENT_CREATE") return PermissionType::ROUTE_EVENT_CREATE;
        if (str == "ROUTE_EVENT_UPDATE") return PermissionType::ROUTE_EVENT_UPDATE;
        if (str == "ROUTE_EVENT_DELETE") return PermissionType::ROUTE_EVENT_DELETE;
        if (str == "ROUTE_ROUTE_CREATE") return PermissionType::ROUTE_ROUTE_CREATE;
        if (str == "ROUTE_ROUTE_UPDATE") return PermissionType::ROUTE_ROUTE_UPDATE;
        if (str == "ROUTE_ROUTE_DELETE") return PermissionType::ROUTE_ROUTE_DELETE;
        if (str == "ROUTE_CATALOG_CREATE") return PermissionType::ROUTE_CATALOG_CREATE;
        if (str == "ROUTE_CATALOG_UPDATE") return PermissionType::ROUTE_CATALOG_UPDATE;
        if (str == "ROUTE_CATALOG_DELETE") return PermissionType::ROUTE_CATALOG_DELETE;
        if (str == "ROUTE_VENUE_CREATE") return PermissionType::ROUTE_VENUE_CREATE;
        if (str == "ROUTE_VENUE_UPDATE") return PermissionType::ROUTE_VENUE_UPDATE;
        if (str == "ROUTE_VENUE_DELETE") return PermissionType::ROUTE_VENUE_DELETE;
        if (str == "ROUTE_POINT_CREATE") return PermissionType::ROUTE_POINT_CREATE;
        if (str == "ROUTE_POINT_UPDATE") return PermissionType::ROUTE_POINT_UPDATE;
        if (str == "ROUTE_POINT_DELETE") return PermissionType::ROUTE_POINT_DELETE;

        // ERP
        if (str == "ERP_INVENTORY_MANAGE") return PermissionType::ERP_INVENTORY_MANAGE;
        if (str == "ERP_PURCHASE_MANAGE") return PermissionType::ERP_PURCHASE_MANAGE;
        if (str == "ERP_INVOICE_MANAGE") return PermissionType::ERP_INVOICE_MANAGE;

        // POS
        if (str == "POS_ORDER_MANAGE") return PermissionType::POS_ORDER_MANAGE;
        if (str == "POS_CASH_MANAGE") return PermissionType::POS_CASH_MANAGE;
        if (str == "POS_TICKET_MANAGE") return PermissionType::POS_TICKET_MANAGE;

        // Legacy / Compatibility Aliases
        if (str == "ROUTE_CONFIG_UPDATE" || str == "ROUTE_SETTING_UPDATE") return PermissionType::CORE_CONFIG_UPDATE;
        if (str == "PAYMENTS_METHOD_CREATE") return PermissionType::CORE_PAYMENT_CREATE;
        if (str == "PAYMENTS_METHOD_UPDATE" || str == "ROUTE_SETTING_PAYMENT_UPDATE") return PermissionType::CORE_PAYMENT_UPDATE;
        if (str == "PAYMENTS_METHOD_DELETE") return PermissionType::CORE_PAYMENT_DELETE;
        if (str == "PAYMENTS_BANK_MANAGE") return PermissionType::CORE_PAYMENT_BANK_MANAGE;
        if (str == "INTEGRATION_STRIPE_MANAGE") return PermissionType::CORE_INTEGRATION_STRIPE;
        if (str == "INTEGRATION_WHATSAPP_MANAGE" || str == "ROUTE_SETTING_WHATSAPP_UPDATE") return PermissionType::CORE_INTEGRATION_WHATSAPP;
        if (str == "INTEGRATION_TEST") return PermissionType::CORE_INTEGRATION_TEST;
        if (str == "NOTIF_STAFF_MANAGE") return PermissionType::CORE_NOTIF_STAFF;
        if (str == "MEMBERSHIPS_MANAGE") return PermissionType::CORE_MEMBERSHIP_MANAGE;
        if (str == "ROUTE_USER_CREATE") return PermissionType::CORE_USER_CREATE;
        if (str == "ROUTE_USER_UPDATE") return PermissionType::CORE_USER_UPDATE;
        if (str == "ROUTE_USER_DELETE") return PermissionType::CORE_USER_DELETE;
        if (str == "ROUTE_ROLE_CREATE") return PermissionType::CORE_ROLE_CREATE;
        if (str == "ROUTE_ROLE_UPDATE") return PermissionType::CORE_ROLE_UPDATE;
        if (str == "ROUTE_ROLE_DELETE") return PermissionType::CORE_ROLE_DELETE;
        if (str == "ROUTE_EMPLOYEE_CREATE") return PermissionType::CORE_EMPLOYEE_CREATE;
        if (str == "ROUTE_EMPLOYEE_UPDATE") return PermissionType::CORE_EMPLOYEE_UPDATE;
        if (str == "ROUTE_EMPLOYEE_DELETE") return PermissionType::CORE_EMPLOYEE_DELETE;
        if (str == "ROUTE_CUSTOMER_CREATE") return PermissionType::CORE_CUSTOMER_CREATE;
        if (str == "ROUTE_CUSTOMER_UPDATE") return PermissionType::CORE_CUSTOMER_UPDATE;
        if (str == "ROUTE_CUSTOMER_DELETE") return PermissionType::CORE_CUSTOMER_DELETE;

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
