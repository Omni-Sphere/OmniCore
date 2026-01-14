#pragma once
#include <optional>
#include <string>

namespace omnicore::model {

class GlobalConfiguration {
public:
  int ConfEntry;
  std::optional<std::string> ImagePath;
  std::optional<std::string> PDFPath;
  std::optional<std::string> XMLPath;
  int PasswordExpirationDays;
};
} // namespace omnicore::model
