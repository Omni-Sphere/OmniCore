#pragma once

#include "DTOs/Login.hpp"
#include "DTOs/Logout.hpp"
#include "Database.hpp"

namespace omnicore::repository {
class Session {
private:
  std::shared_ptr<service::Database> database;
  int GetCurrentSequence() const;
  bool UpdateSessionSequence() const;

public:
  explicit Session(std::shared_ptr<service::Database> Database);
  ~Session() {};

  bool Create(const dto::Login &login) const;
  bool Close(const dto::Logout &logout) const;
  type::DataTable ExistsUUID(const std::string &sessionUUID) const;
  type::DataTable Read(const std::string &) const;
  type::DataTable Read(const dto::Login &) const;
  type::DataTable IsActive(const std::string &) const;
};
} // namespace omnicore::repository