#pragma once

#include "Database.hpp"
#include "Repositories/GlobalConfiguration.hpp"
#include "DTOs/UpdateGlobalConfiguration.hpp"
#include "Models/GlobalConfiguration.hpp"

namespace omnicore::service
{
    /**
     * @brief Service class for managing global configuration settings
     * 
     * This service provides methods to read and update global configuration
     * settings stored in the GlobalConfiguration table.
     */
    class GlobalConfiguration {
    public: 
        /**
         * @brief Constructs a GlobalConfiguration service
         * @param database Shared pointer to the database service
         */
        explicit GlobalConfiguration(std::shared_ptr<service::Database> database);
        
        /**
         * @brief Default destructor
         */
        ~GlobalConfiguration() = default; 
        
        /**
         * @brief Updates global configuration settings
         * @param config DTO containing the configuration fields to update
         * @return true if the update was successful, false otherwise
         */
        bool Modify(const dto::UpdateGlobalConfiguration &config) const;
        
        /**
         * @brief Retrieves global configuration by entry ID
         * @param confEntry The configuration entry ID to retrieve
         * @return GlobalConfiguration model with the configuration data
         * @throws std::runtime_error if configuration not found
         */
        model::GlobalConfiguration Get(int confEntry) const;

    private:
        std::shared_ptr<repository::GlobalConfiguration> repository; ///< Repository instance for data access
    };
}
