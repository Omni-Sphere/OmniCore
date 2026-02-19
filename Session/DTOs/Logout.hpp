#pragma once

#include <optional>
#include <string>


#include "Enums/LogoutReason.hpp"

namespace omnisphere::omnicore::dtos {
struct Logout {
  std::string SessionUUID;
  std::string EndDate;
  omnisphere::omnicore::enums::LogoutReason Reason;
  std::optional<std::string> Message;
};
} // namespace omnisphere::omnicore::dtos