#include <Database.hpp>
#include <DataTable.hpp>
#include <DataTable.hpp>
#include <Database.hpp>
#include <DataTable.hpp>
#include <User/User.hpp>
#include <User/Repositories/User.hpp>
#include <stdexcept>

namespace omnisphere::services {

struct User::Impl {
  std::shared_ptr<omnisphere::repositories::User> user;
  explicit Impl(std::shared_ptr<omnisphere::services::Database> _database)
      : user(std::make_shared<omnisphere::repositories::User>(_database)) {}
};

User::User(std::shared_ptr<omnisphere::services::Database> _database)
    : pimpl(std::make_unique<Impl>(_database)) {}

User::~User() = default;

bool User::Add(const omnisphere::dtos::CreateUser &newUser) const {
  try {
    if (Exists(omnisphere::enums::UserFilter::Code, newUser.Code))
      throw std::runtime_error("Code already exists");

    if (newUser.Name.has_value() &&
        Exists(omnisphere::enums::UserFilter::Name, newUser.Name.value()))
      throw std::runtime_error("Name already exists");

    if (newUser.Phone.has_value() &&
        Exists(omnisphere::enums::UserFilter::Phone, newUser.Phone.value()))
      throw std::runtime_error("Phone already exists");

    if (newUser.Email.has_value() &&
        Exists(omnisphere::enums::UserFilter::Email, newUser.Email.value()))
      throw std::runtime_error("Email already exists");

    if (pimpl->user->Create(newUser))
      return true;

    return false;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[UserExeption] ") + e.what());
  }
};

omnisphere::models::User
User::Modify(const omnisphere::dtos::UpdateUser &uUser) const {
  try {
    if (uUser.Where.Code.has_value() &&
        !Exists(omnisphere::enums::UserFilter::Code, uUser.Where.Code.value()))
      throw std::invalid_argument("User Code doesn't exists");

    if (uUser.Data.Email.has_value() &&
        Exists(omnisphere::enums::UserFilter::Email, uUser.Data.Email.value()))
      throw std::runtime_error("UserEmail already exists");

    if (uUser.Data.Name.has_value() &&
        Exists(omnisphere::enums::UserFilter::Name, uUser.Data.Name.value()))
      throw std::runtime_error("UserName already exists");

    if (uUser.Data.Phone.has_value() &&
        Exists(omnisphere::enums::UserFilter::Phone, uUser.Data.Phone.value()))
      throw std::runtime_error("UserPhone already exists");

    if (pimpl->user->Update(uUser)) {
      omnisphere::types::DataTable data;

      if (uUser.Where.Code.has_value())
        data = pimpl->user->Read(omnisphere::enums::UserFilter::Code,
                                 uUser.Where.Code.value());

      omnisphere::models::User user{
          data[0]["UserEntry"], data[0]["Code"],
          data[0]["Name"].GetOptional<std::string>(),
          data[0]["Email"].GetOptional<std::string>(),
          data[0]["Phone"].GetOptional<std::string>(),
          data[0]["EmpEntry"].GetOptional<int>(),
          data[0]["RoleEntry"].GetOptional<int>(),
          data[0]["MaxDiscountItem"].GetOptional<double>(),
          data[0]["MaxDiscountGeneral"].GetOptional<double>(),
          data[0]["PermissionMode"].GetOptional<std::string>(),
          data[0]["Department"].GetOptional<int>(),
          data[0]["SuperUser"], data[0]["IsLocked"], data[0]["IsActive"],
          data[0]["ChangePasswordNextLogin"], data[0]["PasswordNeverExpires"],
          data[0]["CreatedBy"], data[0]["CreateDate"],
          data[0]["LastUpdatedBy"].GetOptional<int>(),
          data[0]["UpdateDate"].GetOptional<std::string>()};

      return user;
    } else
      throw std::runtime_error("Error updating user ");
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[UpdateUser Exception]") + e.what());
  }
};

bool User::ModifyPassword(
    const omnisphere::dtos::ChangePassword &userDTO) const {
  try {
    if (userDTO.Code.has_value()) {
      if (!CheckPassword(omnisphere::enums::UserFilter::Code,
                         userDTO.Code.value(), userDTO.OldPassword))
        throw std::invalid_argument("Wrong password");

      if (!pimpl->user->UpdatePassword(
              omnisphere::enums::UserFilter::Code, userDTO.Code.value(),
              userDTO.OldPassword, userDTO.NewPassword))
        return false;
    } else if (userDTO.Entry.has_value()) {
      if (!CheckPassword(omnisphere::enums::UserFilter::Entry,
                         std::to_string(userDTO.Entry.value()),
                         userDTO.OldPassword))
        throw std::invalid_argument("Wrong password");

      if (!pimpl->user->UpdatePassword(omnisphere::enums::UserFilter::Entry,
                                       std::to_string(userDTO.Entry.value()),
                                       userDTO.OldPassword,
                                       userDTO.NewPassword))
        return false;
    }

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ModifyPassword Exception]") +
                             e.what());
  }
};

