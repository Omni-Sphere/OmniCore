#pragma once

#include "Database/Database.hpp"

#include "DTOs/ChangePassword.hpp"
#include "DTOs/CreateUser.hpp"
#include "DTOs/SearchUsers.hpp"
#include "DTOs/UpdateUser.hpp"
#include "Enums/UserFilter.hpp"
#include "Models/User.hpp"

namespace omnicore::service {

class User {
public:
  explicit User(std::shared_ptr<service::Database> database);

  ~User();

  bool Add(const dto::CreateUser &user) const;

  model::User Modify(const dto::UpdateUser &user) const;

  bool ModifyPassword(const dto::ChangePassword &) const;

  bool CheckPassword(const enums::UserFilter &filter,
                     const std::string &oldPassword,
                     const std::string &newPassword) const;

  bool LockUnlockUser(const enums::UserFilter &, const std::string &value,
                      const bool &lock) const;

  std::vector<model::User> Search(const dto::SearchUsers &user) const;

  model::User Get(const enums::UserFilter &filter,
                  const std::string &value) const;

  bool Exists(const enums::UserFilter &filter, const std::string &value) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnicore::service
  // namespace omnicore::service