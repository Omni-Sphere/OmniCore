#pragma once
#include <optional>
#include <string>

namespace omnisphere::omnicore::models {

class User {
public:
  int Entry;
  std::string Code;
  std::optional<std::string> Name;
  std::optional<std::string> Email;
  std::optional<std::string> Phone;
  std::optional<int> Employee;
  bool SuperUser;
  bool IsLocked;
  bool IsActive;
  bool ChangePasswordNextLogin;
  bool PasswordNeverExpires;
  int CreatedBy;
  std::string CreateDate;
  std::optional<int> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
};
} // namespace omnisphere::omnicore::models