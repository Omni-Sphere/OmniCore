#include <Database.hpp>
#include <DataTable.hpp>
#include <DataTable.hpp>
#include <Database.hpp>
#include <DataTable.hpp>
#include <User/Repositories/User.hpp>
#include <User/Enums/PermissionMode.hpp>
#include <Hasher.hpp>
#include <functional>

namespace omnisphere::repositories {
User::User(std::shared_ptr<omnisphere::services::Database> _database)
    : database(std::move(_database)) {}

bool User::Create(const omnisphere::dtos::CreateUser &user) const {
  try {
    database->BeginTransaction();

    GetCurrentSequence();

    std::vector<uint8_t> hashedPassword =
        omnisphere::utils::Hasher::HashPassword(user.Password);

    std::string sQuery = "INSERT INTO Users ("
                         "Entry, "
                         "[Code], "
                         "[Name], "
                         "Email, "
                         "Phone, "
                         "Employee, "
                         "RoleEntry, "
                         "MaxDisccountPerLine, "
                         "MaxDisccountPerDocument, "
                         "PermissionMode, "
                         "Department, "
                         "SuperUser, "
                         "IsLocked, "
                         "IsActive, "
                         "[Password], "
                         "PasswordNeverExpires, "
                         "ChangePasswordNextLogin, "
                         "CreatedBy, "
                         "CreateDate"
                         ") "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    const std::vector<omnisphere::types::SQLParam> params = {
        omnisphere::types::MakeSQLParam(GetCurrentSequence()),
        omnisphere::types::MakeSQLParam(user.Code),
        omnisphere::types::MakeSQLParam(user.Name),
        omnisphere::types::MakeSQLParam(user.Email),
        omnisphere::types::MakeSQLParam(user.Phone),
        omnisphere::types::MakeSQLParam(user.Employee),
        omnisphere::types::MakeSQLParam(user.RoleEntry),
        omnisphere::types::MakeSQLParam(user.MaxDisccountPerLine),
        omnisphere::types::MakeSQLParam(user.MaxDisccountPerDocument),
        omnisphere::types::MakeSQLParam(user.PermissionMode.has_value() ? std::optional<std::string>(user.PermissionMode.value() == omnisphere::enums::PermissionMode::P ? "P" : "M") : std::optional<std::string>("P")),
        omnisphere::types::MakeSQLParam(user.Department),
        omnisphere::types::MakeSQLParam(user.SuperUser),
        omnisphere::types::MakeSQLParam(false),
        omnisphere::types::MakeSQLParam(true),
        omnisphere::types::MakeSQLParam(hashedPassword),
        omnisphere::types::MakeSQLParam(user.PasswordNeverExpires),
        omnisphere::types::MakeSQLParam(user.ChangePasswordNextLogin),
        omnisphere::types::MakeSQLParam(user.CreatedBy),
        omnisphere::types::MakeSQLParam(user.CreateDate)};

    if (!database->RunPrepared(sQuery, params)) {
      database->RollbackTransaction();
      throw std::runtime_error("Error creating user");
    }

    if (!UpdateUserSequence()) {
      database->RollbackTransaction();
      throw std::runtime_error("Error updating sequence");
    }

    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw std::runtime_error(std::string("[CreateUser Exception] ") + " " +
                             e.what());
  }
}

bool User::UpdateUserSequence() const {
  try {
    const std::string sQuery =
        "UPDATE Sequences SET UserSequence = ISNULL(UserSequence,0) + 1";

    if (!database->RunStatement(sQuery))
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[UpdateUserSequence Exception] ") +
                             " " + e.what());
  }
};

int User::GetCurrentSequence() const {
  try {
    const std::string sQuery = "SELECT ISNULL(UserSequence, 0) + 1 "
                               "UserSequence FROM Sequences WHERE Entry = 1";

    omnisphere::types::DataTable data = database->FetchResults(sQuery);

    if (data.RowsCount() == 1)
      return data[0]["UserSequence"];
    else
      return 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetCurrentSequence Exception] ") +
                             " " + e.what());
  }
};

