#pragma once
#include "GlobalConfiguration/DTOs/UpdateGlobalConfiguration.hpp"
#include "GlobalConfiguration/Models/GlobalConfiguration.hpp"
#include <OmniData/Database.hpp>
#include <memory>

namespace omnisphere::repositories {
class GlobalConfiguration {
private:
  std::shared_ptr<omnisphere::data::Database> database;

public:
  explicit GlobalConfiguration(
      std::shared_ptr<omnisphere::data::Database> database);

  ~GlobalConfiguration() = default;

  bool Update(const omnisphere::dtos::UpdateGlobalConfiguration &config) const;

  omnisphere::models::GlobalConfiguration Get(int confEntry) const;
};
} // namespace omnisphere::repositories
