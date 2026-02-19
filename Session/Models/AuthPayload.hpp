#pragma once

#include "../../User/Models/User.hpp"
#include <iostream>
#include <memory>

namespace omnisphere::omnicore::models {
class AuthPayload {
public:
  std::string AccessToken;
  std::string SessionUUID;
  std::shared_ptr<omnisphere::omnicore::models::User> User;
};
} // namespace omnisphere::omnicore::models