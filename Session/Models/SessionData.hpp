#include <Database.hpp>
#include <DataTable.hpp>
#pragma once
#include <string>

namespace omnisphere::models {
class SessionData {
public:
  std::string AccessToken;
  std::string ExpiresAt;
  std::string IssuedAt;
  std::string SID;
};
} // namespace omnisphere::models