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

        auto checkLicense = [](const std::string& moduleCode) -> bool {
            if (moduleCode.empty()) return true;
            auto licService = omnisphere::services::LicenseService::GetSharedInstance();
            if (!licService || !licService->IsLicenseActive()) {
                return true;
            }
            return licService->IsModuleLicensed(moduleCode);
        };

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string query = "SELECT \"Code\", \"Name\", \"DisplayName\", \"Category\", \"SettingsTable\", \"LicenseModule\", \"RequiredColumns\" FROM \"Integrations\" WHERE \"IsActive\" = true ORDER BY \"Entry\" ASC";
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
                std::string licenseModule = row["LicenseModule"].GetOptional<std::string>().value_or("");
                std::string requiredCols = row["RequiredColumns"].GetOptional<std::string>().value_or("");

                bool isLicensed = checkLicense(licenseModule);

                if (!settingsTable.empty())
                {
                    try
                    {
                        std::string settingsSql = "SELECT * FROM \"" + settingsTable + "\" LIMIT 1";
                        auto settingsDt = conn->FetchResults(settingsSql);
                        if (!settingsDt.IsEmpty())
                        {
                            const auto& sRow = settingsDt[0];
                            bool dbActive = sRow.HasColumn("IsActive") ? sRow["IsActive"].GetOptional<bool>().value_or(false) : false;
                            item.isTestMode = sRow.HasColumn("IsTestMode") ? sRow["IsTestMode"].GetOptional<bool>().value_or(false) : false;

                            bool configured = true;
                            if (!requiredCols.empty())
                            {
                                std::stringstream ss(requiredCols);
                                std::string colName;
                                while (std::getline(ss, colName, ','))
                                {
                                    colName.erase(0, colName.find_first_not_of(" \t\r\n"));
                                    colName.erase(colName.find_last_not_of(" \t\r\n") + 1);

                                    if (!sRow.HasColumn(colName))
                                    {
                                        configured = false;
                                        break;
                                    }
                                    auto val = sRow[colName].GetOptional<std::string>().value_or("");
                                    if (val.empty())
                                    {
                                        configured = false;
                                        break;
                                    }
                                }
                            }

                            item.isConfigured = configured;
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
                                if (sRow.HasColumn("IsTestMode"))
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
                    catch (const std::exception&)
                    {
                        item.isActive = false;
                        item.isConfigured = false;
                        item.message = "Error al consultar configuración de " + item.name;
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
