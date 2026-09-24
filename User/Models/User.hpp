#pragma once
#include <OmniData/DataTable.hpp>
#include <OmniData/Database.hpp>
#include "User/Enums/PermissionMode.hpp"
#include <boost/describe.hpp>
#include <memory>
#include <optional>
#include <string>

namespace omnisphere::models {
class User {
public:
  int Entry = 0;
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
  bool ChangePasswordNextLogin = false;
  bool PasswordNeverExpires = false;
  int CreatedBy = 1;
  std::string CreateDate;
  std::optional<int> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
  std::shared_ptr<User> CreatedByUser;
  std::shared_ptr<User> LastUpdatedByUser;
};

BOOST_DESCRIBE_STRUCT(User, (), (
    Entry, Code, Name, Email, Phone, Employee, EmployeeCode,
    RoleEntry, RoleCode, MaxDisccountPerLine, MaxDisccountPerDocument, Department,
    SuperUser, IsLocked, IsActive, ChangePasswordNextLogin, PasswordNeverExpires,
    CreatedBy, CreateDate, LastUpdatedBy, UpdateDate
))
} // namespace omnisphere::models