bool User::Update(const omnisphere::dtos::UpdateUser &user) const {
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

    if (user.Data.Email.has_value()) {
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
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Data.MaxDisccountPerLine.value()));
    }

    if (user.Data.MaxDisccountPerDocument.has_value()) {
      sQuery += "MaxDisccountPerDocument = ?, ";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(user.Data.MaxDisccountPerDocument.value()));
    }

    if (user.Data.PermissionMode.has_value()) {
      sQuery += "PermissionMode = ?, ";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(std::string(user.Data.PermissionMode.value() == omnisphere::enums::PermissionMode::P ? "P" : "M")));
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

    database->BeginTransaction();

    if (!database->RunPrepared(sQuery, updateParams))
      throw;

    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw std::runtime_error(std::string("[UpdateUSer Exception]") + e.what());
  }
};

bool User::UpdatePassword(const omnisphere::enums::UserFilter &filter,
                          const std::string &value,
                          const std::string &oldPassword,
                          const std::string &newPassword) const {
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

    database->BeginTransaction();
    if (!database->RunPrepared(sQuery, vParams))
      throw;

    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw std::runtime_error(std::string("[UpdatePassword Exception]: ") +
                             e.what());
  }
};

types::DataTable User::Read(const omnisphere::enums::UserFilter &filter,
                            const std::string &value) const {
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

    std::vector<omnisphere::types::SQLParam> vParams;

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
        database->FetchPrepared(sQuery, value);

    return dataTable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadByCode Exception] ") + " " +
                             e.what());
  }
};

types::DataTable User::Read(const omnisphere::dtos::SearchUsers &filter) const {
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

    std::function<void(const std::string &, const std::optional<std::string> &)>
        addCondition = [&](const std::string &field,
                           const std::optional<std::string> &value) {
          if (value.has_value()) {
            // TODO Implements LIKE condition
            //  if (filter.ExactValues) {
            conditions.push_back(field + " = ?");
            parameters.emplace_back(value.value());
            //}  else {
            //    conditions.push_back(field + " LIKE ?");
            //    parameters.emplace_back("%" + value.value() + "%");
            //}
          }
        };

    std::string query = baseQuery;

    /* if (!conditions.empty()) {
        query += " WHERE " + std::accumulate(
            std::next(conditions.begin()), conditions.end(), conditions[0],
            [&](std::string acc, const std::string& cond) {
                return acc + (filter.ExactValues ? " AND " : " OR ") + cond;
            }
        );
    } */

    omnisphere::types::DataTable dataTable =
        database->FetchPrepared(baseQuery, parameters);

    return dataTable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("ReadUsers exception: ") + e.what());
  }
}

bool User::ValidatePassword(const omnisphere::enums::UserFilter &searchFilter,
                            const std::string &filterValue,
                            const std::string &Password) const {
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
        database->FetchPrepared(sQuery, filterValue);

    if (data.RowsCount() == 0)
      throw std::runtime_error("No records found");

    std::vector<uint8_t> userPassword = data[0]["Password"];

    if (omnisphere::utils::Hasher::VerifyPassword(Password, userPassword))
      return true;

    return false;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw std::runtime_error(std::string("[ValidatePassword Exception]: ") +
                             e.what());
  }
};

bool User::ExistsEntry(const int &entry) const {
  try {
    const std::string sQuery =
        "SELECT ISNULL(COUNT(*), 0) Total FROM Users WHERE Entry = ?";

    omnisphere::types::DataTable data =
        database->FetchPrepared(sQuery, std::to_string(entry));

    if (data.RowsCount() == 0)
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(e.what());
  }
};

bool User::ExistsCode(const std::string &code) const {
  try {
    const std::string sQuery =
        "SELECT ISNULL(COUNT(*), 0) Total FROM Users WHERE [Code] = ?";

    omnisphere::types::DataTable data = database->FetchPrepared(sQuery, code);

    if (data.RowsCount() == 0)
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(e.what());
  }
};
} // namespace omnisphere::repositories
