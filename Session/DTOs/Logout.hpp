#pragma once

#include <optional>
#include <string>

#include "Enums/LogoutReason.hpp"

namespace omnisphere::dtos {
struct Logout {
  std::string SessionUUID;
  std::string EndDate;
  omnisphere::enums::LogoutReason Reason;
  std::optional<std::string> Message;
};
} // namespace omnisphere::dtos