#pragma once
#include <OmniData/DataTable.hpp>
#include <OmniData/DatabasePool.hpp>
#include "User/DTOs/CreateUser.hpp"
#include "User/DTOs/SearchUsers.hpp"
#include "User/DTOs/UpdateUser.hpp"
#include "User/Enums/UserFilter.hpp"
#include "User/Models/User.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace omnisphere::repositories {

struct UserCursorPage {
  std::vector<omnisphere::models::User> users;
  std::optional<int> nextCursor;
  bool hasPreviousPage = false;
  int totalCount = 0;
};

class User {
private:
  std::shared_ptr<omnisphere::data::DatabasePool> database;
  int _UserEntry = -1;

  bool UpdateUserSequence() const;
  int GetCurrentSequence() const;

public:
  explicit User(std::shared_ptr<omnisphere::data::DatabasePool> database);

  ~User() {};

  bool Create(const omnisphere::dtos::CreateUser &user) const;

  bool Update(const omnisphere::dtos::UpdateUser &user) const;

  omnisphere::types::DataTable
  Read(const omnisphere::dtos::SearchUsers &user) const;

  omnisphere::types::DataTable Read(const omnisphere::enums::UserFilter &filter,
                                    const std::string &value) const;

  // Batch lookup for DataLoader
  omnisphere::types::DataTable GetByIds(const std::vector<int> &ids) const;

  // Keyset pagination (cursor = Entry)
  UserCursorPage GetPage(std::optional<int> afterEntry, int limit) const;

  bool ExistsEntry(const int &entry) const;

  bool ExistsCode(const std::string &code) const;

  bool UpdatePassword(const omnisphere::enums::UserFilter &filter,
                      const std::string &value, const std::string &oldPassword,
                      const std::string &newPassword) const;

  bool ValidatePassword(const omnisphere::enums::UserFilter &filter,
                        const std::string &value,
                        const std::string &password) const;
};
} // namespace omnisphere::repositories