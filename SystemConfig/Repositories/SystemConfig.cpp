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

    bool SystemConfig::Update(const omnisphere::dtos::UpdateSystemConfigInput& input, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();

            auto updateCols = omnisphere::types::ExtractUpdateColumns(input, mutationFields);
            bool hasLastUpdatedBy = false;
            bool hasUpdateDate = false;
            for (const auto& c : updateCols) {
                if (c.Column == "\"LastUpdatedBy\"") hasLastUpdatedBy = true;
                if (c.Column == "\"UpdateDate\"") hasUpdateDate = true;
            }
            if (!hasLastUpdatedBy) {
                updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "SYSTEM" : input.LastUpdatedBy)});
            }
            if (!hasUpdateDate) {
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                char buf[32];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::gmtime(&in_time_t));
                updateCols.push_back({"\"UpdateDate\"", omnisphere::types::MakeSQLParam(std::string(buf))});
            }

            int targetEntry = input.Entry;
            if (targetEntry <= 0)
            {
                auto dtActive = GetActiveConfig({"Entry"});
                if (dtActive.RowsCount() > 0 && dtActive[0].HasColumn("Entry"))
                {
                    targetEntry = (int)dtActive[0]["Entry"];
                }
                else
                {
                    targetEntry = 1;
                }
            }

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
            if (dt.RowsCount() > 0 && (int)dt[0]["cnt"] > 0)
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
