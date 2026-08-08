#pragma once

#include <memory>

#include "GlobalConfiguration/DTOs/UpdateGlobalConfiguration.hpp"
#include "GlobalConfiguration/Models/GlobalConfiguration.hpp"
#include <OmniData/Database.hpp>

namespace omnisphere::services {
class GlobalConfiguration {
public:
  explicit GlobalConfiguration(
      std::shared_ptr<omnisphere::data::Database> database);

  ~GlobalConfiguration();

  bool Modify(const omnisphere::dtos::UpdateGlobalConfiguration &config) const;

  omnisphere::models::GlobalConfiguration Get(int confEntry) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnisphere::services
