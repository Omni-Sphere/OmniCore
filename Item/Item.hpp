#pragma once

#include "Database.hpp"

#include "Repositories/Item.hpp"

#include "Models/Item.hpp"
#include "DTOs/SearchItems.hpp"
#include "DTOs/CreateItem.hpp"
#include "DTOs/UpdateItem.hpp"
#include "DTOs/GetItem.hpp"

namespace omnicore::service
{
    /**
     * @brief Service class for managing items
     * 
     * This service provides methods for item CRUD operations and search.
     */
    class Item
    {
    public:
        /**
         * @brief Constructs an Item service
         * @param database Shared pointer to the database service
         */
        explicit Item(std::shared_ptr<service::Database> database);
        
        /**
         * @brief Destructor
         */
        ~Item();

        /**
         * @brief Retrieves a specific item
         * @param _item DTO containing item identification criteria
         * @return Item model
         */
        model::Item Get(const dto::GetItem &_item) const;
        
        /**
         * @brief Retrieves all items
         * @return Vector of all item models
         */
        std::vector<model::Item> GetAll() const;
        
        /**
         * @brief Searches for items based on criteria
         * @param _item DTO containing search criteria
         * @return Vector of item models matching the criteria
         */
        std::vector<model::Item> Search(dto::SearchItems &_item) const;
        
        /**
         * @brief Creates a new item
         * @param _item DTO containing item creation data
         * @return Created item model
         */
        model::Item Add(const dto::CreateItem &_item) const;
        
        /**
         * @brief Updates an existing item
         * @param _item DTO containing item update data
         * @return Updated item model
         */
        model::Item Modify(const dto::UpdateItem &_item) const;

    private:
        std::shared_ptr<repository::Item> item; ///< Repository instance for data access
    };
}