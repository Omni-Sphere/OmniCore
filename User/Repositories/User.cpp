#include <OmniUtils/Hasher.hpp>
#include "User/Enums/PermissionMode.hpp"
#include "User/Repositories/User.hpp"
#include <OmniData/QueryBuilder.hpp>
#include <functional>
#include <algorithm>
#include <sstream>

namespace omnisphere::repositories {

User::User(std::shared_ptr<omnisphere::data::DatabasePool> _database)
    : database(std::move(_database)) {}

bool User::Create(const omnisphere::dtos::CreateUser &user) const {
  auto conn = database->Acquire();
  try {
    conn->BeginTransaction();

    std::vector<uint8_t> hashedPassword =
        omnisphere::utils::Hasher::HashPassword(user.Password);

    std::string sQuery =
        "INSERT INTO \"Users\" ("
        "\"Code\", "
        "\"Name\", "
        "\"Email\", "
        "\"Phone\", "
        "\"Employee\", "
        "\"RoleEntry\", "
        "\"RoleCode\", "
        "\"MaxDisccountPerLine\", "
        "\"MaxDisccountPerDocument\", "
        "\"PermissionMode\", "
        "\"Department\", "
        "\"SuperUser\", "
        "\"IsLocked\", "
        "\"IsActive\", "
        "\"Password\", "
        "\"PasswordNeverExpires\", "
        "\"ChangePasswordNextLogin\", "
        "\"CreatedBy\", "
        "\"CreateDate\", "
        "\"EmployeeCode\""
        ") "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    const std::vector<omnisphere::types::SQLParam> params = {
        omnisphere::types::MakeSQLParam(user.Code),
        omnisphere::types::MakeSQLParam(user.Name),
        omnisphere::types::MakeSQLParam(user.Email),
        omnisphere::types::MakeSQLParam(user.Phone),
        omnisphere::types::MakeSQLParam(user.Employee),
        omnisphere::types::MakeSQLParam(user.RoleEntry),
        omnisphere::types::MakeSQLParam(user.RoleCode),
        omnisphere::types::MakeSQLParam(user.MaxDisccountPerLine),
        omnisphere::types::MakeSQLParam(user.MaxDisccountPerDocument),
        omnisphere::types::MakeSQLParam(
            user.PermissionMode.has_value()
                ? std::optional<std::string>(
                      user.PermissionMode.value() ==
                              omnisphere::enums::PermissionMode::P
                          ? "P"
                          : "R")
                : std::optional<std::string>("P")),
        omnisphere::types::MakeSQLParam(user.Department),
        omnisphere::types::MakeSQLParam(user.SuperUser),
        omnisphere::types::MakeSQLParam(false),
        omnisphere::types::MakeSQLParam(true),
        omnisphere::types::MakeSQLParam(hashedPassword),
        omnisphere::types::MakeSQLParam(user.PasswordNeverExpires),
        omnisphere::types::MakeSQLParam(user.ChangePasswordNextLogin),
        omnisphere::types::MakeSQLParam(user.CreatedBy),
        omnisphere::types::MakeSQLParam(user.CreateDate),
        omnisphere::types::MakeSQLParam(user.EmployeeCode)};

    std::cout << "\n==================================================" << std::endl;
    std::cout << "[OmniCore::User::Create] Executing SQL Query:" << std::endl;
    std::cout << sQuery << std::endl;
    std::cout << "[OmniCore::User::Create] Values: Code='" << user.Code
              << "', Name='" << user.Name.value_or("")
              << "', SuperUser='" << (user.SuperUser ? "true" : "false")
              << "', IsLocked='false', IsActive='true'"
              << "', PasswordNeverExpires='" << (user.PasswordNeverExpires ? "true" : "false")
              << "', ChangePasswordNextLogin='" << (user.ChangePasswordNextLogin ? "true" : "false")
              << "', CreatedBy=" << user.CreatedBy
              << ", CreateDate='" << user.CreateDate << "'" << std::endl;
    std::cout << "==================================================\n" << std::endl;

    if (!conn->RunPrepared(sQuery, params)) {
      conn->RollbackTransaction();
      throw std::runtime_error("Error executing User::Create statement");
    }

    conn->CommitTransaction();
    return true;
  } catch (const std::exception &e) {
    conn->RollbackTransaction();
    throw std::runtime_error(std::string("[CreateUser Exception] ") + " " +
                             e.what());
  }
}

bool User::UpdateUserSequence() const {
  auto conn = database->Acquire();
  try {
    const std::string sQuery =
        "UPDATE Sequences SET UserSequence = COALESCE(UserSequence,0) + 1";

    if (!conn->RunStatement(sQuery))
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[UpdateUserSequence Exception] ") +
                             " " + e.what());
  }
}

