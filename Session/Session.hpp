#pragma once

#include <memory>
#include <string>

namespace omnicore::service {
class Database;
class User;
} // namespace omnicore::service
namespace omnicore::repository {
class Session;
}

#include "Models/AuthPayload.hpp"
#include "Models/LogoutPayload.hpp"

#include "DTOs/Login.hpp"
#include "DTOs/Logout.hpp"

namespace omnicore::service {

class Session {
public:
  explicit Session(std::shared_ptr<service::Database> database);

  ~Session();

  model::AuthPayload Login(const dto::Login &login) const;

  model::LogoutPayload Logout(const dto::Logout &logout) const;

  bool Exists(const std::string &token) const;

  bool Active(const std::string &token) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnicore::service