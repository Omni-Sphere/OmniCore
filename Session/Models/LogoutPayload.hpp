#pragma once

#include "Enums/LogoutReason.hpp"
#include <iostream>


namespace omnisphere::omnicore::models {
class LogoutPayload {
public:
  std::string SessionUUID;
  std::string StartDate;
  std::string EndDate;
  int Duration;
  omnisphere::omnicore::enums::LogoutReason Reason;
  std::optional<std::string> Message;
};
} // namespace omnisphere::omnicore::models