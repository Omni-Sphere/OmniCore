#pragma once

#include "../../OmniData/Include/Database.hpp"
#include "../DTOs/Login.hpp"
#include "../DTOs/Logout.hpp"


namespace omnisphere::omnicore::repositories {
class Session {
private:
  std::shared_ptr<omnidata::services::Database> database;
  int GetCurrentSequence() const;
  bool UpdateSessionSequence() const;

public:
  explicit Session(std::shared_ptr<omnidata::services::Database> Database);
  ~Session() {};

  bool Create(const omnisphere::omnicore::dtos::Login &login) const;
  bool Close(const omnisphere::omnicore::dtos::Logout &logout) const;
  omnidata::types::DataTable ExistsUUID(const std::string &sessionUUID) const;
  omnidata::types::DataTable Read(const std::string &) const;
  omnidata::types::DataTable
  Read(const omnisphere::omnicore::dtos::Login &) const;
  omnidata::types::DataTable IsActive(const std::string &) const;
};
} // namespace omnisphere::omnicore::repositories