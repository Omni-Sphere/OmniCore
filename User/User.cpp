#include "User.hpp"
#include "Database.hpp"
#include "DataTable.hpp"
#include "Repositories/User.hpp"

namespace omnicore::service {

struct User::Impl {
  std::shared_ptr<repository::User> user;
  explicit Impl(std::shared_ptr<service::Database> _database)
      : user(std::make_shared<repository::User>(_database)) {}
};

User::User(std::shared_ptr<service::Database> _database)
    : pimpl(std::make_unique<Impl>(_database)) {}

User::~User() = default;

bool User::Add(const dto::CreateUser &newUser) const {
  try {
    if (Exists(enums::UserFilter::Code, newUser.Code))
      throw std::runtime_error("Code already exists");

    if (newUser.Name.has_value() &&
        Exists(enums::UserFilter::Name, newUser.Name.value()))
      throw std::runtime_error("Name already exists");

    if (newUser.Phone.has_value() &&
        Exists(enums::UserFilter::Phone, newUser.Phone.value()))
      throw std::runtime_error("Phone already exists");

    if (newUser.Email.has_value() &&
        Exists(enums::UserFilter::Email, newUser.Email.value()))
      throw std::runtime_error("Email already exists");

    if (pimpl->user->Create(newUser))
      return true;

    return false;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[UserExeption] ") + e.what());
  }
};

model::User User::Modify(const dto::UpdateUser &uUser) const {
  try {
    if (uUser.Where.Code.has_value() &&
        !Exists(enums::UserFilter::Code, uUser.Where.Code.value()))
      throw std::invalid_argument("User Code doesn't exists");

    if (uUser.Data.Email.has_value() &&
        Exists(enums::UserFilter::Email, uUser.Data.Email.value()))
      throw std::runtime_error("UserEmail already exists");

    if (uUser.Data.Name.has_value() &&
        Exists(enums::UserFilter::Name, uUser.Data.Name.value()))
      throw std::runtime_error("UserName already exists");

    if (uUser.Data.Phone.has_value() &&
        Exists(enums::UserFilter::Phone, uUser.Data.Phone.value()))
      throw std::runtime_error("UserPhone already exists");

    if (pimpl->user->Update(uUser)) {
      type::DataTable data;

      if (uUser.Where.Code.has_value())
        data = pimpl->user->Read(enums::UserFilter::Code,
                                 uUser.Where.Code.value());

      model::User user(
          data[0]["UserEntry"], data[0]["Code"],
          data[0]["Name"].GetOptional<std::string>(),
          data[0]["Email"].GetOptional<std::string>(),
          data[0]["Phone"].GetOptional<std::string>(), data[0]["EmployeeEntry"],
          data[0]["SuperUser"], data[0]["IsLocked"], data[0]["IsActive"],
          data[0]["ChangePasswordNextLogin"], data[0]["PasswordNeverExpires"],
          data[0]["CreatedBy"], data[0]["CreateDate"],
          data[0]["LastUpdatedBy"].GetOptional<int>(),
          data[0]["UpdateDate"].GetOptional<std::string>());

      return user;
    } else
      throw std::runtime_error("Error updating user ");
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[UpdateUser Exception]") + e.what());
  }
};

bool User::ModifyPassword(const dto::ChangePassword &userDTO) const {
  try {
    if (userDTO.Code.has_value() &&
        !CheckPassword(enums::UserFilter::Code, userDTO.Code.value(),
                       userDTO.OldPassword))
      throw std::invalid_argument("Wrong password");

    if (userDTO.Code.has_value() &&
        !pimpl->user->UpdatePassword(enums::UserFilter::Code,
                                     userDTO.Code.value(), userDTO.OldPassword,
                                     userDTO.NewPassword))
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ModifyPassword Exception]") +
                             e.what());
  }
};

