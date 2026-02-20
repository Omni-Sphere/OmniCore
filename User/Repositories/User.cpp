#include "User.hpp"
#include "../../OmniUtils/Include/Hasher.hpp"
#include <functional>

namespace omnisphere::omnicore::repositories {
User::User(std::shared_ptr<omnisphere::omnidata::services::Database> _database)
    : database(std::move(_database)) {}

bool User::Create(const omnisphere::omnicore::dtos::CreateUser &user) const {
  try {
    database->BeginTransaction();

    GetCurrentSequence();

    std::vector<uint8_t> hashedPassword =
        omnisphere::utils::Hasher::HashPassword(user.Password);

    std::string sQuery = "INSERT INTO Users ("
                         "UserEntry, "
                         "[Code], "
                         "[Name], "
                         "Email, "
                         "Phone, "
                         "EmployeeEntry, "
                         "SuperUser, "
                         "IsLocked, "
                         "IsActive, "
                         "[Password], "
                         "PasswordNeverExpires, "
                         "ChangePasswordNextLogin, "
                         "CreatedBy, "
                         "CreateDate"
                         ") "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    const std::vector<omnisphere::omnidata::types::SQLParam> params = {
        omnisphere::omnidata::types::MakeSQLParam(GetCurrentSequence()),
        omnisphere::omnidata::types::MakeSQLParam(user.Code),
        omnisphere::omnidata::types::MakeSQLParam(user.Name),
        omnisphere::omnidata::types::MakeSQLParam(user.Email),
        omnisphere::omnidata::types::MakeSQLParam(user.Phone),
        omnisphere::omnidata::types::MakeSQLParam(user.Employee),
        omnisphere::omnidata::types::MakeSQLParam(user.SuperUser),
        omnisphere::omnidata::types::MakeSQLParam(false),
        omnisphere::omnidata::types::MakeSQLParam(true),
        omnisphere::omnidata::types::MakeSQLParam(hashedPassword),
        omnisphere::omnidata::types::MakeSQLParam(user.PasswordNeverExpires),
        omnisphere::omnidata::types::MakeSQLParam(user.ChangePasswordNextLogin),
        omnisphere::omnidata::types::MakeSQLParam(user.CreatedBy),
        omnisphere::omnidata::types::MakeSQLParam(user.CreateDate)};

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
                               "UserSequence FROM Sequences WHERE SeqEntry = 1";

    omnisphere::omnidata::types::DataTable data =
        database->FetchResults(sQuery);

    if (data.RowsCount() == 1)
      return data[0]["UserSequence"];
    else
      return 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetCurrentSequence Exception] ") +
                             " " + e.what());
  }
};

bool User::Update(const omnisphere::omnicore::dtos::UpdateUser &user) const {
  try {
    std::string sQuery = "UPDATE Users SET ";
    std::vector<omnisphere::omnidata::types::SQLParam> updateParams;

    if (user.Data.Name.has_value()) {
      sQuery += "Name = ?, ";
      updateParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(user.Data.Name.value()));
    }

    if (user.Data.Email.has_value()) {
      sQuery += "Email = ?, ";
      updateParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(user.Data.Email.value()));
    }

    if (user.Data.Email.has_value()) {
      sQuery += "Phone = ?, ";
      updateParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(user.Data.Phone.value()));
    }

    if (user.Data.Employee.has_value()) {
      sQuery += "EmployeeEntry = ?, ";
      updateParams.emplace_back(omnisphere::omnidata::types::MakeSQLParam(
          user.Data.Employee.value()));
    }

    if (user.Where.Entry.has_value()) {
      sQuery += "WHERE UserEntry = ?";
      updateParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(user.Where.Entry.value()));
    }

    if (user.Where.Code.has_value()) {
      sQuery += "WHERE Code = ?";
      updateParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(user.Where.Code.value()));
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

bool User::UpdatePassword(const omnisphere::omnicore::enums::UserFilter &filter,
                          const std::string &value,
                          const std::string &oldPassword,
                          const std::string &newPassword) const {
  try {
    std::string sQuery = "UPDATE Users SET Password = ? WHERE ";

    const std::vector<uint8_t> hashedPassword =
        omnisphere::utils::Hasher::HashPassword(newPassword);

    std::vector<omnisphere::omnidata::types::SQLParam> vParams = {
        omnisphere::omnidata::types::MakeSQLParam(hashedPassword)};

    switch (filter) {
    case omnisphere::omnicore::enums::UserFilter::Code:
      sQuery += "Code = ?";
      vParams.push_back(omnisphere::omnidata::types::MakeSQLParam(value));
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

omnidata::types::DataTable
User::Read(const omnisphere::omnicore::enums::UserFilter &filter,
           const std::string &value) const {
  try {
    std::string sQuery = "SELECT "
                         "[UserEntry], "
                         "[Code], "
                         "[Name], "
                         "Email, "
                         "Phone, "
                         "EmployeeEntry, "
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

    std::vector<omnisphere::omnidata::types::SQLParam> vParams;

    switch (filter) {

    case omnisphere::omnicore::enums::UserFilter::Entry:
      sQuery += "[UserEntry] = ?";
      break;

    case omnisphere::omnicore::enums::UserFilter::Name:
      sQuery += "[Name] = ?";
      break;

    case omnisphere::omnicore::enums::UserFilter::Code:
      sQuery += "[Code] = ?";
      break;

    case omnisphere::omnicore::enums::UserFilter::Email:
      sQuery += "Email = ?";
      break;

    case omnisphere::omnicore::enums::UserFilter::Phone:
      sQuery += "Phone = ?";
      break;

    case omnisphere::omnicore::enums::UserFilter::Employee:
      sQuery += "EmployeeEntry = ?";
      break;

    default:
      break;
    }

    omnisphere::omnidata::types::DataTable dataTable =
        database->FetchPrepared(sQuery, value);

    return dataTable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadByCode Exception] ") + " " +
                             e.what());
  }
};

omnidata::types::DataTable
User::Read(const omnisphere::omnicore::dtos::SearchUsers &filter) const {
  try {
    std::string baseQuery = "SELECT "
                            "[UserEntry], "
                            "[Code], "
                            "[Name], "
                            "Email, "
                            "Phone, "
                            "IsLocked, "
                            "IsActive, "
                            "EmployeeEntry, "
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

    omnisphere::omnidata::types::DataTable dataTable =
        database->FetchPrepared(baseQuery, parameters);

    return dataTable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("ReadUsers exception: ") + e.what());
  }
}

bool User::ValidatePassword(
    const omnisphere::omnicore::enums::UserFilter &searchFilter,
    const std::string &filterValue, const std::string &Password) const {
  try {
    std::string sQuery = "SELECT Password FROM Users WHERE ";

    switch (searchFilter) {
    case omnisphere::omnicore::enums::UserFilter::Code:
      sQuery += "[Code] = ?";
      break;

    case omnisphere::omnicore::enums::UserFilter::Email:
      sQuery += "Email = ?";
      break;

    case omnisphere::omnicore::enums::UserFilter::Phone:
      sQuery += "Phone = ?";
      break;

    default:
      break;
    }

    omnisphere::omnidata::types::DataTable data =
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
        "SELECT ISNULL(COUNT(*), 0) Total FROM Users WHERE UserEntry = ?";

    omnisphere::omnidata::types::DataTable data =
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

    omnisphere::omnidata::types::DataTable data =
        database->FetchPrepared(sQuery, code);

    if (data.RowsCount() == 0)
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(e.what());
  }
};
} // namespace omnisphere::omnicore::repositories
