#pragma once
#include <OmniData/DataTable.hpp>
#include <OmniData/Database.hpp>
#include "User/Enums/PermissionMode.hpp"
#include "Employee/Models/Employee.hpp"
#include "Authorization/Models/Role.hpp"
#include <boost/describe.hpp>
#include <memory>
#include <optional>
#include <string>

namespace omnisphere::dtos {
struct UserView {
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
  bool IsCanceled = false;
  bool ChangePasswordNextLogin = false;
  bool PasswordNeverExpires = false;
  std::string CreatedBy = "SYSTEM";
  std::string CreateDate;
  std::optional<std::string> LastUpdatedBy;
  std::optional<std::string> UpdateDate;

  // Entidades Enriquecidas Anidadas
  std::optional<omnisphere::models::Employee> employeeModel;
  std::optional<omnisphere::models::Role> role;
};

BOOST_DESCRIBE_STRUCT(UserView, (), (
    Entry, Code, Name, Email, Phone, Employee, EmployeeCode,
    RoleEntry, RoleCode, MaxDisccountPerLine, MaxDisccountPerDocument, Department,
    SuperUser, IsLocked, IsActive, IsCanceled, ChangePasswordNextLogin, PasswordNeverExpires,
    CreatedBy, CreateDate, LastUpdatedBy, UpdateDate,
    employeeModel, role
))
} // namespace omnisphere::dtos
