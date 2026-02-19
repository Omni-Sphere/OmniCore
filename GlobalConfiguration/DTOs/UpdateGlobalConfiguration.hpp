#pragma once
#include <optional>
#include <string>

namespace omnisphere::omnicore::dtos {

struct UpdateGlobalConfiguration {
  std::optional<std::string> ImagePath;
  std::optional<std::string> PDFPath;
  std::optional<std::string> XMLPath;
  std::optional<int> PasswordExpirationDays;
};
} // namespace omnisphere::omnicore::dtos
