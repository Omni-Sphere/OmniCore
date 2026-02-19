#pragma once

#include <optional>
#include <string>


namespace omnisphere::omnicore::dtos {
struct Login {
  std::optional<std::string> Code;
  std::optional<std::string> Email;
  std::optional<std::string> Phone;
  std::string StartDate;
  std::string DeviceIP;
  std::string HostName;
  std::string Password;
};
} // namespace omnisphere::omnicore::dtos