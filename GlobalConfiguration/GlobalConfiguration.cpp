#include "GlobalConfiguration.hpp"
#include "Repositories/GlobalConfiguration.hpp"

namespace omnisphere::omnicore::services {

struct GlobalConfiguration::Impl {
  std::shared_ptr<omnisphere::omnicore::repositories::GlobalConfiguration>
      repository;
  explicit Impl(
      std::shared_ptr<omnisphere::omnidata::services::Database> database)
      : repository(std::make_shared<
                   omnisphere::omnicore::repositories::GlobalConfiguration>(
            database)) {}
};

GlobalConfiguration::GlobalConfiguration(
    std::shared_ptr<omnisphere::omnidata::services::Database> database)
    : pimpl(std::make_unique<Impl>(database)) {}

GlobalConfiguration::~GlobalConfiguration() = default;

bool GlobalConfiguration::Modify(
    const omnisphere::omnicore::dtos::UpdateGlobalConfiguration &config) const {
  return pimpl->repository->Update(config);
}

omnisphere::omnicore::models::GlobalConfiguration
GlobalConfiguration::Get(int confEntry) const {
  return pimpl->repository->Get(confEntry);
}
} // namespace omnisphere::omnicore::services
