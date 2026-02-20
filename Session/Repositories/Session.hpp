#pragma once

#include "../../OmniData/Include/Database.hpp"
#include "../DTOs/Login.hpp"
#include "../DTOs/Logout.hpp"

namespace omnisphere::omnicore::repositories {
class Session {
private:
  std::shared_ptr<omnisphere::omnidata::services::Database> database;
  int GetCurrentSequence() const;
  bool UpdateSessionSequence() const;

public:
  explicit Session(
      std::shared_ptr<omnisphere::omnidata::services::Database> Database);
  ~Session() {};

  bool Create(const omnisphere::omnicore::dtos::Login &login) const;
  bool Close(const omnisphere::omnicore::dtos::Logout &logout) const;
  omnisphere::omnidata::types::DataTable
  ExistsUUID(const std::string &sessionUUID) const;
  omnisphere::omnidata::types::DataTable Read(const std::string &) const;
  omnisphere::omnidata::types::DataTable
  Read(const omnisphere::omnicore::dtos::Login &) const;
  omnisphere::omnidata::types::DataTable IsActive(const std::string &) const;
};
} // namespace omnisphere::omnicore::repositories