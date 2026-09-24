#include <stdexcept>

#include "User/Enums/PermissionMode.hpp"
#include "Repositories/User.hpp"
#include "User.hpp"
#include <OmniData/DataMapper.hpp>

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
User::Search(const omnisphere::dtos::SearchUsers &user, const std::vector<std::string> &fields) const {
  try {
    omnisphere::types::DataTable dataTable = pimpl->user->Read(user, fields);
    return omnisphere::types::DataTableToModels<omnisphere::models::User>(dataTable);
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[SearchUser Exception] ") + e.what());
  }
}

omnisphere::models::User User::Get(const omnisphere::enums::UserFilter &filter,
                                   const std::string &value, const std::vector<std::string> &fields) const {
  try {
    omnisphere::types::DataTable dataTable = pimpl->user->Read(filter, value, fields);
    if (dataTable.RowsCount() == 0)
      throw std::invalid_argument("User not found");

    return omnisphere::types::FromDataRow<omnisphere::models::User>(dataTable[0]);
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
User::GetPage(std::optional<int> afterEntry, int limit, const std::vector<std::string> &fields) const {
  return pimpl->user->GetPage(afterEntry, limit, fields);
}

bool User::Delete(const std::string &code) const {
  return pimpl->user->Delete(code);
}

} // namespace omnisphere::services