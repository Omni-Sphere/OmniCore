#pragma once
#include <string>
#include <OmniData/SQLParams.hpp>

namespace omnisphere::enums
{
    enum class ModuleType
    {
        MOD_OVERVIEW,
        MOD_DASHBOARD,
        MOD_RESERVATIONS,
        MOD_SCHEDULES,
        MOD_EVENTS,
        MOD_ROUTES,
        MOD_VENUES,
        MOD_POINTS,
        MOD_PAYMENTS,
        MOD_INTEGRATIONS,
        MOD_SETTINGS,
        MOD_FAQS,
        MOD_SUGGESTIONS,
        MOD_MEMBERSHIPS,
        MOD_USERS,
        MOD_ROLES,
        MOD_EMPLOYEES,
        MOD_CUSTOMERS,
        // Legacy / Fallback
        MOD_CATALOGS,
        UNKNOWN
    };

    inline std::string ModuleTypeToString(ModuleType mod)
    {
        switch (mod)
        {
            case ModuleType::MOD_OVERVIEW: return "MOD_OVERVIEW";
            case ModuleType::MOD_DASHBOARD: return "MOD_DASHBOARD";
            case ModuleType::MOD_RESERVATIONS: return "MOD_RESERVATIONS";
            case ModuleType::MOD_SCHEDULES: return "MOD_SCHEDULES";
            case ModuleType::MOD_EVENTS: return "MOD_EVENTS";
            case ModuleType::MOD_ROUTES: return "MOD_ROUTES";
            case ModuleType::MOD_VENUES: return "MOD_VENUES";
            case ModuleType::MOD_POINTS: return "MOD_POINTS";
            case ModuleType::MOD_PAYMENTS: return "MOD_PAYMENTS";
            case ModuleType::MOD_INTEGRATIONS: return "MOD_INTEGRATIONS";
            case ModuleType::MOD_SETTINGS: return "MOD_SETTINGS";
            case ModuleType::MOD_FAQS: return "MOD_FAQS";
            case ModuleType::MOD_SUGGESTIONS: return "MOD_SUGGESTIONS";
            case ModuleType::MOD_MEMBERSHIPS: return "MOD_MEMBERSHIPS";
            case ModuleType::MOD_USERS: return "MOD_USERS";
            case ModuleType::MOD_ROLES: return "MOD_ROLES";
            case ModuleType::MOD_EMPLOYEES: return "MOD_EMPLOYEES";
            case ModuleType::MOD_CUSTOMERS: return "MOD_CUSTOMERS";
            case ModuleType::MOD_CATALOGS: return "MOD_CATALOGS";
            default: return "UNKNOWN";
        }
    }

    inline ModuleType StringToModuleType(const std::string& str)
    {
        if (str == "MOD_OVERVIEW") return ModuleType::MOD_OVERVIEW;
        if (str == "MOD_DASHBOARD") return ModuleType::MOD_DASHBOARD;
        if (str == "MOD_RESERVATIONS") return ModuleType::MOD_RESERVATIONS;
        if (str == "MOD_SCHEDULES") return ModuleType::MOD_SCHEDULES;
        if (str == "MOD_EVENTS") return ModuleType::MOD_EVENTS;
        if (str == "MOD_ROUTES") return ModuleType::MOD_ROUTES;
        if (str == "MOD_VENUES") return ModuleType::MOD_VENUES;
        if (str == "MOD_POINTS") return ModuleType::MOD_POINTS;
        if (str == "MOD_PAYMENTS") return ModuleType::MOD_PAYMENTS;
        if (str == "MOD_INTEGRATIONS") return ModuleType::MOD_INTEGRATIONS;
        if (str == "MOD_SETTINGS") return ModuleType::MOD_SETTINGS;
        if (str == "MOD_FAQS" || str == "MOD_FAQ") return ModuleType::MOD_FAQS;
        if (str == "MOD_SUGGESTIONS" || str == "MOD_FEEDBACK") return ModuleType::MOD_SUGGESTIONS;
        if (str == "MOD_MEMBERSHIPS") return ModuleType::MOD_MEMBERSHIPS;
        if (str == "MOD_USERS") return ModuleType::MOD_USERS;
        if (str == "MOD_ROLES") return ModuleType::MOD_ROLES;
        if (str == "MOD_EMPLOYEES") return ModuleType::MOD_EMPLOYEES;
        if (str == "MOD_CUSTOMERS") return ModuleType::MOD_CUSTOMERS;
        if (str == "MOD_CATALOGS") return ModuleType::MOD_CATALOGS;
        return ModuleType::UNKNOWN;
    }
}

namespace omnisphere::types
{
    inline SQLParam MakeSQLParam(omnisphere::enums::ModuleType val)
    {
        return SQLParam{omnisphere::enums::ModuleTypeToString(val)};
    }
}
