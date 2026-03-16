#pragma once

#include "../../User/Models/User.hpp"
#include <iostream>
#include <memory>

namespace omnisphere::models {
class AuthPayload {
public:
  std::string AccessToken;
  std::string SessionUUID;
  std::shared_ptr<omnisphere::models::User> User;
};
} // namespace omnisphere::models