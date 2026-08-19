#pragma once

#include <OmniData/DatabasePool.hpp>

#include "DTOs/ChangePassword.hpp"
#include "DTOs/CreateUser.hpp"
#include "DTOs/SearchUsers.hpp"
#include "DTOs/UpdateUser.hpp"
#include "Enums/UserFilter.hpp"
#include "Models/User.hpp"
#include "Repositories/User.hpp"

namespace omnisphere::services {
class User {
public:
  explicit User(std::shared_ptr<omnisphere::data::DatabasePool> database);

  ~User();

  bool Add(const omnisphere::dtos::CreateUser &user) const;
  omnisphere::models::User
  Modify(const omnisphere::dtos::UpdateUser &user) const;
  bool ModifyPassword(const omnisphere::dtos::ChangePassword &) const;
  bool CheckPassword(const omnisphere::enums::UserFilter &filter,
                     const std::string &oldPassword,
                     const std::string &newPassword) const;
  bool LockUnlockUser(const omnisphere::enums::UserFilter &filter,
                      const std::string &value, const bool &lock) const;
  std::vector<omnisphere::models::User>
  Search(const omnisphere::dtos::SearchUsers &user) const;
  omnisphere::models::User Get(const omnisphere::enums::UserFilter &filter,
                               const std::string &value) const;
  bool Exists(const omnisphere::enums::UserFilter &filter,
              const std::string &value) const;

  omnisphere::repositories::UserCursorPage
  GetPage(std::optional<int> afterEntry, int limit) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnisphere::services