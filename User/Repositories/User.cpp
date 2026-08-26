#include <OmniUtils/Hasher.hpp>
#include "User/Enums/PermissionMode.hpp"
#include "User/Repositories/User.hpp"
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
        "\"CreateDate\""
        ") "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    const std::vector<omnisphere::types::SQLParam> params = {
        omnisphere::types::MakeSQLParam(user.Code),
        omnisphere::types::MakeSQLParam(user.Name),
        omnisphere::types::MakeSQLParam(user.Email),
        omnisphere::types::MakeSQLParam(user.Phone),
        omnisphere::types::MakeSQLParam(user.Employee),
        omnisphere::types::MakeSQLParam(user.RoleEntry),
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
        omnisphere::types::MakeSQLParam(user.CreateDate)};

    std::cout << "\n==================================================" << std::endl;
    std::cout << "[OmniCore::User::Create] Executing SQL Query:" << std::endl;
    std::cout << sQuery << std::endl;
    std::cout << "[OmniCore::User::Create] Values: Code='" << user.Code
              << "', Name='" << user.Name
              << "', SuperUser='" << (user.SuperUser ? "Y" : "N")
              << "', IsLocked='N', IsActive='Y'"
              << "', PasswordNeverExpires='" << (user.PasswordNeverExpires ? "Y" : "N")
              << "', ChangePasswordNextLogin='" << (user.ChangePasswordNextLogin ? "Y" : "N")
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
  try {
    std::string sQuery = "UPDATE Users SET ";
    std::vector<omnisphere::types::SQLParam> updateParams;

    if (user.Data.Name.has_value()) {
      sQuery += "Name = ?, ";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Data.Name.value()));
    }

    if (user.Data.Email.has_value()) {
      sQuery += "Email = ?, ";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Data.Email.value()));
    }

    if (user.Data.Phone.has_value()) {
      sQuery += "Phone = ?, ";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Data.Phone.value()));
    }

    if (user.Data.Employee.has_value()) {
      sQuery += "Employee = ?, ";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Data.Employee.value()));
    }

    if (user.Data.RoleEntry.has_value()) {
      sQuery += "RoleEntry = ?, ";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Data.RoleEntry.value()));
    }

    if (user.Data.MaxDisccountPerLine.has_value()) {
      sQuery += "MaxDisccountPerLine = ?, ";
      updateParams.emplace_back(omnisphere::types::MakeSQLParam(
          user.Data.MaxDisccountPerLine.value()));
    }

    if (user.Data.MaxDisccountPerDocument.has_value()) {
      sQuery += "MaxDisccountPerDocument = ?, ";
      updateParams.emplace_back(omnisphere::types::MakeSQLParam(
          user.Data.MaxDisccountPerDocument.value()));
    }

    if (user.Data.PermissionMode.has_value()) {
      sQuery += "PermissionMode = ?, ";
      updateParams.emplace_back(omnisphere::types::MakeSQLParam(
          std::string(user.Data.PermissionMode.value() ==
                              omnisphere::enums::PermissionMode::P
                          ? "P"
                          : "M")));
    }

    if (user.Data.Department.has_value()) {
      sQuery += "Department = ?, ";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Data.Department.value()));
    }

    if (user.Where.Entry.has_value()) {
      sQuery += "WHERE Entry = ?";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Where.Entry.value()));
    }

    if (user.Where.Code.has_value()) {
      sQuery += "WHERE Code = ?";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Where.Code.value()));
    }

    conn->BeginTransaction();

    if (!conn->RunPrepared(sQuery, updateParams))
      throw std::runtime_error("Update failed");

    conn->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
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
                            const std::string &value) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery = "SELECT "
                         "[Entry] AS UserEntry, "
                         "[Code], "
                         "[Name], "
                         "Email, "
                         "Phone, "
                         "Employee AS EmpEntry, "
                         "RoleEntry, "
                         "MaxDisccountPerLine, "
                         "MaxDisccountPerDocument, "
                         "PermissionMode, "
                         "Department, "
                         "SuperUser, "
                         "IsLocked, "
                         "IsActive, "
                         "ChangePasswordNextLogin, "
                         "PasswordNeverExpires, "
                         "CreateDate, "
                         "CreatedBy, "
                         "LastUpdatedBy, "
                         "UpdateDate "
                         "FROM Users WHERE ";

    switch (filter) {
    case omnisphere::enums::UserFilter::Entry:
      sQuery += "[Entry] = ?";
      break;

    case omnisphere::enums::UserFilter::Name:
      sQuery += "[Name] = ?";
      break;

    case omnisphere::enums::UserFilter::Code:
      sQuery += "[Code] = ?";
      break;

    case omnisphere::enums::UserFilter::Email:
      sQuery += "Email = ?";
      break;

    case omnisphere::enums::UserFilter::Phone:
      sQuery += "Phone = ?";
      break;

    case omnisphere::enums::UserFilter::Employee:
      sQuery += "Employee = ?";
      break;

    default:
      break;
    }

    omnisphere::types::DataTable dataTable =
        conn->FetchPrepared(sQuery, value);

    return dataTable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadByCode Exception] ") + " " +
                             e.what());
  }
}

