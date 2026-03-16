#pragma once

#include <memory>

#include "DTOs/UpdateGlobalConfiguration.hpp"
#include "Database.hpp"
#include "Models/GlobalConfiguration.hpp"

namespace omnisphere::services {

class GlobalConfiguration {
public:
  explicit GlobalConfiguration(
      std::shared_ptr<omnisphere::services::Database> database);

  ~GlobalConfiguration();

  bool Modify(const omnisphere::dtos::UpdateGlobalConfiguration &config) const;

  omnisphere::models::GlobalConfiguration Get(int confEntry) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnisphere::services
