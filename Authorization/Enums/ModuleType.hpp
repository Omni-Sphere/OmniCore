#pragma once
#include <string>
#include <OmniData/SQLParams.hpp>

namespace omnisphere::enums
{
    enum class ModuleType
    {
        MOD_OVERVIEW,
        MOD_RESERVATIONS,
        MOD_SCHEDULES,
        MOD_EVENTS,
        MOD_ROUTES,
        MOD_CATALOGS,
        MOD_EMPLOYEES,
        MOD_CUSTOMERS,
        MOD_USERS,
        MOD_ROLES,
        MOD_SETTINGS,
        // Legacy aliases
        MOD_VENUES,
        MOD_POINTS,
        MOD_PAYMENTS,
        MOD_INTEGRATIONS,
        UNKNOWN
    };

    inline std::string ModuleTypeToString(ModuleType mod)
    {
        switch (mod)
        {
            case ModuleType::MOD_OVERVIEW: return "MOD_OVERVIEW";
            case ModuleType::MOD_RESERVATIONS: return "MOD_RESERVATIONS";
            case ModuleType::MOD_SCHEDULES: return "MOD_SCHEDULES";
            case ModuleType::MOD_EVENTS: return "MOD_EVENTS";
            case ModuleType::MOD_ROUTES: return "MOD_ROUTES";
            case ModuleType::MOD_CATALOGS: return "MOD_CATALOGS";
            case ModuleType::MOD_EMPLOYEES: return "MOD_EMPLOYEES";
            case ModuleType::MOD_CUSTOMERS: return "MOD_CUSTOMERS";
            case ModuleType::MOD_USERS: return "MOD_USERS";
            case ModuleType::MOD_ROLES: return "MOD_ROLES";
            case ModuleType::MOD_SETTINGS: return "MOD_SETTINGS";
            // Legacy aliases map to official catalogs/settings
            case ModuleType::MOD_VENUES:
            case ModuleType::MOD_POINTS: return "MOD_CATALOGS";
            case ModuleType::MOD_PAYMENTS:
            case ModuleType::MOD_INTEGRATIONS: return "MOD_SETTINGS";
            default: return "UNKNOWN";
        }
    }

    inline ModuleType StringToModuleType(const std::string& str)
    {
        if (str == "MOD_OVERVIEW") return ModuleType::MOD_OVERVIEW;
        if (str == "MOD_RESERVATIONS") return ModuleType::MOD_RESERVATIONS;
        if (str == "MOD_SCHEDULES") return ModuleType::MOD_SCHEDULES;
        if (str == "MOD_EVENTS") return ModuleType::MOD_EVENTS;
        if (str == "MOD_ROUTES") return ModuleType::MOD_ROUTES;
        if (str == "MOD_CATALOGS") return ModuleType::MOD_CATALOGS;
        if (str == "MOD_EMPLOYEES") return ModuleType::MOD_EMPLOYEES;
        if (str == "MOD_CUSTOMERS") return ModuleType::MOD_CUSTOMERS;
        if (str == "MOD_USERS") return ModuleType::MOD_USERS;
        if (str == "MOD_ROLES") return ModuleType::MOD_ROLES;
        if (str == "MOD_SETTINGS") return ModuleType::MOD_SETTINGS;
        // Legacy mappings
        if (str == "MOD_VENUES" || str == "MOD_POINTS") return ModuleType::MOD_CATALOGS;
        if (str == "MOD_PAYMENTS" || str == "MOD_INTEGRATIONS") return ModuleType::MOD_SETTINGS;
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
