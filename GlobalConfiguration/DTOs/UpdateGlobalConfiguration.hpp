#pragma once
#include <optional>
#include <string>

namespace omnicore::dto {

struct UpdateGlobalConfiguration {
  std::optional<std::string> ImagePath;
  std::optional<std::string> PDFPath;
  std::optional<std::string> XMLPath;
  std::optional<int> PasswordExpirationDays;
};
} // namespace omnicore::dto