std::vector<omnisphere::models::User>
User::Search(const omnisphere::dtos::SearchUsers &filter) const {
  try {
    std::vector<omnisphere::models::User> users;
    omnisphere::types::DataTable data = pimpl->user->Read(filter);

    if (data.RowsCount() == 0) {
      users.emplace_back(omnisphere::models::User{-1, "", std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, false,
                         false, false, false, false, -1, "", std::nullopt,
                         std::nullopt});
    }

    if (data.RowsCount() > 0)
      for (int i = 0; i < data.RowsCount(); i++)
        users.emplace_back(omnisphere::models::User{
            data[i]["UserEntry"], data[i]["Code"],
            data[i]["Name"].GetOptional<std::string>(),
            data[i]["Email"].GetOptional<std::string>(),
            data[i]["Phone"].GetOptional<std::string>(),
            data[i]["EmpEntry"].GetOptional<int>(),
            data[i]["RoleEntry"].GetOptional<int>(),
            data[i]["MaxDiscountItem"].GetOptional<double>(),
            data[i]["MaxDiscountGeneral"].GetOptional<double>(),
            data[i]["PermissionMode"].GetOptional<std::string>(),
            data[i]["Department"].GetOptional<int>(),
            data[i]["SuperUser"],
            data[i]["IsLocked"], data[i]["IsActive"],
            data[i]["ChangePasswordNextLogin"], data[i]["PasswordNeverExpires"],
            data[i]["CreatedBy"], data[i]["CreateDate"],
            data[i]["LastUpdatedBy"].GetOptional<int>(),
            data[i]["UpdateDate"].GetOptional<std::string>()});

    return users;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[SearhUsers Exception]") + e.what());
  }
};

omnisphere::models::User User::Get(const omnisphere::enums::UserFilter &filter,
                                   const std::string &value) const {
  try {
    omnisphere::types::DataTable data;
    omnisphere::models::User userDef{-1, "", std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                     false, false, false, false, false, -1, "",
                                     std::nullopt, std::nullopt};

    data = pimpl->user->Read(filter, value);

    if (data.RowsCount() > 1)
      throw std::runtime_error(
          "Inconstence retreiving Users, not exists or exists more than one");

    if (data.RowsCount() == 0)
      return userDef;

    omnisphere::models::User user{
        data[0]["UserEntry"], data[0]["Code"],
        data[0]["Name"].GetOptional<std::string>(),
        data[0]["Email"].GetOptional<std::string>(),
        data[0]["Phone"].GetOptional<std::string>(),
        data[0]["EmpEntry"].GetOptional<int>(),
        data[0]["RoleEntry"].GetOptional<int>(),
        data[0]["MaxDiscountItem"].GetOptional<double>(),
        data[0]["MaxDiscountGeneral"].GetOptional<double>(),
        data[0]["PermissionMode"].GetOptional<std::string>(),
        data[0]["Department"].GetOptional<int>(),
        data[0]["SuperUser"],
        data[0]["IsLocked"], data[0]["IsActive"],
        data[0]["ChangePasswordNextLogin"], data[0]["PasswordNeverExpires"],
        data[0]["CreatedBy"], data[0]["CreateDate"],
        data[0]["LastUpdatedBy"].GetOptional<int>(),
        data[0]["UpdateDate"].GetOptional<std::string>()};

    return user;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetUser Exception] ") + e.what());
  }
};

bool User::Exists(const omnisphere::enums::UserFilter &filter,
                  const std::string &value) const {
  try {
    omnisphere::types::DataTable data = pimpl->user->Read(filter, value);

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

bool User::CheckPassword(const omnisphere::enums::UserFilter &userFilter, const std::string &value, const std::string &oldPassword) const {
  try {
    switch (userFilter) {
    case omnisphere::enums::UserFilter::Entry:
      if (!pimpl->user->ValidatePassword(omnisphere::enums::UserFilter::Entry, value, oldPassword))
        return false;
      break;

    case omnisphere::enums::UserFilter::Code:
      if (!pimpl->user->ValidatePassword(omnisphere::enums::UserFilter::Code, value, oldPassword))
        return false;
      break;

    case omnisphere::enums::UserFilter::Email:
      if (!pimpl->user->ValidatePassword(omnisphere::enums::UserFilter::Email, value, oldPassword))
        return false;
      break;

    case omnisphere::enums::UserFilter::Phone:
      if (!pimpl->user->ValidatePassword(omnisphere::enums::UserFilter::Phone, value, oldPassword))
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

bool User::LockUnlockUser(const omnisphere::enums::UserFilter &userFilter,
                          const std::string &value, const bool &lock) const {
  try {
    switch (userFilter) {
    case omnisphere::enums::UserFilter::Code:
      if (!Exists(omnisphere::enums::UserFilter::Code, value))
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
} // namespace omnisphere::services