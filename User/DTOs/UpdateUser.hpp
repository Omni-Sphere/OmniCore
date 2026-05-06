#include <Database.hpp>
#include <DataTable.hpp>
#pragma once
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
  std::optional<double> MaxDiscountItem;
  std::optional<double> MaxDiscountGeneral;
  std::optional<std::string> PermissionMode;
  std::optional<int> Department;
};

struct UpdateUser {
  UserCondition Where;
  UserData Data;
  std::string UpdateDate;
  int UpdatedBy;
};
} // namespace omnisphere::dtos