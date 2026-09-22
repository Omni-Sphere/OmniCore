#pragma once
#include "User/Enums/PermissionMode.hpp"
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::dtos {
struct UserCondition {
  std::optional<std::string> Code;
};

BOOST_DESCRIBE_STRUCT(UserCondition, (), (
  Code
))

struct UserData {
  std::optional<std::string> Name;
  std::optional<std::string> Email;
  std::optional<std::string> Phone;
  std::optional<int> Employee;
  std::optional<std::string> EmployeeCode;
  std::optional<bool> IsActive;
  std::optional<int> RoleEntry;
  std::optional<std::string> RoleCode;
  std::optional<double> MaxDisccountPerLine;
  std::optional<double> MaxDisccountPerDocument;
  std::optional<omnisphere::enums::PermissionMode> PermissionMode;
  std::optional<int> Department;
  std::optional<int> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
};

BOOST_DESCRIBE_STRUCT(UserData, (), (
  Name,
  Email,
  Phone,
  Employee,
  EmployeeCode,
  IsActive,
  RoleEntry,
  RoleCode,
  MaxDisccountPerLine,
  MaxDisccountPerDocument,
  PermissionMode,
  Department,
  LastUpdatedBy,
  UpdateDate
))

struct UpdateUser {
  UserCondition Where;
  UserData Data;
  std::string UpdateDate;
  int UpdatedBy;
};

BOOST_DESCRIBE_STRUCT(UpdateUser, (), (
  Where,
  Data,
  UpdateDate,
  UpdatedBy
))

} // namespace omnisphere::dtos