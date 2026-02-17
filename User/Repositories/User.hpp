#pragma once
#include "DTOs/CreateUser.hpp"
#include "DTOs/SearchUsers.hpp"
#include "DTOs/UpdateUser.hpp"
#include "Database.hpp"
#include "Enums/UserFilter.hpp"
#include "Models/User.hpp"
#include <memory>

namespace omnicore::repository {

class User {
private:
  std::shared_ptr<service::Database> database;
  int _UserEntry = -1;

  bool UpdateUserSequence() const;

  int GetCurrentSequence() const;

public:
  explicit User(std::shared_ptr<service::Database> database);

  ~User() {};

  bool Create(const dto::CreateUser &user) const;

  bool Update(const dto::UpdateUser &user) const;

  type::DataTable Read(const dto::SearchUsers &user) const;

  type::DataTable Read(const enums::UserFilter &filter,
                       const std::string &value) const;

  bool ExistsEntry(const int &entry) const;

  bool ExistsCode(const std::string &code) const;

  bool UpdatePassword(const enums::UserFilter &filter, const std::string &value,
                      const std::string &oldPassword,
                      const std::string &newPassword) const;

  bool ValidatePassword(const enums::UserFilter &filter,
                        const std::string &value,
                        const std::string &password) const;
};
} // namespace omnicore::repository