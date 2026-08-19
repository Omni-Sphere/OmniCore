#include "GlobalConfiguration/GlobalConfiguration.hpp"
#include "GlobalConfiguration/Repositories/GlobalConfiguration.hpp"

namespace omnisphere::services {
struct GlobalConfiguration::Impl {
  std::shared_ptr<omnisphere::repositories::GlobalConfiguration> repository;
  explicit Impl(std::shared_ptr<omnisphere::data::DatabasePool> database)
      : repository(
            std::make_shared<omnisphere::repositories::GlobalConfiguration>(
                database)) {}
};

GlobalConfiguration::GlobalConfiguration(
    std::shared_ptr<omnisphere::data::DatabasePool> database)
    : pimpl(std::make_unique<Impl>(database)) {}

GlobalConfiguration::~GlobalConfiguration() = default;

bool GlobalConfiguration::Modify(
    const omnisphere::dtos::UpdateGlobalConfiguration &config) const {
  return pimpl->repository->Update(config);
}

omnisphere::models::GlobalConfiguration
GlobalConfiguration::Get(int confEntry) const {
  return pimpl->repository->Get(confEntry);
}
} // namespace omnisphere::services
