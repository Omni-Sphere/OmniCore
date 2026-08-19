#pragma once
#include "GlobalConfiguration/DTOs/UpdateGlobalConfiguration.hpp"
#include "GlobalConfiguration/Models/GlobalConfiguration.hpp"
#include <OmniData/DatabasePool.hpp>
#include <memory>

namespace omnisphere::repositories {
class GlobalConfiguration {
private:
  std::shared_ptr<omnisphere::data::DatabasePool> database;

public:
  explicit GlobalConfiguration(
      std::shared_ptr<omnisphere::data::DatabasePool> database);

  ~GlobalConfiguration() = default;

  bool Update(const omnisphere::dtos::UpdateGlobalConfiguration &config) const;

  omnisphere::models::GlobalConfiguration Get(int confEntry) const;
};
} // namespace omnisphere::repositories
