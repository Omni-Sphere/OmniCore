#pragma once

#include <memory>

namespace omnicore::service {
class Database;
}

namespace omnicore::dto {
class UpdateGlobalConfiguration;
}

namespace omnicore::model {
class GlobalConfiguration;
}

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
