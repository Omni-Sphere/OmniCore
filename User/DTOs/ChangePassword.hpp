#pragma once
#include <optional>
#include <string>

namespace omnisphere::dtos {
struct ChangePassword {
  std::string Code;
  std::optional<std::string> OldPassword;
  std::string NewPassword;
  std::optional<bool> ChangePasswordNextLogin;
  std::optional<std::string> UpdateDate;
  std::optional<std::string> UpdatedBy;
};
} // namespace omnisphere::dtos