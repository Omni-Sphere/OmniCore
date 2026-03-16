#pragma once

#include <string>

namespace omnisphere::models {
class Payload {
public:
  uint64_t ExpiresAt;
  uint64_t IssuedAt;
  std::string SessionUUID;
};
} // namespace omnisphere::models