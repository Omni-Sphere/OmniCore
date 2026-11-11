#include "Company/Repositories/CompanyPortalFeatureRepository.hpp"
#include "Company/Models/CompanyPortalFeature.hpp"
#include <OmniData/DataTable.hpp>
#include <OmniData/Database.hpp>
#include <OmniData/SQLParams.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <iostream>
#include <chrono>

namespace omnisphere::repositories {

CompanyPortalFeatureRepository::CompanyPortalFeatureRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
    : m_dbPool(std::move(dbPool)) {}

CompanyPortalFeatureRepository::CompanyPortalFeatureRepository(std::shared_ptr<omnisphere::data::Database> database)
    : m_legacyDb(std::move(database)) {}

CompanyPortalFeatureRepository::~CompanyPortalFeatureRepository() = default;

omnisphere::types::DataTable
CompanyPortalFeatureRepository::List(const omnisphere::dtos::GetPortalFeatures &dto, const std::vector<std::string>& fields) const {
  try {
    auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::CompanyPortalFeature>(fields);
    std::vector<omnisphere::types::Condition> conditions;
    std::vector<omnisphere::types::SQLParam> params;

    conditions.push_back({"", "\"CompanyEntry\"", "=", "?"});
    params.push_back(omnisphere::types::MakeSQLParam(dto.CompanyEntry));

    if (dto.OnlyActive.has_value() && dto.OnlyActive.value()) {
      conditions.push_back({"AND", "\"IsActive\"", "=", "?"});
      params.push_back(omnisphere::types::MakeSQLParam(true));
    }

    auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
    std::string sQuery = "SELECT " + qp.SelectClause + " FROM \"CompanyPortalFeatures\"";
    if (!qp.WhereClause.empty()) {
      sQuery += " WHERE " + qp.WhereClause;
    }
    sQuery += " ORDER BY \"SortOrder\" ASC, \"Entry\" ASC";

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->FetchPrepared(sQuery, params);
    } else if (m_legacyDb) {
      return m_legacyDb->FetchPrepared(sQuery, params);
    }
    return {};
  } catch (const std::exception &e) {
    std::cerr << "[CompanyPortalFeatureRepository::List Exception] " << e.what() << std::endl;
    return {};
  }
}

bool CompanyPortalFeatureRepository::Create(const omnisphere::dtos::CreatePortalFeature &dto) const {
  try {
    auto createDto = dto;
    if (!createDto.Code.has_value() || createDto.Code->empty()) {
      auto now = std::chrono::system_clock::now().time_since_epoch().count();
      createDto.Code = "FEAT-" + std::to_string(now % 1000000);
    }
    auto insertData = omnisphere::types::BuildInsertQuery("\"CompanyPortalFeatures\"", 0, createDto);
    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->RunPrepared(insertData.Query, insertData.Parameters);
    } else if (m_legacyDb) {
      return m_legacyDb->RunPrepared(insertData.Query, insertData.Parameters);
    }
    return false;
  } catch (const std::exception &e) {
    std::cerr << "[CompanyPortalFeatureRepository::Create Exception] " << e.what() << std::endl;
    return false;
  }
}

bool CompanyPortalFeatureRepository::Update(const omnisphere::dtos::UpdatePortalFeature &dto, const std::vector<std::string>& mutationFields) const {
  try {
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
      "\"CompanyPortalFeatures\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(dto.Entry)
    );

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
    } else if (m_legacyDb) {
      return m_legacyDb->RunPrepared(updateResult.Query, updateResult.Parameters);
    }
    return false;
  } catch (const std::exception &e) {
    std::cerr << "[CompanyPortalFeatureRepository::Update Exception] " << e.what() << std::endl;
    return false;
  }
}

bool CompanyPortalFeatureRepository::Delete(const omnisphere::dtos::DeletePortalFeature &dto) const {
  try {
    std::string sQuery = "DELETE FROM \"CompanyPortalFeatures\" WHERE \"Entry\" = ?";
    std::vector<omnisphere::types::SQLParam> params;
    params.push_back(omnisphere::types::MakeSQLParam(dto.Entry));

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->RunPrepared(sQuery, params);
    } else if (m_legacyDb) {
      return m_legacyDb->RunPrepared(sQuery, params);
    }
    return false;
  } catch (const std::exception &e) {
    std::cerr << "[CompanyPortalFeatureRepository::Delete Exception] " << e.what() << std::endl;
    return false;
  }
}

} // namespace omnisphere::repositories

