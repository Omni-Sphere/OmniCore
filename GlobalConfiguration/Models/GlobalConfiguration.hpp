#pragma once
#include <optional>
#include <string>

namespace omnisphere::omnicore::models {

class GlobalConfiguration {
public:
  int ConfEntry;
  std::optional<std::string> ImagePath;
  std::optional<std::string> PDFPath;
  std::optional<std::string> XMLPath;
  int PasswordExpirationDays;
};
} // namespace omnisphere::omnicore::models
