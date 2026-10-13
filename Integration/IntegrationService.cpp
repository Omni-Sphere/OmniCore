#include "Integration/IntegrationService.hpp"
#include "License/Services/LicenseService.hpp"
#include "License/Models/LicenseModules.hpp"
#include <OmniData/Database.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>

namespace omnisphere::services
{
    IntegrationService::IntegrationService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool))
    {
    }

    std::vector<omnisphere::models::IntegrationInfo> IntegrationService::GetIntegrations(
        const omnisphere::models::SecurityContext& /*ctx*/,
        const std::string& category
    ) const
    {
        std::vector<omnisphere::models::IntegrationInfo> list;
        if (!m_dbPool) return list;

        auto getCanonicalLicenseModule = [](const std::string& integrationCode) -> std::string {
            std::string code = integrationCode;
            std::transform(code.begin(), code.end(), code.begin(), ::toupper);
            if (code == "STRIPE") return omnisphere::license::MODULE_STRIPE;
            if (code == "MERCADOPAGO") return omnisphere::license::MODULE_MERCADOPAGO;
            if (code == "OPENPAY") return omnisphere::license::MODULE_OPENPAY;
            if (code == "WHATSAPP") return omnisphere::license::MODULE_WHATSAPP;
            return "";
        };

        auto checkLicense = [&](const std::string& integrationCode, const std::string& dbModule) -> bool {
            // 1. Determinar el módulo canónico estricto desde C++ para evitar bypass si alguien altera la base de datos
            std::string moduleCode = getCanonicalLicenseModule(integrationCode);
            if (moduleCode.empty() && !dbModule.empty() && dbModule != "true" && dbModule != "false" && dbModule != "1" && dbModule != "0")
            {
                moduleCode = dbModule;
            }

            // Si la integración no requiere ningún módulo de licencia, está permitida
            if (moduleCode.empty())
            {
                return true;
            }

            // 2. Fail-closed: si no hay LicenseService o la licencia no está activa, acceso DENEGADO
            auto licService = omnisphere::services::LicenseService::GetSharedInstance();
            if (!licService || !licService->IsLicenseActive())
            {
                return false;
            }

            // 3. Validación criptográfica en RAM con el HMAC token firmado
            return licService->IsModuleLicensed(moduleCode);
        };

        try
        {
            auto conn = m_dbPool->Acquire();

            // Consulta consolidada única con LEFT JOIN a las tablas de configuración.
            // Elimina por completo el problema N+1 de ejecutar una consulta secundaria por cada fila.
            std::string query =
                "SELECT "
                "    i.\"Code\", "
                "    i.\"Name\", "
                "    i.\"DisplayName\", "
                "    i.\"Category\", "
                "    i.\"SettingsTable\", "
                "    i.\"LicenseModule\", "
                "    i.\"RequiredColumns\", "
                "    COALESCE(s.\"IsActive\", mp.\"IsActive\", op.\"IsActive\", wa.\"IsActive\", false) AS \"SettingsActive\", "
                "    COALESCE(s.\"IsTestMode\", mp.\"IsTestMode\", op.\"IsTestMode\", false) AS \"SettingsTestMode\", "
                "    CASE "
                "        WHEN UPPER(i.\"Code\") = 'STRIPE' THEN (s.\"PublishableKey\" IS NOT NULL AND TRIM(s.\"PublishableKey\") != '' AND s.\"SecretKey\" IS NOT NULL AND TRIM(s.\"SecretKey\") != '') "
                "        WHEN UPPER(i.\"Code\") = 'MERCADOPAGO' THEN (mp.\"PublicKey\" IS NOT NULL AND TRIM(mp.\"PublicKey\") != '' AND mp.\"AccessToken\" IS NOT NULL AND TRIM(mp.\"AccessToken\") != '') "
                "        WHEN UPPER(i.\"Code\") = 'OPENPAY' THEN (op.\"MerchantId\" IS NOT NULL AND TRIM(op.\"MerchantId\") != '' AND op.\"PublicKey\" IS NOT NULL AND TRIM(op.\"PublicKey\") != '' AND op.\"PrivateKey\" IS NOT NULL AND TRIM(op.\"PrivateKey\") != '') "
                "        WHEN UPPER(i.\"Code\") = 'WHATSAPP' THEN (wa.\"PhoneId\" IS NOT NULL AND TRIM(wa.\"PhoneId\") != '' AND wa.\"ApiToken\" IS NOT NULL AND TRIM(wa.\"ApiToken\") != '') "
                "        ELSE true "
                "    END AS \"IsConfigured\", "
                "    CASE "
                "        WHEN UPPER(i.\"Code\") = 'STRIPE' THEN (s.\"HasRow\" = 1) "
                "        WHEN UPPER(i.\"Code\") = 'MERCADOPAGO' THEN (mp.\"HasRow\" = 1) "
                "        WHEN UPPER(i.\"Code\") = 'OPENPAY' THEN (op.\"HasRow\" = 1) "
                "        WHEN UPPER(i.\"Code\") = 'WHATSAPP' THEN (wa.\"HasRow\" = 1) "
                "        ELSE false "
                "    END AS \"HasSettings\", "
                "    CASE "
                "        WHEN UPPER(i.\"Code\") = 'WHATSAPP' THEN false "
                "        ELSE true "
                "    END AS \"HasTestModeSupport\" "
                "FROM \"Integrations\" i "
                "LEFT JOIN (SELECT \"IsActive\", \"IsTestMode\", \"PublishableKey\", \"SecretKey\", 1 AS \"HasRow\" FROM \"StripeSettings\" LIMIT 1) s ON UPPER(i.\"Code\") = 'STRIPE' "
                "LEFT JOIN (SELECT \"IsActive\", \"IsTestMode\", \"PublicKey\", \"AccessToken\", 1 AS \"HasRow\" FROM \"MercadoPagoSettings\" LIMIT 1) mp ON UPPER(i.\"Code\") = 'MERCADOPAGO' "
                "LEFT JOIN (SELECT \"IsActive\", \"IsTestMode\", \"MerchantId\", \"PublicKey\", \"PrivateKey\", 1 AS \"HasRow\" FROM \"OpenPaySettings\" LIMIT 1) op ON UPPER(i.\"Code\") = 'OPENPAY' "
                "LEFT JOIN (SELECT \"IsActive\", \"PhoneId\", \"ApiToken\", 1 AS \"HasRow\" FROM \"WhatsAppSettings\" LIMIT 1) wa ON UPPER(i.\"Code\") = 'WHATSAPP' "
                "WHERE i.\"IsActive\" = true "
                "ORDER BY i.\"Entry\" ASC";

            auto dt = conn->FetchResults(query);

            for (size_t i = 0; i < dt.RowsCount(); ++i)
            {
                const auto& row = dt[i];
                omnisphere::models::IntegrationInfo item;
                item.code = row["Code"].GetOptional<std::string>().value_or("");
                item.name = row["Name"].GetOptional<std::string>().value_or("");
                item.displayName = row["DisplayName"].GetOptional<std::string>().value_or("");
                item.category = row["Category"].GetOptional<std::string>().value_or("PAYMENT");
                std::string settingsTable = row["SettingsTable"].GetOptional<std::string>().value_or("");
                std::string dbLicenseModule = row["LicenseModule"].GetOptional<std::string>().value_or("");
                std::string requiredCols = row["RequiredColumns"].GetOptional<std::string>().value_or("");

                bool isLicensed = checkLicense(item.code, dbLicenseModule);

                if (!settingsTable.empty())
                {
                    bool hasSettings = row["HasSettings"].GetOptional<bool>().value_or(false);
                    if (hasSettings)
                    {
                        bool dbActive = row["SettingsActive"].GetOptional<bool>().value_or(false);
                        bool hasTestMode = row["HasTestModeSupport"].GetOptional<bool>().value_or(false);
                        item.isTestMode = hasTestMode ? row["SettingsTestMode"].GetOptional<bool>().value_or(false) : false;
                        item.isConfigured = row["IsConfigured"].GetOptional<bool>().value_or(false);
                        item.isActive = dbActive && isLicensed;

                        if (!isLicensed)
                        {
                            item.message = "Módulo no incluido en la licencia activa del sistema";
                        }
                        else if (!item.isConfigured)
                        {
                            item.message = "Faltan credenciales requeridas (" + requiredCols + ")";
                        }
                        else if (!item.isActive)
                        {
                            item.message = "Deshabilitado en Ajustes de " + item.name;
                        }
                        else
                        {
                            if (hasTestMode)
                            {
                                item.message = item.isTestMode ? "Operativo (Modo Sandbox / Pruebas)" : "Operativo (Producción)";
                            }
                            else
                            {
                                item.message = "Operativo";
                            }
                        }
                    }
                    else
                    {
                        item.isActive = false;
                        item.isConfigured = false;
                        item.message = "Sin configuración guardada";
                    }
                }
                else
                {
                    item.isConfigured = true;
                    item.isActive = isLicensed;
                    item.message = isLicensed ? "Operativo" : "Módulo no incluido en la licencia activa del sistema";
                }

                list.push_back(std::move(item));
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[IntegrationService::GetIntegrations Exception] " << ex.what() << std::endl;
        }

        if (category.empty())
        {
            return list;
        }

        std::string filterCat = category;
        std::transform(filterCat.begin(), filterCat.end(), filterCat.begin(), ::toupper);
        if (filterCat == "ALL")
        {
            return list;
        }

        std::vector<omnisphere::models::IntegrationInfo> filtered;
        for (auto& item : list)
        {
            if (item.category == filterCat)
            {
                filtered.push_back(std::move(item));
            }
        }
        return filtered;
    }

    std::optional<omnisphere::models::IntegrationInfo> IntegrationService::GetIntegrationByCode(
        const omnisphere::models::SecurityContext& ctx,
        const std::string& code
    ) const
    {
        std::string targetCode = code;
        std::transform(targetCode.begin(), targetCode.end(), targetCode.begin(), ::toupper);

        auto all = GetIntegrations(ctx);
        for (const auto& item : all)
        {
            std::string itemCode = item.code;
            std::transform(itemCode.begin(), itemCode.end(), itemCode.begin(), ::toupper);
            if (itemCode == targetCode)
            {
                return item;
            }
        }
        return std::nullopt;
    }
} // namespace omnisphere::services
