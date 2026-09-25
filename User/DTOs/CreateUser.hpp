#pragma once
#include "User/Enums/PermissionMode.hpp"
#include <boost/describe.hpp>
#include <optional>
#include <string>
#include <vector>

namespace omnisphere::dtos {
struct CreateUser {
  std::string Code;
  std::optional<std::string> Name;
  std::optional<std::string> Email;
  std::optional<std::string> Phone;
  std::optional<int> Employee;
  std::optional<std::string> EmployeeCode;
  std::optional<int> RoleEntry;
  std::optional<std::string> RoleCode;
  std::optional<double> MaxDisccountPerLine;
  std::optional<double> MaxDisccountPerDocument;
  std::optional<omnisphere::enums::PermissionMode> PermissionMode;
  std::optional<int> Department;
  bool SuperUser = false;
  bool IsLocked = false;
  bool IsActive = true;
  std::string Password;
  bool PasswordNeverExpires = false;
  bool ChangePasswordNextLogin = false;
  std::string CreatedBy = "SYSTEM";
  std::string CreateDate;
};

BOOST_DESCRIBE_STRUCT(CreateUser, (), (
  Code,
  Name,
  Email,
  Phone,
  Employee,
  RoleEntry,
  RoleCode,
  MaxDisccountPerLine,
  MaxDisccountPerDocument,
  PermissionMode,
  Department,
  SuperUser,
  IsLocked,
  IsActive,
  Password,
  PasswordNeverExpires,
  ChangePasswordNextLogin,
  CreatedBy,
  CreateDate,
  EmployeeCode
))

} // namespace omnisphere::dtos