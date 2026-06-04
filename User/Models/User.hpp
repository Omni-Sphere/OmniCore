#pragma once
#include <Database.hpp>
#include <DataTable.hpp>
#include <User/Enums/PermissionMode.hpp>
#include <optional>
#include <string>

namespace omnisphere::models {
class User {
public:
  int Entry;
  std::string Code;
  std::optional<std::string> Name;
  std::optional<std::string> Email;
  std::optional<std::string> Phone;
  std::optional<int> Employee;
  
  std::optional<int> RoleEntry;
  std::optional<double> MaxDisccountPerLine;
  std::optional<double> MaxDisccountPerDocument;
  std::optional<omnisphere::enums::PermissionMode> PermissionMode;
  std::optional<int> Department;

  bool SuperUser;
  bool IsLocked;
  bool IsActive = true;
  bool ChangePasswordNextLogin;
  bool PasswordNeverExpires;
  int CreatedBy;
  std::string CreateDate;
  std::optional<int> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
};
} // namespace omnisphere::models