#pragma once
#include <Database.hpp>
#include <DataTable.hpp>
#include <User/Enums/PermissionMode.hpp>
#include <optional>
#include <string>

namespace omnisphere::dtos {
struct CreateUser {
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
  std::string Password;
  bool SuperUser;
  bool ChangePasswordNextLogin;
  bool PasswordNeverExpires;
  int CreatedBy;
  std::string CreateDate;
};
} // namespace omnisphere::dtos