int User::GetCurrentSequence() const {
  auto conn = database->Acquire();
  try {
    const std::string sQuery = "SELECT COALESCE(UserSequence, 0) + 1 "
                               "UserSequence FROM Sequences WHERE Entry = 1";

    omnisphere::types::DataTable data = conn->FetchResults(sQuery);

    if (data.RowsCount() == 1)
      return data[0]["UserSequence"];
    else
      return 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetCurrentSequence Exception] ") +
                             " " + e.what());
  }
}

bool User::Update(const omnisphere::dtos::UpdateUser &user) const {
  auto conn = database->Acquire();

  try 
  {
    auto updateColumns = omnisphere::types::ExtractUpdateColumns(user.Data);

    if(updateColumns.empty())
      return false;
    
    auto updateResult = omnisphere::types::BuildUpdateQuery("\"Users\"", updateColumns, "\"Code\"", omnisphere::types::MakeSQLParam(user.Where.Code));

    if(!conn->RunPrepared(updateResult.Query, updateResult.Parameters))
      return false;
    
    return true;
  } 
  catch (const std::exception &e) 
  {
    conn->RollbackTransaction();
    throw std::runtime_error(std::string("[UpdateUser Exception]") + e.what());
  }
}

bool User::UpdatePassword(const omnisphere::enums::UserFilter &filter,
                          const std::string &value,
                          const std::string &oldPassword,
                          const std::string &newPassword) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery = "UPDATE Users SET Password = ? WHERE ";

    const std::vector<uint8_t> hashedPassword =
        omnisphere::utils::Hasher::HashPassword(newPassword);

    std::vector<omnisphere::types::SQLParam> vParams = {
        omnisphere::types::MakeSQLParam(hashedPassword)};

    switch (filter) {
    case omnisphere::enums::UserFilter::Code:
      sQuery += "Code = ?";
      vParams.push_back(omnisphere::types::MakeSQLParam(value));
      break;

    default:
      break;
    }

    conn->BeginTransaction();

    if (!conn->RunPrepared(sQuery, vParams))
      throw std::runtime_error("UpdatePassword failed");

    conn->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    conn->RollbackTransaction();
    throw std::runtime_error(std::string("[UpdatePassword Exception]: ") +
                             e.what());
  }
}

types::DataTable User::Read(const omnisphere::enums::UserFilter &filter,
                            const std::string &value,
                            const std::vector<std::string> &fields) const {
  auto conn = database->Acquire();
  try {
    auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::User>(fields);
    std::string filterCol;
    switch (filter) {
    case omnisphere::enums::UserFilter::Entry: filterCol = "\"Entry\""; break;
    case omnisphere::enums::UserFilter::Name: filterCol = "\"Name\""; break;
    case omnisphere::enums::UserFilter::Code: filterCol = "\"Code\""; break;
    case omnisphere::enums::UserFilter::Email: filterCol = "\"Email\""; break;
    case omnisphere::enums::UserFilter::Phone: filterCol = "\"Phone\""; break;
    case omnisphere::enums::UserFilter::Employee: filterCol = "\"Employee\""; break;
    default: filterCol = "\"Code\""; break;
    }

    std::vector<omnisphere::types::Condition> conditions = {
      {"", filterCol, "=", "?"},
      {"AND", "\"IsCanceled\"", "=", "?"}
    };
    auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
    std::string sQuery = "SELECT " + qp.SelectClause + " FROM \"Users\" WHERE " + qp.WhereClause;

    std::vector<omnisphere::types::SQLParam> params = {
      omnisphere::types::MakeSQLParam(value),
      omnisphere::types::MakeSQLParam(false)
    };

    return conn->FetchPrepared(sQuery, params);
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadByUserFilter Exception] ") + e.what());
  }
}

types::DataTable User::Read(const omnisphere::dtos::SearchUsers &filter,
                            const std::vector<std::string> &fields) const {
  auto conn = database->Acquire();
  try {
    auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::User>(fields);
    std::vector<omnisphere::types::Condition> conditions = {
      {"", "\"IsCanceled\"", "=", "?"}
    };
    auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
    std::string baseQuery = "SELECT " + qp.SelectClause + " FROM \"Users\" WHERE " + qp.WhereClause;

    return conn->FetchPrepared(baseQuery, { omnisphere::types::MakeSQLParam(false) });
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("ReadUsers exception: ") + e.what());
  }
}

omnisphere::types::DataTable User::GetByIds(const std::vector<int> &ids, const std::vector<std::string> &fields) const {
  if (ids.empty()) return omnisphere::types::DataTable{};
  auto conn = database->Acquire();
  auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::User>(fields);
  auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
  std::string sQuery = "SELECT " + qp.SelectClause + " FROM \"Users\" WHERE \"IsCanceled\" = false AND \"Entry\" IN (";
  std::vector<omnisphere::types::SQLParam> params;
  for (size_t i = 0; i < ids.size(); ++i) {
    if (i > 0) sQuery += ", ";
    sQuery += "?";
    params.push_back(omnisphere::types::MakeSQLParam(ids[i]));
  }
  sQuery += ")";
  return conn->FetchPrepared(sQuery, params);
}

