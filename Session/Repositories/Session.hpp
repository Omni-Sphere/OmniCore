#pragma once

#include "../../OmniData/Include/Database.hpp"
#include "../DTOs/Login.hpp"
#include "../DTOs/Logout.hpp"

namespace omnisphere::repositories {
class Session {
private:
  std::shared_ptr<omnisphere::services::Database> database;
  int GetCurrentSequence() const;
  bool UpdateSessionSequence() const;

public:
  explicit Session(std::shared_ptr<omnisphere::services::Database> Database);
  ~Session() {};

  bool Create(const omnisphere::dtos::Login &login) const;
  bool Close(const omnisphere::dtos::Logout &logout) const;
  omnisphere::types::DataTable ExistsUUID(const std::string &sessionUUID) const;
  omnisphere::types::DataTable Read(const std::string &) const;
  omnisphere::types::DataTable Read(const omnisphere::dtos::Login &) const;
  omnisphere::types::DataTable IsActive(const std::string &) const;
};
} // namespace omnisphere::repositories