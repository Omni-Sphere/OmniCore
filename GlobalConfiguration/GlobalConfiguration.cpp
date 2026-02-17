#include "GlobalConfiguration.hpp"
#include "Repositories/GlobalConfiguration.hpp"

namespace omnicore::service {

struct GlobalConfiguration::Impl {
  std::shared_ptr<repository::GlobalConfiguration> repository;
  explicit Impl(std::shared_ptr<service::Database> database)
      : repository(
            std::make_shared<repository::GlobalConfiguration>(database)) {}
};

GlobalConfiguration::GlobalConfiguration(
    std::shared_ptr<service::Database> database)
    : pimpl(std::make_unique<Impl>(database)) {}

GlobalConfiguration::~GlobalConfiguration() = default;

bool GlobalConfiguration::Modify(
    const dto::UpdateGlobalConfiguration &config) const {
  return pimpl->repository->Update(config);
}

model::GlobalConfiguration GlobalConfiguration::Get(int confEntry) const {
  return pimpl->repository->Get(confEntry);
}
} // namespace omnicore::service
