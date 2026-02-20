#pragma once

#include <memory>
#include <string>

#include "../OmniData/Include/Database.hpp"

#include "Models/AuthPayload.hpp"
#include "Models/LogoutPayload.hpp"

#include "DTOs/Login.hpp"
#include "DTOs/Logout.hpp"

namespace omnisphere::omnicore::services {

class Session {
public:
  explicit Session(
      std::shared_ptr<omnisphere::omnidata::services::Database> database);

  ~Session();

  omnisphere::omnicore::models::AuthPayload
  Login(const omnisphere::omnicore::dtos::Login &login) const;

  omnisphere::omnicore::models::LogoutPayload
  Logout(const omnisphere::omnicore::dtos::Logout &logout) const;

  bool Exists(const std::string &token) const;

  bool Active(const std::string &token) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnisphere::omnicore::services