#pragma once

#include "Repositories/ItemGroup.hpp"
#include "Models/ItemGroup.hpp"

namespace omnicore::service
{
    /**
     * @brief Service class for managing item groups
     * 
     * This service provides methods for item group CRUD operations.
     */
    class ItemGroup
    {
    private:
        std::shared_ptr<respository::ItemGroup> itemGroupRepository; ///< Repository instance for data access

    public:
        /**
         * @brief Constructs an ItemGroup service
         * @param db Shared pointer to the database service
         */
        explicit ItemGroup(std::shared_ptr<service::Database> db);
        
        /**
         * @brief Default destructor
         */
        ~ItemGroup() = default;

        /**
         * @brief Creates a new item group
         * @param createItemGroup DTO containing item group creation data
         * @return true if successful, false otherwise
         */
        bool Add(const dto::CreateItemGroup &createItemGroup) const;
        
        /**
         * @brief Updates an existing item group
         * @param updateItemGroup DTO containing item group update data
         * @return true if successful, false otherwise
         */
        bool Modify(const dto::UpdateItemGroup &updateItemGroup) const;
        
        /**
         * @brief Retrieves all item groups
         * @return Vector of all item group models
         */
        std::vector<model::ItemGroup> GetAll() const;
        
        /**
         * @brief Retrieves a specific item group
         * @param getItemGroup DTO containing item group identification criteria
         * @return ItemGroup model
         */
        model::ItemGroup Get(const dto::GetItemGroup &getItemGroup) const;
    };
}