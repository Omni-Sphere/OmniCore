#pragma once

#include <memory>

#include "DTOs/UpdateGlobalConfiguration.hpp"
#include "Database.hpp"
#include "Models/GlobalConfiguration.hpp"

namespace omnisphere::omnicore::services {

class GlobalConfiguration {
public:
  explicit GlobalConfiguration(
      std::shared_ptr<omnidata::services::Database> database);

  ~GlobalConfiguration();

  bool Modify(const omnisphere::omnicore::dtos::UpdateGlobalConfiguration
                  &config) const;

  omnisphere::omnicore::models::GlobalConfiguration Get(int confEntry) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnisphere::omnicore::services
