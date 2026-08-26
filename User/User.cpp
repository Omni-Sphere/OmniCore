#include <stdexcept>

#include "Enums/PermissionMode.hpp"
#include "Repositories/User.hpp"
#include "User.hpp"

namespace omnisphere::services {
struct User::Impl {
  std::shared_ptr<omnisphere::repositories::User> user;
  explicit Impl(std::shared_ptr<omnisphere::data::DatabasePool> db)
      : user(std::make_shared<omnisphere::repositories::User>(db)) {}
};

User::User(std::shared_ptr<omnisphere::data::DatabasePool> db)
    : pimpl(std::make_unique<Impl>(db)) {}

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
}

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
      throw std::runtime_error("User Phone already exists");

    if (!pimpl->user->Update(uUser))
      throw std::runtime_error("User wasn't modified");

    return Get(omnisphere::enums::UserFilter::Code, uUser.Where.Code.value());

  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ModifyUser Exeption] ") + e.what());
  }
}

bool User::ModifyPassword(const omnisphere::dtos::ChangePassword &cPass) const {
  try {
    if (!cPass.Code.has_value() || cPass.Code.value().empty() || cPass.OldPassword.empty() ||
        cPass.NewPassword.empty())
      throw std::invalid_argument(
          "Code, OldPassword and NewPassword are required");

    if (!Exists(omnisphere::enums::UserFilter::Code, cPass.Code.value()))
      throw std::invalid_argument("User Code doesn't exists");

    if (!pimpl->user->ValidatePassword(omnisphere::enums::UserFilter::Code,
                                       cPass.Code.value(), cPass.OldPassword))
      throw std::invalid_argument("Invalid password");

    if (pimpl->user->UpdatePassword(omnisphere::enums::UserFilter::Code,
                                    cPass.Code.value(), cPass.OldPassword,
                                    cPass.NewPassword))
      return true;

    return false;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ModifyPassword Exception] ") +
                             e.what());
  }
}

bool User::CheckPassword(const omnisphere::enums::UserFilter &filter,
                         const std::string &value,
                         const std::string &password) const {
  try {
    return pimpl->user->ValidatePassword(filter, value, password);
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[CheckPassword Exception] ") +
                             e.what());
  }
}

bool User::LockUnlockUser(const omnisphere::enums::UserFilter &filter,
                          const std::string &value, const bool &lock) const {
  return true;
}

std::vector<omnisphere::models::User>
User::Search(const omnisphere::dtos::SearchUsers &user) const {
  try {
    omnisphere::types::DataTable dataTable = pimpl->user->Read(user);
    std::vector<omnisphere::models::User> vUsers;

    for (size_t i = 0; i < dataTable.RowsCount(); i++) {
      omnisphere::models::User UserData;

      UserData.Entry = dataTable[i]["UserEntry"];
      UserData.Code = static_cast<std::string>(dataTable[i]["Code"]);

      if (!dataTable[i]["Name"].IsNull())
        UserData.Name = static_cast<std::string>(dataTable[i]["Name"]);

      if (!dataTable[i]["Email"].IsNull())
        UserData.Email = static_cast<std::string>(dataTable[i]["Email"]);

      if (!dataTable[i]["Phone"].IsNull())
        UserData.Phone = static_cast<std::string>(dataTable[i]["Phone"]);

      if (!dataTable[i]["EmpEntry"].IsNull())
        UserData.Employee = static_cast<int>(dataTable[i]["EmpEntry"]);

      if (!dataTable[i]["RoleEntry"].IsNull())
        UserData.RoleEntry = static_cast<int>(dataTable[i]["RoleEntry"]);

      if (!dataTable[i]["MaxDisccountPerLine"].IsNull())
        UserData.MaxDisccountPerLine =
            static_cast<double>(dataTable[i]["MaxDisccountPerLine"]);

      if (!dataTable[i]["MaxDisccountPerDocument"].IsNull())
        UserData.MaxDisccountPerDocument =
            static_cast<double>(dataTable[i]["MaxDisccountPerDocument"]);

      if (!dataTable[i]["PermissionMode"].IsNull()) {
        std::string mode =
            static_cast<std::string>(dataTable[i]["PermissionMode"]);
        UserData.PermissionMode = mode == "P"
                                      ? omnisphere::enums::PermissionMode::P
                                      : omnisphere::enums::PermissionMode::R;
      }

      if (!dataTable[i]["Department"].IsNull())
        UserData.Department = static_cast<int>(dataTable[i]["Department"]);

      UserData.SuperUser = dataTable[i]["SuperUser"];
      UserData.IsLocked = dataTable[i]["IsLocked"];
      UserData.IsActive = dataTable[i]["IsActive"];
      UserData.PasswordNeverExpires = dataTable[i]["PasswordNeverExpires"];
      UserData.ChangePasswordNextLogin = dataTable[i]["ChangePasswordNextLogin"];
      UserData.CreatedBy = dataTable[i]["CreatedBy"];
      UserData.CreateDate = static_cast<std::string>(dataTable[i]["CreateDate"]);

      if (!dataTable[i]["LastUpdatedBy"].IsNull())
        UserData.LastUpdatedBy = static_cast<int>(dataTable[i]["LastUpdatedBy"]);

      if (!dataTable[i]["UpdateDate"].IsNull())
        UserData.UpdateDate =
            static_cast<std::string>(dataTable[i]["UpdateDate"]);

      vUsers.push_back(UserData);
    }

    return vUsers;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[SearchUser Exception] ") + e.what());
  }
}

