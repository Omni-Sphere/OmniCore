#pragma once
#include "DTOs/UpdateGlobalConfiguration.hpp"
#include "Database.hpp"
#include "Models/GlobalConfiguration.hpp"
#include <memory>

namespace omnisphere::repositories {

class GlobalConfiguration {
private:
  std::shared_ptr<omnisphere::services::Database> database;

public:
  explicit GlobalConfiguration(
      std::shared_ptr<omnisphere::services::Database> database);

  ~GlobalConfiguration() = default;

  bool Update(const omnisphere::dtos::UpdateGlobalConfiguration &config) const;

  omnisphere::models::GlobalConfiguration Get(int confEntry) const;
};
} // namespace omnisphere::repositories
