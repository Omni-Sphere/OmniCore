#pragma once
#include <optional>
#include <string>


namespace omnisphere::omnicore::dtos {
struct UserCondition {
  std::optional<int> Entry;
  std::optional<std::string> Code;
};

struct UserData {
  std::optional<std::string> Name;
  std::optional<std::string> Email;
  std::optional<std::string> Phone;
  std::optional<int> Employee;
};

struct UpdateUser {
  UserCondition Where;
  UserData Data;
  std::string UpdateDate;
  int UpdatedBy;
};
} // namespace omnisphere::omnicore::dtos