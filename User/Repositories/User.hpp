#pragma once
#include "../../OmniData/Include/Database.hpp"
#include "DTOs/CreateUser.hpp"
#include "DTOs/SearchUsers.hpp"
#include "DTOs/UpdateUser.hpp"
#include "Enums/UserFilter.hpp"
#include "Models/User.hpp"
#include <memory>

namespace omnisphere::omnicore::repositories {

class User {
private:
  std::shared_ptr<omnisphere::omnidata::services::Database> database;
  int _UserEntry = -1;

  bool UpdateUserSequence() const;

  int GetCurrentSequence() const;

public:
  explicit User(
      std::shared_ptr<omnisphere::omnidata::services::Database> database);

  ~User() {};

  bool Create(const omnisphere::omnicore::dtos::CreateUser &user) const;

  bool Update(const omnisphere::omnicore::dtos::UpdateUser &user) const;

  omnisphere::omnidata::types::DataTable
  Read(const omnisphere::omnicore::dtos::SearchUsers &user) const;

  omnisphere::omnidata::types::DataTable
  Read(const omnisphere::omnicore::enums::UserFilter &filter,
       const std::string &value) const;

  bool ExistsEntry(const int &entry) const;

  bool ExistsCode(const std::string &code) const;

  bool UpdatePassword(const omnisphere::omnicore::enums::UserFilter &filter,
                      const std::string &value, const std::string &oldPassword,
                      const std::string &newPassword) const;

  bool ValidatePassword(const omnisphere::omnicore::enums::UserFilter &filter,
                        const std::string &value,
                        const std::string &password) const;
};
} // namespace omnisphere::omnicore::repositories