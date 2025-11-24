#pragma once 

#include "Database.hpp"
#include "DTOs/CreateItemBrand.hpp"
#include "DTOs/UpdateItemBrand.hpp"
#include "DTOs/GetItemBrand.hpp"

namespace omnicore::respository 
{
    class ItemBrand 
    {
    private:
        std::shared_ptr<service::Database> database;
        int GetCurrentSequence() const;
        bool UpdateItemBrandsSequence() const;
        
    public:
        explicit ItemBrand(std::shared_ptr<service::Database> Database);
        ~ItemBrand() {};
        bool Create(const dto::CreateItemBrand &createItemBrand) const;
        bool Update(const dto::UpdateItemBrand &updateItemBrand) const;
        type::Datatable ReadAll() const;
        type::Datatable Read(const dto::GetItemBrand) const;
    };
}