UserCursorPage User::GetPage(std::optional<int> afterEntry, int limit, const std::vector<std::string> &fields) const {
  auto conn = database->Acquire();
  std::string countQuery = "SELECT COALESCE(COUNT(*), 0) AS Total FROM \"Users\" WHERE \"IsCanceled\" = false";
  auto totalTable = conn->FetchResults(countQuery);
  int totalCount = 0;
  if (totalTable.RowsCount() > 0) {
    totalCount = totalTable[0]["Total"];
  }

  auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::User>(fields);
  bool hasEntry = false;
  for (const auto& f : selectFields) {
    if (f == "\"Entry\"" || f == "Entry") { hasEntry = true; break; }
  }
  if (!hasEntry) {
    selectFields.insert(selectFields.begin(), "\"Entry\"");
  }

  std::vector<omnisphere::types::Condition> conditions = {
    {"", "\"IsCanceled\"", "=", "?"}
  };
  std::vector<omnisphere::types::SQLParam> params = {
    omnisphere::types::MakeSQLParam(false)
  };

  if (afterEntry.has_value()) {
    conditions.push_back({"AND", "\"Entry\"", ">", "?"});
    params.push_back(omnisphere::types::MakeSQLParam(afterEntry.value()));
  }

  auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
  std::string sQuery = "SELECT " + qp.SelectClause + " FROM \"Users\" WHERE " + qp.WhereClause + " ORDER BY \"Entry\" ASC LIMIT ?";
  params.push_back(omnisphere::types::MakeSQLParam(limit + 1));

  auto table = conn->FetchPrepared(sQuery, params);
  UserCursorPage page;
  page.totalCount = totalCount;
  page.hasPreviousPage = afterEntry.has_value();

  size_t rowLimit = std::min<size_t>(table.RowsCount(), static_cast<size_t>(limit));
  for (size_t i = 0; i < rowLimit; ++i) {
    page.users.push_back(omnisphere::types::FromDataRow<omnisphere::models::User>(table[i]));
  }

  if (table.RowsCount() > static_cast<size_t>(limit)) {
    page.nextCursor = page.users.back().Entry;
  }

  return page;
}

bool User::ValidatePassword(const omnisphere::enums::UserFilter &searchFilter,
                            const std::string &filterValue,
                            const std::string &Password) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery = "SELECT \"Password\" FROM \"Users\" WHERE ";

    switch (searchFilter) {
    case omnisphere::enums::UserFilter::Entry:
      sQuery += "\"Entry\" = ?";
      break;

    case omnisphere::enums::UserFilter::Code:
      sQuery += "\"Code\" = ?";
      break;

    case omnisphere::enums::UserFilter::Email:
      sQuery += "\"Email\" = ?";
      break;

    case omnisphere::enums::UserFilter::Phone:
      sQuery += "\"Phone\" = ?";
      break;

    default:
      break;
    }

    omnisphere::types::DataTable data =
        conn->FetchPrepared(sQuery, filterValue);

    if (data.RowsCount() == 0)
      throw std::runtime_error("No records found");

    std::vector<uint8_t> userPassword = data[0]["Password"];

    if (omnisphere::utils::Hasher::VerifyPassword(Password, userPassword))
      return true;

    return false;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ValidatePassword Exception]: ") +
                             e.what());
  }
}

bool User::ExistsEntry(const int &entry) const {
  auto conn = database->Acquire();
  try {
    const std::string sQuery =
        "SELECT COALESCE(COUNT(*), 0) AS \"Total\" FROM \"Users\" WHERE \"Entry\" = ?";

    omnisphere::types::DataTable data =
        conn->FetchPrepared(sQuery, std::to_string(entry));

    if (data.RowsCount() == 0)
      return false;

    int total = data[0]["Total"];
    return total > 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(e.what());
  }
}

bool User::ExistsCode(const std::string &code) const {
  auto conn = database->Acquire();
  try {
    const std::string sQuery =
        "SELECT COALESCE(COUNT(*), 0) AS \"Total\" FROM \"Users\" WHERE \"Code\" = ?";

    omnisphere::types::DataTable data = conn->FetchPrepared(sQuery, code);

    if (data.RowsCount() == 0)
      return false;

    int total = data[0]["Total"];
    return total > 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(e.what());
  }
}

bool User::Delete(const std::string &code) const {
  auto conn = database->Acquire();
  try {
    std::string sql = "UPDATE \"Users\" SET \"IsCanceled\" = true, \"IsActive\" = false, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"Code\" = ?";
    return conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[User Delete Exception] ") + e.what());
  }
}

} // namespace omnisphere::repositories
