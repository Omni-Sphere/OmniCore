#pragma once

#include "../OmniData/Include/Database.hpp"

#include "DTOs/ChangePassword.hpp"
#include "DTOs/CreateUser.hpp"
#include "DTOs/SearchUsers.hpp"
#include "DTOs/UpdateUser.hpp"
#include "Enums/UserFilter.hpp"
#include "Models/User.hpp"

namespace omnisphere::omnicore::services {

class User {
public:
  explicit User(std::shared_ptr<omnidata::services::Database> database);

  ~User();

  bool Add(const omnisphere::omnicore::dtos::CreateUser &user) const;

  omnisphere::omnicore::models::User
  Modify(const omnisphere::omnicore::dtos::UpdateUser &user) const;

  bool ModifyPassword(const omnisphere::omnicore::dtos::ChangePassword &) const;

  bool CheckPassword(const omnisphere::omnicore::enums::UserFilter &filter,
                     const std::string &oldPassword,
                     const std::string &newPassword) const;

  bool LockUnlockUser(const omnisphere::omnicore::enums::UserFilter &,
                      const std::string &value, const bool &lock) const;

  std::vector<omnisphere::omnicore::models::User>
  Search(const omnisphere::omnicore::dtos::SearchUsers &user) const;

  omnisphere::omnicore::models::User
  Get(const omnisphere::omnicore::enums::UserFilter &filter,
      const std::string &value) const;

  bool Exists(const omnisphere::omnicore::enums::UserFilter &filter,
              const std::string &value) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnisphere::omnicore::services