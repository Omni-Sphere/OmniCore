#include "GlobalConfiguration.hpp"

namespace omnicore::service
{
    GlobalConfiguration::GlobalConfiguration(std::shared_ptr<service::Database> database)
    {
        repository = std::make_shared<repository::GlobalConfiguration>(database);
    }

    bool GlobalConfiguration::Modify(const dto::UpdateGlobalConfiguration &config) const
    {
        return repository->Update(config);
    }

    model::GlobalConfiguration GlobalConfiguration::Get(int confEntry) const
    {
        return repository->Get(confEntry);
    }
}
