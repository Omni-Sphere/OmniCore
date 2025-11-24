#pragma once

#include "Database.hpp"
#include "DTOs/CreateItemBrand.hpp"
#include "DTOs/UpdateItemBrand.hpp"
#include "DTOs/GetItemBrand.hpp"
#include "Models/ItemBrand.hpp"
#include "Repositories/ItemBrand.hpp"

namespace omnicore::service
{
    /**
     * @brief Service class for managing item brands
     * 
     * This service provides methods for item brand CRUD operations.
     */
    class ItemBrand
    {
    private:
        std::shared_ptr<respository::ItemBrand> itemBrandRepository; ///< Repository instance for data access

    public:
        /**
         * @brief Constructs an ItemBrand service
         * @param db Shared pointer to the database service
         */
        explicit ItemBrand(std::shared_ptr<service::Database> db);
        
        /**
         * @brief Default destructor
         */
        ~ItemBrand() = default;
        
        /**
         * @brief Creates a new item brand
         * @param createItemBrand DTO containing item brand creation data
         * @return true if successful, false otherwise
         */
        bool Add(const dto::CreateItemBrand &createItemBrand) const;
        
        /**
         * @brief Updates an existing item brand
         * @param updateItemBrand DTO containing item brand update data
         * @return true if successful, false otherwise
         */
        bool Modify(const dto::UpdateItemBrand &updateItemBrand) const;
        
        /**
         * @brief Retrieves all item brands
         * @return Vector of all item brand models
         */
        std::vector<model::ItemBrand> GetAll() const;
        
        /**
         * @brief Retrieves a specific item brand
         * @param getItemBrand DTO containing item brand identification criteria
         * @return ItemBrand model
         */
        model::ItemBrand Get(const dto::GetItemBrand &getItemBrand) const;
    };
}