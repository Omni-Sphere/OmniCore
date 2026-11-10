#pragma once

#include <string>

namespace omnisphere::dtos {
struct GetCompany {
  int Entry = 1;
  std::string Code = "";
  std::string Name = "";
};
} // namespace omnisphere::dtos