std::vector<model::User> User::Search(const dto::SearchUsers &filter) const {
  try {
    std::vector<model::User> users;
    type::DataTable data = pimpl->user->Read(filter);

    if (data.RowsCount() == 0) {
      users.emplace_back(-1, "", "", std::nullopt, std::nullopt, -1, false,
                         false, false, false, false, -1, "", std::nullopt,
                         std::nullopt);
    }

    if (data.RowsCount() > 0)
      for (int i = 0; i < data.RowsCount(); i++)
        users.emplace_back(
            data[i]["UserEntry"], data[i]["Code"],
            data[i]["Name"].GetOptional<std::string>(),
            data[i]["Email"].GetOptional<std::string>(),
            data[i]["Phone"].GetOptional<std::string>(),
            data[i]["EmployeeEntry"].GetOptional<int>(), data[i]["SuperUser"],
            data[i]["IsLocked"], data[i]["IsActive"],
            data[i]["ChangePasswordNextLogin"], data[i]["PasswordNeverExpires"],
            data[i]["CreatedBy"], data[i]["CreateDate"],
            data[i]["LastUpdatedBy"].GetOptional<int>(),
            data[i]["UpdateDate"].GetOptional<std::string>());

    return users;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[SearhUsers Exception]") + e.what());
  }
};

model::User User::Get(const enums::UserFilter &filter,
                      const std::string &value) const {
  try {
    type::DataTable data;
    model::User userDef(-1, "", "", std::nullopt, std::nullopt, -1, false,
                        false, false, false, false, -1, "", std::nullopt,
                        std::nullopt);

    data = pimpl->user->Read(filter, value);

    if (data.RowsCount() > 1)
      throw std::runtime_error(
          "Inconstence retreiving Users, not exists or exists more than one");

    if (data.RowsCount() == 0)
      return userDef;

    model::User user(data[0]["UserEntry"], data[0]["Code"],
                     data[0]["Name"].GetOptional<std::string>(),
                     data[0]["Email"].GetOptional<std::string>(),
                     data[0]["Phone"].GetOptional<std::string>(),
                     data[0]["EmployeeEntry"].GetOptional<int>(),
                     data[0]["SuperUser"], data[0]["IsLocked"],
                     data[0]["IsActive"], data[0]["ChangePasswordNextLogin"],
                     data[0]["PasswordNeverExpires"], data[0]["CreatedBy"],
                     data[0]["CreateDate"],
                     data[0]["LastUpdatedBy"].GetOptional<int>(),
                     data[0]["UpdateDate"].GetOptional<std::string>());

    return user;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetUser Exception] ") + e.what());
  }
};

bool User::Exists(const enums::UserFilter &filter,
                  const std::string &value) const {
  try {
    type::DataTable data = pimpl->user->Read(filter, value);

    if (data.RowsCount() > 1)
      throw std::runtime_error(
          "Inconstence retreiving Users exists more than one");

    if (data.RowsCount() == 0)
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ExistsUser Exception] ") + e.what());
  }
};

bool User::CheckPassword(const enums::UserFilter &userFilter,
                         const std::string &value,
                         const std::string &oldPassword) const {
  try {
    switch (userFilter) {
    case enums::UserFilter::Code:
      if (!pimpl->user->ValidatePassword(enums::UserFilter::Code, value,
                                         oldPassword))
        return false;
      break;

    case enums::UserFilter::Email:
      if (!pimpl->user->ValidatePassword(enums::UserFilter::Email, value,
                                         oldPassword))
        return false;
      break;

    case enums::UserFilter::Phone:
      if (!pimpl->user->ValidatePassword(enums::UserFilter::Phone, value,
                                         oldPassword))
        return false;
      break;

    default:
      break;
    }

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[CheckPassword Exception] ") +
                             e.what());
  }
}

bool User::LockUnlockUser(const enums::UserFilter &userFilter,
                          const std::string &value, const bool &lock) const {
  try {
    switch (userFilter) {
    case enums::UserFilter::Code:
      if (!Exists(enums::UserFilter::Code, value))
        throw std::runtime_error("User Code doesn't exists");

    default:
      break;
    }

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[LockUnlockUser Exception] ") +
                             e.what());
  }
}
} // namespace omnicore::service