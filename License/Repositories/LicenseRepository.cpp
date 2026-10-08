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

    static omnisphere::models::SystemLicense MapRow(const omnisphere::types::DataTable::Row& row)
    {
        omnisphere::models::SystemLicense lic;
        lic.code       = row["Code"].GetOptional<std::string>().value_or("");
        lic.apiKey     = row["ApiKey"].GetOptional<std::string>().value_or("");
        lic.clientName = row["ClientName"].GetOptional<std::string>().value_or("");
        lic.issuer     = row["Issuer"].GetOptional<std::string>().value_or("");
        lic.issuedAt   = row["IssuedAt"].GetOptional<std::string>().value_or("");
        lic.expiresAt  = row["ExpiresAt"].GetOptional<std::string>().value_or("");
        lic.isActive   = row["IsActive"].GetOptional<bool>().value_or(false);

        // Parsear modules JSON "[\"MODULE_WHATSAPP\",\"MODULE_STRIPE\"]"
        std::string modulesJson = row["Modules"].GetOptional<std::string>().value_or("");
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

            std::string licenseCode = license.code.empty() ? "ACTIVE_LICENSE" : license.code;

            // 1. Verificar si ya existe algún registro en la tabla
            std::string checkSql = "SELECT \"Entry\" FROM \"SystemLicenses\" ORDER BY \"Entry\" ASC";
            std::vector<omnisphere::types::SQLParam> emptyParams;
            auto dt = conn->FetchPrepared(checkSql, emptyParams);

            if (!dt.IsEmpty())
            {
                // Obtenemos el Entry del único registro a mantener
                const auto& row = dt[0];
                int targetEntry = row["Entry"].GetOptional<int>().value_or(1);

                // Si por alguna razón histórica existía más de un registro, eliminamos los duplicados
                // ANTES del UPDATE para evitar violaciones del índice único UQ_SystemLicenses_Active.
                if (dt.RowsCount() > 1)
                {
                    std::vector<omnisphere::types::SQLParam> delParams = {
                        omnisphere::types::MakeSQLParam(targetEntry)
                    };
                    conn->RunPrepared("DELETE FROM \"SystemLicenses\" WHERE \"Entry\" != ?", delParams);
                }

                // Si ya existe la licencia: hacer UPDATE sobre el único registro existente
                std::string updateSql =
                    "UPDATE \"SystemLicenses\" SET "
                    "\"Code\" = ?, "
                    "\"ApiKey\" = ?, "
                    "\"ClientName\" = ?, "
                    "\"Issuer\" = ?, "
                    "\"IssuedAt\" = ?::date, "
                    "\"ExpiresAt\" = ?::date, "
                    "\"Modules\" = ?, "
                    "\"IsActive\" = true, "
                    "\"UpdateDate\" = NOW() "
                    "WHERE \"Entry\" = ?";

                std::vector<omnisphere::types::SQLParam> updateParams = {
                    omnisphere::types::MakeSQLParam(licenseCode),
                    omnisphere::types::MakeSQLParam(license.apiKey),
                    omnisphere::types::MakeSQLParam(license.clientName),
                    omnisphere::types::MakeSQLParam(license.issuer),
                    omnisphere::types::MakeSQLParam(license.issuedAt),
                    omnisphere::types::MakeSQLParam(license.expiresAt),
                    omnisphere::types::MakeSQLParam(modulesJson),
                    omnisphere::types::MakeSQLParam(targetEntry)
                };

                return conn->RunPrepared(updateSql, updateParams);
            }
            else
            {
                // Primera vez que se registra una licencia (tabla vacía): INSERT del único registro
                std::string insertSql =
                    "INSERT INTO \"SystemLicenses\" "
                    "(\"Code\", \"ApiKey\", \"ClientName\", \"Issuer\", \"IssuedAt\", \"ExpiresAt\", \"Modules\", \"IsActive\", \"CreatedBy\", \"CreateDate\") "
                    "VALUES (?, ?, ?, ?, ?::date, ?::date, ?, true, 1, NOW())";

                std::vector<omnisphere::types::SQLParam> insertParams = {
                    omnisphere::types::MakeSQLParam(licenseCode),
                    omnisphere::types::MakeSQLParam(license.apiKey),
                    omnisphere::types::MakeSQLParam(license.clientName),
                    omnisphere::types::MakeSQLParam(license.issuer),
                    omnisphere::types::MakeSQLParam(license.issuedAt),
                    omnisphere::types::MakeSQLParam(license.expiresAt),
                    omnisphere::types::MakeSQLParam(modulesJson)
                };

                return conn->RunPrepared(insertSql, insertParams);
            }
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
                "FROM \"SystemLicenses\" WHERE \"IsActive\" = true "
                "ORDER BY \"UpdateDate\" DESC NULLS LAST, \"CreateDate\" DESC, \"Entry\" DESC LIMIT 1";

            std::vector<omnisphere::types::SQLParam> emptyParams;
            auto dt = conn->FetchPrepared(sql, emptyParams);
            if (!dt.IsEmpty())
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
            std::string sql = code.empty()
                ? "UPDATE \"SystemLicenses\" SET \"IsActive\" = false, \"UpdateDate\" = NOW()"
                : "UPDATE \"SystemLicenses\" SET \"IsActive\" = false, \"UpdateDate\" = NOW() WHERE \"Code\" = ?";
            std::vector<omnisphere::types::SQLParam> params;
            if (!code.empty()) params.push_back(omnisphere::types::MakeSQLParam(code));
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

            std::vector<omnisphere::types::SQLParam> emptyParams;
            auto dt = conn->FetchPrepared(sql, emptyParams);
            result.reserve(dt.RowsCount());
            for (const auto& row : dt)
                result.push_back(MapRow(row));

            return result;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[LicenseRepository::GetAll] " << ex.what() << std::endl;
            return result;
        }
    }

} // namespace omnisphere::repositories
