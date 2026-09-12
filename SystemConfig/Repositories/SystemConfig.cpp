#include "SystemConfig/Repositories/SystemConfig.hpp"
#include "SystemConfig/Models/SystemConfig.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <iostream>
#include <cmath>

namespace omnisphere::repositories
{
    SystemConfig::SystemConfig(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    omnisphere::types::DataTable SystemConfig::GetActiveConfig(const std::vector<std::string>& fields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::SystemConfig>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"IsActive\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"SystemConfigs\" WHERE " + qp.WhereClause + " ORDER BY \"Entry\" ASC LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(true)
            };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemConfigRepository::GetActiveConfig Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool SystemConfig::Update(const omnisphere::dtos::UpdateSystemConfigInput& input) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();

            std::vector<omnisphere::types::ColumnValue> updateCols;
            if (input.FeeHandlingStrategy.has_value()) updateCols.push_back({"\"FeeHandlingStrategy\"", omnisphere::types::MakeSQLParam(input.FeeHandlingStrategy.value())});
            if (input.TaxRatePercent.has_value()) updateCols.push_back({"\"TaxRatePercent\"", omnisphere::types::MakeSQLParam(input.TaxRatePercent.value())});
            if (input.DefaultCurrency.has_value()) updateCols.push_back({"\"DefaultCurrency\"", omnisphere::types::MakeSQLParam(input.DefaultCurrency.value())});
            if (input.CompanyName.has_value()) updateCols.push_back({"\"CompanyName\"", omnisphere::types::MakeSQLParam(input.CompanyName.value())});
            if (input.EnableEmailNotifications.has_value()) updateCols.push_back({"\"EnableEmailNotifications\"", omnisphere::types::MakeSQLParam(input.EnableEmailNotifications.value())});
            if (input.EnableWhatsappNotifications.has_value()) updateCols.push_back({"\"EnableWhatsappNotifications\"", omnisphere::types::MakeSQLParam(input.EnableWhatsappNotifications.value())});
            if (input.AllowPartialPayments.has_value()) updateCols.push_back({"\"AllowPartialPayments\"", omnisphere::types::MakeSQLParam(input.AllowPartialPayments.value())});
            if (input.IsActive.has_value()) updateCols.push_back({"\"IsActive\"", omnisphere::types::MakeSQLParam(input.IsActive.value())});

            if (updateCols.empty()) return true;

            updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(input.LastUpdatedBy)});

            int targetEntry = input.Entry > 0 ? input.Entry : 1;
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"SystemConfigs\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(targetEntry)
            );
            return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemConfigRepository::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool SystemConfig::EnsureDefaultExists() const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string checkSql = "SELECT COUNT(*) as cnt FROM \"SystemConfigs\"";
            std::vector<omnisphere::types::SQLParam> emptyParams;
            auto dt = conn->FetchPrepared(checkSql, emptyParams);
            if (dt.RowsCount() > 0 && static_cast<long long>(dt[0]["cnt"]) > 0)
            {
                return true;
            }

            std::string insertSql = "INSERT INTO \"SystemConfigs\" (\"FeeHandlingStrategy\", \"TaxRatePercent\", \"DefaultCurrency\", \"CompanyName\", \"EnableEmailNotifications\", \"EnableWhatsappNotifications\", \"AllowPartialPayments\", \"IsActive\") VALUES ('ABSORBED', 0.0, 'MXN', 'OmniRoute', true, true, false, true)";
            return conn->RunPrepared(insertSql, emptyParams);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemConfigRepository::EnsureDefaultExists Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    double SystemConfig::CalculateAuthorizedTotal(double baseAmount, int paymentMethodEntry) const
    {
        if (baseAmount <= 0) return 0.0;
        if (!m_dbPool) return baseAmount;

        std::string strategy = "ABSORBED";
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string configSql = "SELECT \"FeeHandlingStrategy\" FROM \"SystemConfigs\" WHERE \"IsActive\" = true LIMIT 1";
            std::vector<omnisphere::types::SQLParam> emptyParams;
            auto dtConfig = conn->FetchPrepared(configSql, emptyParams);
            if (dtConfig.RowsCount() > 0)
            {
                strategy = (std::string)dtConfig[0]["FeeHandlingStrategy"];
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemConfigRepository::CalculateAuthorizedTotal Config Exception] " << ex.what() << std::endl;
        }

        if (strategy == "ABSORBED" || strategy == "INCLUSIVE" || paymentMethodEntry <= 0)
        {
            return baseAmount;
        }

        try
        {
            auto conn = m_dbPool->Acquire();
            std::string pmSql = "SELECT \"UsesCommission\", \"CommissionRate\" FROM \"PaymentMethods\" WHERE \"Entry\" = ? AND \"IsActive\" = true";
            std::vector<omnisphere::types::SQLParam> pmParams = { omnisphere::types::MakeSQLParam(paymentMethodEntry) };
            auto dtPm = conn->FetchPrepared(pmSql, pmParams);
            if (dtPm.RowsCount() > 0)
            {
                bool usesComm = (bool)dtPm[0]["UsesCommission"];
                double commRate = (double)dtPm[0]["CommissionRate"];
                if (usesComm && commRate > 0)
                {
                    double commAmount = (baseAmount * commRate) / 100.0;
                    return std::round((baseAmount + commAmount) * 100.0) / 100.0;
                }
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemConfigRepository::CalculateAuthorizedTotal PaymentMethod Exception] " << ex.what() << std::endl;
        }

        return baseAmount;
    }
} // namespace omnisphere::repositories