types::DataTable User::Read(const omnisphere::dtos::SearchUsers &filter) const {
  auto conn = database->Acquire();
  try {
    std::string baseQuery = "SELECT "
                            "[Entry] AS UserEntry, "
                            "[Code], "
                            "[Name], "
                            "Email, "
                            "Phone, "
                            "IsLocked, "
                            "IsActive, "
                            "Employee AS EmpEntry, "
                            "RoleEntry, "
                            "MaxDisccountPerLine, "
                            "MaxDisccountPerDocument, "
                            "PermissionMode, "
                            "Department, "
                            "SuperUser, "
                            "PasswordNeverExpires, "
                            "ChangePasswordNextLogin, "
                            "CreatedBy, "
                            "LastUpdatedBy, "
                            "UpdateDate "
                            "FROM Users";

    std::vector<std::string> conditions;
    std::vector<std::string> parameters;

    omnisphere::types::DataTable dataTable =
        conn->FetchPrepared(baseQuery, parameters);

    return dataTable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("ReadUsers exception: ") + e.what());
  }
}

omnisphere::types::DataTable User::GetByIds(const std::vector<int> &ids) const {
  if (ids.empty()) return omnisphere::types::DataTable{};
  auto conn = database->Acquire();
  std::string sQuery = "SELECT Entry, Code, Name, Email, Phone, IsLocked, IsActive, "
                       "RoleEntry, SuperUser, CreateDate FROM Users WHERE Entry IN (";
  std::vector<omnisphere::types::SQLParam> params;
  for (size_t i = 0; i < ids.size(); ++i) {
    if (i > 0) sQuery += ", ";
    sQuery += "?";
    params.push_back(omnisphere::types::MakeSQLParam(ids[i]));
  }
  sQuery += ")";
  return conn->FetchPrepared(sQuery, params);
}

UserCursorPage User::GetPage(std::optional<int> afterEntry, int limit) const {
  auto conn = database->Acquire();
  std::string countQuery = "SELECT COALESCE(COUNT(*), 0) AS Total FROM Users";
  auto totalTable = conn->FetchResults(countQuery);
  int totalCount = 0;
  if (totalTable.RowsCount() > 0) {
    totalCount = totalTable[0]["Total"];
  }

  std::string sQuery;
  std::vector<omnisphere::types::SQLParam> params;

  if (afterEntry.has_value()) {
    sQuery = "SELECT Entry, Code, Name, Email, Phone, IsLocked, IsActive, CreatedBy, CreateDate "
             "FROM Users WHERE Entry > ? ORDER BY Entry ASC LIMIT ?";
    params.push_back(omnisphere::types::MakeSQLParam(afterEntry.value()));
    params.push_back(omnisphere::types::MakeSQLParam(limit + 1));
  } else {
    sQuery = "SELECT Entry, Code, Name, Email, Phone, IsLocked, IsActive, CreatedBy, CreateDate "
             "FROM Users ORDER BY Entry ASC LIMIT ?";
    params.push_back(omnisphere::types::MakeSQLParam(limit + 1));
  }

  auto table = conn->FetchPrepared(sQuery, params);
  UserCursorPage page;
  page.totalCount = totalCount;
  page.hasPreviousPage = afterEntry.has_value();

  size_t rowLimit = std::min<size_t>(table.RowsCount(), static_cast<size_t>(limit));
  for (size_t i = 0; i < rowLimit; ++i) {
    omnisphere::models::User u;
    u.Entry = table[i]["Entry"];
    u.Code = static_cast<std::string>(table[i]["Code"]);
    if (!table[i]["Name"].IsNull()) u.Name = static_cast<std::string>(table[i]["Name"]);
    if (!table[i]["Email"].IsNull()) u.Email = static_cast<std::string>(table[i]["Email"]);
    if (!table[i]["Phone"].IsNull()) u.Phone = static_cast<std::string>(table[i]["Phone"]);
    u.IsLocked = table[i]["IsLocked"];
    u.IsActive = table[i]["IsActive"];
    page.users.push_back(u);
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
    std::string sQuery = "SELECT Password FROM Users WHERE ";

    switch (searchFilter) {
    case omnisphere::enums::UserFilter::Entry:
      sQuery += "[Entry] = ?";
      break;

    case omnisphere::enums::UserFilter::Code:
      sQuery += "[Code] = ?";
      break;

    case omnisphere::enums::UserFilter::Email:
      sQuery += "Email = ?";
      break;

    case omnisphere::enums::UserFilter::Phone:
      sQuery += "Phone = ?";
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
    conn->RollbackTransaction();
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

} // namespace omnisphere::repositories
