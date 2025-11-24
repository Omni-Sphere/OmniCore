#pragma once
#include <memory>
#include "Database.hpp"
#include "Models/GlobalConfiguration.hpp"
#include "DTOs/UpdateGlobalConfiguration.hpp"

namespace omnicore::repository
{
    /**
     * @brief Repository class for GlobalConfiguration data access
     * 
     * This repository handles all database operations related to the
     * GlobalConfiguration table.
     */
    class GlobalConfiguration
    {
    private:
        std::shared_ptr<service::Database> database; ///< Database service instance

    public:
        /**
         * @brief Constructs a GlobalConfiguration repository
         * @param database Shared pointer to the database service
         */
        explicit GlobalConfiguration(std::shared_ptr<service::Database> database);
        
        /**
         * @brief Default destructor
         */
        ~GlobalConfiguration() = default;

        /**
         * @brief Updates global configuration in the database
         * @param config DTO containing the configuration fields to update
         * @return true if the update was successful, false otherwise
         * @throws std::runtime_error if update fails
         */
        bool Update(const dto::UpdateGlobalConfiguration &config) const;
        
        /**
         * @brief Retrieves global configuration from the database
         * @param confEntry The configuration entry ID to retrieve
         * @return GlobalConfiguration model with the configuration data
         * @throws std::runtime_error if configuration not found
         */
        model::GlobalConfiguration Get(int confEntry) const;
    };
}
