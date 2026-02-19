#pragma once

#include <string>

namespace omnisphere::omnicore::models {
class Payload {
public:
  uint64_t ExpiresAt;
  uint64_t IssuedAt;
  std::string SessionUUID;
};
} // namespace omnisphere::omnicore::models