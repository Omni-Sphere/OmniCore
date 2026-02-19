#pragma once
#include <string>

namespace omnisphere::omnicore::models {
class SessionData {
public:
  std::string AccessToken;
  std::string ExpiresAt;
  std::string IssuedAt;
  std::string SID;
};
} // namespace omnisphere::omnicore::models