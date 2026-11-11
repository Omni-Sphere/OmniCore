#include "Company/Repositories/CompanyBrandingRepository.hpp"
#include "Company/Models/CompanyBranding.hpp"
#include <OmniData/DataTable.hpp>
#include <OmniData/Database.hpp>
#include <OmniData/SQLParams.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <iostream>
#include <chrono>

namespace omnisphere::repositories {

CompanyBrandingRepository::CompanyBrandingRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
    : m_dbPool(std::move(dbPool)) {}

CompanyBrandingRepository::CompanyBrandingRepository(std::shared_ptr<omnisphere::data::Database> database)
    : m_legacyDb(std::move(database)) {}

CompanyBrandingRepository::~CompanyBrandingRepository() = default;

bool CompanyBrandingRepository::EnsureDefaultExists(int companyEntry) const {
  try {
    std::string checkSql = "SELECT COUNT(*) as cnt FROM \"CompanyBranding\" WHERE \"CompanyEntry\" = ?";
    std::vector<omnisphere::types::SQLParam> params;
    params.push_back(omnisphere::types::MakeSQLParam(companyEntry));
    omnisphere::types::DataTable dt;

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      dt = conn->FetchPrepared(checkSql, params);
    } else if (m_legacyDb) {
      dt = m_legacyDb->FetchPrepared(checkSql, params);
    } else {
      return false;
    }

    if (!dt.IsEmpty()) {
      const auto& row = dt[0];
      if (row["cnt"].GetOptional<int>().value_or(0) > 0) {
        return true;
      }
    }

    // Inicializar heredando el LogoFile existente de la tabla Company si está presente
    std::string initialLogo = "";
    std::string companySql = "SELECT \"LogoFile\" FROM \"Company\" WHERE \"Entry\" = ? LIMIT 1";
    omnisphere::types::DataTable cDt;
    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      cDt = conn->FetchPrepared(companySql, params);
    } else if (m_legacyDb) {
      cDt = m_legacyDb->FetchPrepared(companySql, params);
    }
    if (!cDt.IsEmpty() && cDt[0]["LogoFile"].GetOptional<std::string>().has_value()) {
      initialLogo = cDt[0]["LogoFile"].GetOptional<std::string>().value();
    }

    std::string insertSql =
      "INSERT INTO \"CompanyBranding\" ("
      "\"CompanyEntry\", \"LogoFile\", \"Slogan\", \"SloganDescription\", "
      "\"PrimaryColor\", \"PrimaryContainerColor\", \"PrimaryDarkColor\", \"PrimaryLightColor\", "
      "\"SecondaryAmberColor\", \"SecondaryOrangeColor\", \"SecondaryNavyColor\", \"SecondaryCoralColor\", "
      "\"IsActive\", \"CreatedBy\") "
      "VALUES (?, ?, 'Viaja Seguro y Cómodo a tus Conciertos Favoritos', "
      "'Plataforma líder en transporte colectivo directo a festivales y conciertos de la CDMX. Traslados puntuales de ida y vuelta con puntos de salida estratégicos.', "
      "'#45a6df', '#2d77b4', '#20436a', '#95c0d8', "
      "'#F2A65A', '#E97845', '#20436a', '#D96C5F', true, 'system') "
      "ON CONFLICT (\"CompanyEntry\") DO NOTHING";

    std::vector<omnisphere::types::SQLParam> insertParams;
    insertParams.push_back(omnisphere::types::MakeSQLParam(companyEntry));
    insertParams.push_back(omnisphere::types::MakeSQLParam(initialLogo));

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->RunPrepared(insertSql, insertParams);
    } else if (m_legacyDb) {
      return m_legacyDb->RunPrepared(insertSql, insertParams);
    }
    return false;
  } catch (const std::exception& e) {
    std::cerr << "[CompanyBrandingRepository::EnsureDefaultExists Exception] " << e.what() << std::endl;
    return false;
  }
}

omnisphere::types::DataTable
CompanyBrandingRepository::Read(const omnisphere::dtos::GetCompanyBranding &dto, const std::vector<std::string>& fields) const {
  try {
    EnsureDefaultExists(dto.CompanyEntry);

    auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::CompanyBranding>(fields);
    std::vector<omnisphere::types::Condition> conditions;
    std::vector<omnisphere::types::SQLParam> params;

    conditions.push_back({"", "\"CompanyEntry\"", "=", "?"});
    params.push_back(omnisphere::types::MakeSQLParam(dto.CompanyEntry));

    auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
    std::string sQuery = "SELECT " + qp.SelectClause + " FROM \"CompanyBranding\"";
    if (!qp.WhereClause.empty()) {
      sQuery += " WHERE " + qp.WhereClause;
    }
    sQuery += " LIMIT 1";

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->FetchPrepared(sQuery, params);
    } else if (m_legacyDb) {
      return m_legacyDb->FetchPrepared(sQuery, params);
    }
    return {};
  } catch (const std::exception &e) {
    std::cerr << "[CompanyBrandingRepository::Read Exception] " << e.what() << std::endl;
    return {};
  }
}

bool CompanyBrandingRepository::Update(const omnisphere::dtos::UpdateCompanyBranding &dto, const std::vector<std::string>& mutationFields) const {
  try {
    EnsureDefaultExists(dto.CompanyEntry);

    auto mutableDto = dto;
    auto updateCols = omnisphere::types::ExtractUpdateColumns(mutableDto, mutationFields);

    bool hasUpdateDate = false;
    for (const auto& c : updateCols) {
      if (c.Column == "\"UpdateDate\"") {
        hasUpdateDate = true;
        break;
      }
    }
    if (!hasUpdateDate) {
      auto now = std::chrono::system_clock::now();
      auto in_time_t = std::chrono::system_clock::to_time_t(now);
      char buf[32];
      std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::gmtime(&in_time_t));
      updateCols.push_back({"\"UpdateDate\"", omnisphere::types::MakeSQLParam(std::string(buf))});
    }

    auto updateResult = omnisphere::types::BuildUpdateQuery(
      "\"CompanyBranding\"", updateCols, "\"CompanyEntry\"", omnisphere::types::MakeSQLParam(dto.CompanyEntry)
    );

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
    } else if (m_legacyDb) {
      return m_legacyDb->RunPrepared(updateResult.Query, updateResult.Parameters);
    }
    return false;
  } catch (const std::exception &e) {
    std::cerr << "[CompanyBrandingRepository::Update Exception] " << e.what() << std::endl;
    return false;
  }
}

} // namespace omnisphere::repositories

