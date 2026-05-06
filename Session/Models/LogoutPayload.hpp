#include <Database.hpp>
#include <DataTable.hpp>
#pragma once

#include <Session/Enums/LogoutReason.hpp>
#include <iostream>

namespace omnisphere::models {
class LogoutPayload {
public:
  std::string SessionUUID;
  std::string StartDate;
  std::string EndDate;
  int Duration;
  omnisphere::enums::LogoutReason Reason;
  std::optional<std::string> Message;
};
} // namespace omnisphere::models