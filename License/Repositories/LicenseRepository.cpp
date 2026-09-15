#include "License/Repositories/LicenseRepository.hpp"
#include <OmniData/Database.hpp>
#include <iostream>
#include <sstream>

namespace omnisphere::repositories
{
    LicenseRepository::LicenseRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    // -------------------------------------------------------------------------
    // Helpers internos
    // -------------------------------------------------------------------------

    template <typename T, typename TRow>
    static T GetVal(const TRow& row, const std::string& col, T defaultVal = T{})
    {
        if (row.HasColumn(col)) {
            auto val = row[col];
            if (val.has_value()) {
                if (auto p = std::get_if<T>(&(*val))) return *p;
            }
        }
        return defaultVal;
    }

    static omnisphere::models::SystemLicense MapRow(const auto& row)
    {
        omnisphere::models::SystemLicense lic;
        lic.code       = GetVal<std::string>(row, "Code");
        lic.apiKey     = GetVal<std::string>(row, "ApiKey");
        lic.clientName = GetVal<std::string>(row, "ClientName");
        lic.issuer     = GetVal<std::string>(row, "Issuer");
        lic.issuedAt   = GetVal<std::string>(row, "IssuedAt");
        lic.expiresAt  = GetVal<std::string>(row, "ExpiresAt");
        lic.isActive   = GetVal<bool>(row, "IsActive");

        // Parsear modules JSON "[\"MODULE_WHATSAPP\",\"MODULE_STRIPE\"]"
        std::string modulesJson = GetVal<std::string>(row, "Modules");
        std::string token;
        std::istringstream ss(modulesJson);
        while (std::getline(ss, token, '"'))
        {
            if (!token.empty() && token.find("MODULE_") != std::string::npos)
                lic.modules.insert(token);
        }
        return lic;
    }

    // -------------------------------------------------------------------------
    // Save — Persiste (INSERT o UPDATE) la licencia activa
    // -------------------------------------------------------------------------
    bool LicenseRepository::Save(const omnisphere::models::SystemLicense& license) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();

            // Desactivar licencias previas
            std::string sqlDeactivate = "UPDATE \"SystemLicenses\" SET \"IsActive\" = false, \"UpdateDate\" = NOW() WHERE \"IsActive\" = true";
            conn->RunPrepared(sqlDeactivate, {});

            // Construir JSON de módulos
            std::string modulesJson = "[";
            bool first = true;
            for (const auto& mod : license.modules)
            {
                if (!first) modulesJson += ",";
                modulesJson += "\"" + mod + "\"";
                first = false;
            }
            modulesJson += "]";

            std::string sql =
                "INSERT INTO \"SystemLicenses\" "
                "(\"Code\", \"ApiKey\", \"ClientName\", \"Issuer\", \"IssuedAt\", \"ExpiresAt\", \"Modules\", \"IsActive\", \"CreatedBy\", \"CreateDate\") "
                "VALUES (?, ?, ?, ?, ?::date, ?::date, ?, true, 1, NOW()) "
                "ON CONFLICT (\"Code\") DO UPDATE SET "
                "\"ApiKey\" = EXCLUDED.\"ApiKey\", "
                "\"ClientName\" = EXCLUDED.\"ClientName\", "
                "\"Issuer\" = EXCLUDED.\"Issuer\", "
                "\"IssuedAt\" = EXCLUDED.\"IssuedAt\", "
                "\"ExpiresAt\" = EXCLUDED.\"ExpiresAt\", "
                "\"Modules\" = EXCLUDED.\"Modules\", "
                "\"IsActive\" = true, "
                "\"UpdateDate\" = NOW()";

            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(license.code),
                omnisphere::types::MakeSQLParam(license.apiKey),
                omnisphere::types::MakeSQLParam(license.clientName),
                omnisphere::types::MakeSQLParam(license.issuer),
                omnisphere::types::MakeSQLParam(license.issuedAt),
                omnisphere::types::MakeSQLParam(license.expiresAt),
                omnisphere::types::MakeSQLParam(modulesJson)
            };

            return conn->RunPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[LicenseRepository::Save] " << ex.what() << std::endl;
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // GetActive — Recupera la licencia activa actual
    // -------------------------------------------------------------------------
    std::optional<omnisphere::models::SystemLicense> LicenseRepository::GetActive() const
    {
        if (!m_dbPool) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql =
                "SELECT \"Code\", \"ApiKey\", \"ClientName\", \"Issuer\", "
                "TO_CHAR(\"IssuedAt\", 'YYYY-MM-DD') AS \"IssuedAt\", "
                "TO_CHAR(\"ExpiresAt\", 'YYYY-MM-DD') AS \"ExpiresAt\", "
                "\"Modules\", \"IsActive\" "
                "FROM \"SystemLicenses\" WHERE \"IsActive\" = true LIMIT 1";

            auto dt = conn->FetchPrepared(sql, {});
            if (dt.RowsCount() > 0)
                return MapRow(dt[0]);

            return std::nullopt;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[LicenseRepository::GetActive] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    // -------------------------------------------------------------------------
    // Deactivate — Revoca la licencia activa por código
    // -------------------------------------------------------------------------
    bool LicenseRepository::Deactivate(const std::string& code) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "UPDATE \"SystemLicenses\" SET \"IsActive\" = false, \"UpdateDate\" = NOW() WHERE \"Code\" = ?";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(code) };
            return conn->RunPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[LicenseRepository::Deactivate] " << ex.what() << std::endl;
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // GetAll — Historial de licencias registradas
    // -------------------------------------------------------------------------
    std::vector<omnisphere::models::SystemLicense> LicenseRepository::GetAll() const
    {
        std::vector<omnisphere::models::SystemLicense> result;
        if (!m_dbPool) return result;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql =
                "SELECT \"Code\", \"ApiKey\", \"ClientName\", \"Issuer\", "
                "TO_CHAR(\"IssuedAt\", 'YYYY-MM-DD') AS \"IssuedAt\", "
                "TO_CHAR(\"ExpiresAt\", 'YYYY-MM-DD') AS \"ExpiresAt\", "
                "\"Modules\", \"IsActive\" "
                "FROM \"SystemLicenses\" ORDER BY \"CreateDate\" DESC";

            auto dt = conn->FetchPrepared(sql, {});
            for (size_t i = 0; i < dt.RowsCount(); ++i)
                result.push_back(MapRow(dt[i]));

            return result;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[LicenseRepository::GetAll] " << ex.what() << std::endl;
            return result;
        }
    }

} // namespace omnisphere::repositories
