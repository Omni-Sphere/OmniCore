#include "ItemBrand.hpp"

namespace omnicore::service
{
    ItemBrand::ItemBrand(std::shared_ptr<service::Database> db)
    {
        itemBrandRepository = std::make_shared<respository::ItemBrand>(db);
    };

    bool ItemBrand::Add(const dto::CreateItemBrand &createItemBrand) const
    {
        try
        {
            return itemBrandRepository->Create(createItemBrand);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[AddItemBrand Exception] ") + e.what());
        }
    }

    bool ItemBrand::Modify(const dto::UpdateItemBrand &updateItemBrand) const
    {
        try
        {
            return itemBrandRepository->Update(updateItemBrand);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[ModifyItemBrand Exception] ") + e.what());
        }
    }  

    std::vector<model::ItemBrand> ItemBrand::GetAll() const
    {
        try
        {
            std::vector<model::ItemBrand> itemBrands;
            type::Datatable dataTable = itemBrandRepository->ReadAll();

            for(int i = 0; i < dataTable.RowsCount(); i++)
                itemBrands.emplace_back(model::ItemBrand{
                    dataTable[i]["Entry"],
                    dataTable[i]["Name"],
                    dataTable[i]["Code"],
                    dataTable[i]["CreatedBy"],
                    dataTable[i]["CreateDate"],
                    dataTable[i]["LastUpdatedBy"].AsDBNull<int>(),
                    dataTable[i]["UpdateDate"].AsDBNull<std::string>()
                });
            
            return itemBrands;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[GetAllItemBrands Exception] ") + e.what());
        }
    }

    model::ItemBrand ItemBrand::Get(const dto::GetItemBrand &getItemBrand) const
    {
        try
        {
            type::Datatable dataTable = itemBrandRepository->Read(getItemBrand);

            if (dataTable.RowsCount() == 0)
                throw std::runtime_error("No ItemBrands found");

            model::ItemBrand itemBrand{
                dataTable[0]["Entry"],
                dataTable[0]["Code"],
                dataTable[0]["Name"],
                dataTable[0]["CreatedBy"],
                dataTable[0]["CreateDate"],
                dataTable[0]["LastUpdatedBy"].AsDBNull<int>(),
                dataTable[0]["UpdateDate"].AsDBNull<std::string>()};

            return itemBrand;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[GetItemBrand Exception] ") + e.what());
        }
    }


}