#include "ItemGroup.hpp"

namespace omnicore::service
{
    ItemGroup::ItemGroup(std::shared_ptr<service::Database> db)
    {
        itemGroupRepository = std::make_shared<respository::ItemGroup>(db);
    }

    bool ItemGroup::Add(const dto::CreateItemGroup &createItemGroup) const
    {
        try
        {
            return itemGroupRepository->Create(createItemGroup);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[AddItemGroup Exception] ") + e.what());
        }
    }

    bool ItemGroup::Modify(const dto::UpdateItemGroup &updateItemGroup) const
    {
        try
        {
            return itemGroupRepository->Update(updateItemGroup);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[ModifyItemGroup Exception] ") + e.what());
        }
    }

    std::vector<model::ItemGroup> ItemGroup::GetAll() const
    {
        try
        {
            std::vector<model::ItemGroup> itemGroups;
            type::Datatable dataTable = itemGroupRepository->ReadAll();

            for(int i = 0; i < dataTable.RowsCount(); i++)
                itemGroups.emplace_back(model::ItemGroup{
                    dataTable[i]["Entry"],
                    dataTable[i]["Code"],
                    dataTable[i]["Name"],
                    dataTable[i]["CreatedBy"],
                    dataTable[i]["CreateDate"],
                    dataTable[i]["LastUpdatedBy"].AsDBNull<int>(),
                    dataTable[i]["UpdateDate"].AsDBNull<std::string>()
                });
            
            return itemGroups;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[GetAllItemGroups Exception] ") + e.what());
        }
    }

    model::ItemGroup ItemGroup::Get(const dto::GetItemGroup &getItemGroup) const
    {
        try
        {
            type::Datatable dataTable = itemGroupRepository->Read(getItemGroup);

            if (dataTable.RowsCount() == 0)
                throw std::runtime_error("No ItemGroups found");

            model::ItemGroup itemGroup{
                dataTable[0]["Entry"],
                dataTable[0]["Code"],
                dataTable[0]["Name"],
                dataTable[0]["CreatedBy"],
                dataTable[0]["CreateDate"],
                dataTable[0]["LastUpdatedBy"].AsDBNull<int>(),
                dataTable[0]["UpdateDate"].AsDBNull<std::string>()
            };

            return itemGroup;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[GetItemGroup Exception] ") + e.what());
        }
    }
}