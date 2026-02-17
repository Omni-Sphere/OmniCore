#pragma once

#include <memory>

#include "DTOs/UpdateGlobalConfiguration.hpp"
#include "Database.hpp"
#include "Models/GlobalConfiguration.hpp"

namespace omnicore::service {

class GlobalConfiguration {
public:
  explicit GlobalConfiguration(std::shared_ptr<service::Database> database);

  ~GlobalConfiguration();

  bool Modify(const dto::UpdateGlobalConfiguration &config) const;

  model::GlobalConfiguration Get(int confEntry) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnicore::service
