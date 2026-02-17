#pragma once
#include "DTOs/UpdateGlobalConfiguration.hpp"
#include "Database.hpp"
#include "Models/GlobalConfiguration.hpp"
#include <memory>


namespace omnicore::repository {

class GlobalConfiguration {
private:
  std::shared_ptr<service::Database> database;

public:
  explicit GlobalConfiguration(std::shared_ptr<service::Database> database);

  ~GlobalConfiguration() = default;

  bool Update(const dto::UpdateGlobalConfiguration &config) const;

  model::GlobalConfiguration Get(int confEntry) const;
};
} // namespace omnicore::repository
