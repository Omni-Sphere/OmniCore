#pragma once
#include "DTOs/UpdateGlobalConfiguration.hpp"
#include "Database.hpp"
#include "Models/GlobalConfiguration.hpp"
#include <memory>

namespace omnisphere::omnicore::repositories {

class GlobalConfiguration {
private:
  std::shared_ptr<omnidata::services::Database> database;

public:
  explicit GlobalConfiguration(
      std::shared_ptr<omnidata::services::Database> database);

  ~GlobalConfiguration() = default;

  bool Update(const omnisphere::omnicore::dtos::UpdateGlobalConfiguration
                  &config) const;

  omnisphere::omnicore::models::GlobalConfiguration Get(int confEntry) const;
};
} // namespace omnisphere::omnicore::repositories