omnisphere::models::User User::Get(const omnisphere::enums::UserFilter &filter,
                                   const std::string &value) const {
  try {
    omnisphere::types::DataTable dataTable = pimpl->user->Read(filter, value);
    if (dataTable.RowsCount() == 0)
      throw std::invalid_argument("User not found");

    omnisphere::models::User UserData;

    UserData.Entry = dataTable[0]["UserEntry"];
    UserData.Code = static_cast<std::string>(dataTable[0]["Code"]);

    if (!dataTable[0]["Name"].IsNull())
      UserData.Name = static_cast<std::string>(dataTable[0]["Name"]);

    if (!dataTable[0]["Email"].IsNull())
      UserData.Email = static_cast<std::string>(dataTable[0]["Email"]);

    if (!dataTable[0]["Phone"].IsNull())
      UserData.Phone = static_cast<std::string>(dataTable[0]["Phone"]);

    if (!dataTable[0]["EmpEntry"].IsNull())
      UserData.Employee = static_cast<int>(dataTable[0]["EmpEntry"]);

    if (!dataTable[0]["RoleEntry"].IsNull())
      UserData.RoleEntry = static_cast<int>(dataTable[0]["RoleEntry"]);

    if (!dataTable[0]["MaxDisccountPerLine"].IsNull())
      UserData.MaxDisccountPerLine =
          static_cast<double>(dataTable[0]["MaxDisccountPerLine"]);

    if (!dataTable[0]["MaxDisccountPerDocument"].IsNull())
      UserData.MaxDisccountPerDocument =
          static_cast<double>(dataTable[0]["MaxDisccountPerDocument"]);

    if (!dataTable[0]["PermissionMode"].IsNull()) {
      std::string mode =
          static_cast<std::string>(dataTable[0]["PermissionMode"]);
      UserData.PermissionMode = mode == "P"
                                    ? omnisphere::enums::PermissionMode::P
                                    : omnisphere::enums::PermissionMode::R;
    }

    if (!dataTable[0]["Department"].IsNull())
      UserData.Department = static_cast<int>(dataTable[0]["Department"]);

    UserData.SuperUser = dataTable[0]["SuperUser"];
    UserData.IsLocked = dataTable[0]["IsLocked"];
    UserData.IsActive = dataTable[0]["IsActive"];
    UserData.PasswordNeverExpires = dataTable[0]["PasswordNeverExpires"];
    UserData.ChangePasswordNextLogin = dataTable[0]["ChangePasswordNextLogin"];
    UserData.CreatedBy = dataTable[0]["CreatedBy"];
    UserData.CreateDate = static_cast<std::string>(dataTable[0]["CreateDate"]);

    if (!dataTable[0]["LastUpdatedBy"].IsNull())
      UserData.LastUpdatedBy = static_cast<int>(dataTable[0]["LastUpdatedBy"]);

    if (!dataTable[0]["UpdateDate"].IsNull())
      UserData.UpdateDate =
          static_cast<std::string>(dataTable[0]["UpdateDate"]);

    return UserData;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetUser Exception] ") + e.what());
  }
}

bool User::Exists(const omnisphere::enums::UserFilter &filter,
                  const std::string &value) const {
  try {
    switch (filter) {
    case omnisphere::enums::UserFilter::Entry:
      return pimpl->user->ExistsEntry(std::stoi(value));

    case omnisphere::enums::UserFilter::Code:
      return pimpl->user->ExistsCode(value);

    default:
      return false;
    }
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ExistsUser Exception] ") + e.what());
  }
}

omnisphere::repositories::UserCursorPage
User::GetPage(std::optional<int> afterEntry, int limit) const {
  return pimpl->user->GetPage(afterEntry, limit);
}

} // namespace omnisphere::services