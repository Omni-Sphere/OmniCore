#pragma once
#include <Database.hpp>
#include <DataTable.hpp>
#include <User/Enums/PermissionMode.hpp>
#include <optional>
#include <string>

namespace omnisphere::dtos {
struct UserCondition {
  std::optional<int> Entry;
  std::optional<std::string> Code;
};

struct UserData {
  std::optional<std::string> Name;
  std::optional<std::string> Email;
  std::optional<std::string> Phone;
  std::optional<int> Employee;
  std::optional<int> RoleEntry;
  std::optional<double> MaxDisccountPerLine;
  std::optional<double> MaxDisccountPerDocument;
  std::optional<omnisphere::enums::PermissionMode> PermissionMode;
  std::optional<int> Department;
};

struct UpdateUser {
  UserCondition Where;
  UserData Data;
  std::string UpdateDate;
  int UpdatedBy;
};
} // namespace omnisphere::dtos