#include "Company/Repositories/Company.hpp"
#include "Company/Models/Company.hpp"
#include <OmniData/DataTable.hpp>
#include <OmniData/Database.hpp>
#include <OmniData/SQLParams.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <iostream>
#include <chrono>

namespace omnisphere::repositories {

Company::Company(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
    : m_dbPool(std::move(dbPool)) {}

Company::Company(std::shared_ptr<omnisphere::data::Database> database)
    : m_legacyDb(std::move(database)) {}

Company::~Company() = default;

bool Company::EnsureDefaultExists() const {
  try {
    std::string checkSql = "SELECT COUNT(*) as cnt FROM \"Company\"";
    std::vector<omnisphere::types::SQLParam> emptyParams;
    omnisphere::types::DataTable dt;

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      dt = conn->FetchPrepared(checkSql, emptyParams);
    } else if (m_legacyDb) {
      dt = m_legacyDb->FetchPrepared(checkSql, emptyParams);
    } else {
      return false;
    }

    if (!dt.IsEmpty()) {
      const auto& row = dt[0];
      if (row["cnt"].GetOptional<int>().value_or(0) > 0) {
        return true;
      }
    }

    std::string insertSql = 
      "INSERT INTO \"Company\" (\"Code\", \"Name\", \"CommercialName\", \"Address\", \"TaxID\", \"Currency\", \"Phone1\", \"Email\", \"WebSite\", \"PrimaryColor\", \"SecondaryColor\", \"IsActive\") "
      "VALUES ('COMP1', 'OmniRoute Logistics S.A. de C.V.', 'OmniRoute', 'Av. Insurgentes Sur 1602, CDMX', 'XAXX010101000', 1, '+52 55 1234 5678', 'contacto@omniroute.com', 'https://omniroute.com', '#45a6df', '#F2A65A', true)";
    
    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->RunPrepared(insertSql, emptyParams);
    } else if (m_legacyDb) {
      return m_legacyDb->RunPrepared(insertSql, emptyParams);
    }
    return false;
  } catch (const std::exception& e) {
    std::cerr << "[CompanyRepository::EnsureDefaultExists Exception] " << e.what() << std::endl;
    return false;
  }
}

bool Company::Create(const omnisphere::dtos::CreateCompany &_company) const {
  try {
    auto insertData = omnisphere::types::BuildInsertQuery("\"Company\"", 0, _company);
    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->RunPrepared(insertData.Query, insertData.Parameters);
    } else if (m_legacyDb) {
      return m_legacyDb->RunPrepared(insertData.Query, insertData.Parameters);
    }
    return false;
  } catch (const std::exception &e) {
    std::cerr << "[CompanyRepository::Create Exception] " << e.what() << std::endl;
    return false;
  }
}

bool Company::Update(const omnisphere::dtos::UpdateCompany &_company, const std::vector<std::string>& mutationFields) const {
  try {
    auto mutableCompany = _company;
    auto updateCols = omnisphere::types::ExtractUpdateColumns(mutableCompany, mutationFields);

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

    int targetEntry = _company.Entry;
    if (targetEntry <= 0) {
      targetEntry = 1;
    }

    auto updateResult = omnisphere::types::BuildUpdateQuery(
      "\"Company\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(targetEntry)
    );

    if (m_dbPool) {
      auto conn = m_dbPool->Acquire();
      return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
    } else if (m_legacyDb) {
      return m_legacyDb->RunPrepared(updateResult.Query, updateResult.Parameters);
    }
    return false;
  } catch (const std::exception &e) {
    std::cerr << "[CompanyRepository::Update Exception] " << e.what() << std::endl;
    return false;
  }
}

omnisphere::types::DataTable
Company::Read(const omnisphere::dtos::GetCompany &getCompany, const std::vector<std::string>& fields) const {
  try {
    EnsureDefaultExists();

    auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::Company>(fields);
    std::vector<omnisphere::types::Condition> conditions;
    std::vector<omnisphere::types::SQLParam> params;

    if (getCompany.Entry > 0) {
      conditions.push_back({"", "\"Entry\"", "=", "?"});
      params.push_back(omnisphere::types::MakeSQLParam(getCompany.Entry));
    } else if (!getCompany.Code.empty()) {
      conditions.push_back({"", "\"Code\"", "=", "?"});
      params.push_back(omnisphere::types::MakeSQLParam(getCompany.Code));
    }

    auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
    std::string sQuery = "SELECT " + qp.SelectClause + " FROM \"Company\"";
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
    std::cerr << "[CompanyRepository::Read Exception] " << e.what() << std::endl;
    return {};
  }
}

} // namespace omnisphere::repositories
