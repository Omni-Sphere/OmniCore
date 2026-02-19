#pragma once
#include <optional>
#include <string>


namespace omnisphere::omnicore::dtos {
struct ChangePassword {
  std::optional<int> Entry;
  std::optional<std::string> Code;
  std::string OldPassword;
  std::string NewPassword;
  std::string UpdateDate;
  int UpdatedBy;
};
} // namespace omnisphere::omnicore::dtos