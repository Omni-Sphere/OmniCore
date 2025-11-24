#include "Item.hpp"

namespace omnicore::service
{
    Item::Item(std::shared_ptr<service::Database> _database)
    {
        item = std::make_shared<repository::Item>(_database);
    };

    Item::~Item() = default;

    model::Item Item::Get(const dto::GetItem &_item) const
    {
        try
        {
            type::Datatable dataTable = item->Read(_item);

            if (dataTable.RowsCount() == 0)
                throw std::runtime_error("No items found");

            if (dataTable.RowsCount() > 1)
                throw std::runtime_error("Inconstence retreiving Items, more than one found");

            return model::Item{
                dataTable[0]["ItemEntry"],
                dataTable[0]["Code"],
                dataTable[0]["Name"],
                dataTable[0]["Description"].AsDBNull<std::string>(),
                dataTable[0]["Image"].AsDBNull<std::string>(),
                dataTable[0]["IsActive"],
                dataTable[0]["PurchaseItem"],
                dataTable[0]["SellItem"],
                dataTable[0]["InventoryItem"],
                dataTable[0]["Price"],
                dataTable[0]["Brand"].AsDBNull<int>(),
                dataTable[0]["Group"].AsDBNull<int>(),
                dataTable[0]["OnHand"],
                dataTable[0]["OnOrder"].AsDBNull<double>(),
                dataTable[0]["OnRequest"].AsDBNull<double>(),
                dataTable[0]["MinStock"].AsDBNull<double>(),
                dataTable[0]["MaxStock"].AsDBNull<double>(),
                dataTable[0]["MinOrder"].AsDBNull<double>(),
                dataTable[0]["MaxOrder"].AsDBNull<double>(),
                dataTable[0]["MinRequest"].AsDBNull<double>(),
                dataTable[0]["MaxRequest"].AsDBNull<double>(),
                dataTable[0]["CreatedBy"],
                dataTable[0]["CreateDate"],
                dataTable[0]["LastUpdatedBy"].AsDBNull<int>(),
                dataTable[0]["UpdateDate"].AsDBNull<std::string>()};
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[Item::Get Exception] ") + e.what());
        }
    };

    std::vector<model::Item> Item::GetAll() const
    {
        try
        {
            type::Datatable dataTable = item->Read();
            std::vector<model::Item> items;

            if (dataTable.RowsCount() == 0)
                return items;

            for (size_t i = 0; i < dataTable.RowsCount(); ++i)
            {
                items.push_back(model::Item{
                    dataTable[i]["ItemEntry"],
                    dataTable[i]["Code"],
                    dataTable[i]["Name"],
                    dataTable[i]["Description"].AsDBNull<std::string>(),
                    dataTable[i]["Image"].AsDBNull<std::string>(),
                    dataTable[i]["IsActive"],
                    dataTable[i]["PurchaseItem"],
                    dataTable[i]["SellItem"],
                    dataTable[i]["InventoryItem"],
                    dataTable[i]["Price"],
                    dataTable[i]["Brand"].AsDBNull<int>(),
                    dataTable[i]["Group"].AsDBNull<int>(),
                    dataTable[i]["OnHand"],
                    dataTable[i]["OnOrder"],
                    dataTable[i]["OnRequest"].AsDBNull<double>(),
                    dataTable[i]["MinStock"].AsDBNull<double>(),
                    dataTable[i]["MaxStock"].AsDBNull<double>(),
                    dataTable[i]["MinOrder"].AsDBNull<double>(),
                    dataTable[i]["MaxOrder"].AsDBNull<double>(),
                    dataTable[i]["MinRequest"].AsDBNull<double>(),
                    dataTable[i]["MaxRequest"].AsDBNull<double>(),
                    dataTable[i]["CreatedBy"],
                    dataTable[i]["CreateDate"],
                    dataTable[i]["LastUpdatedBy"].AsDBNull<int>(),
                    dataTable[i]["UpdateDate"].AsDBNull<std::string>()});
            }

            return items;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[Item::GetAll Exception] ") + e.what());
        }
    }

    std::vector<model::Item> Item::Search(dto::SearchItems &_item) const
    {
        try
        {
            std::vector<model::Item> items;
            return items;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[Item::Search Exception] ") + e.what());
        }
    }

    model::Item Item::Add(const dto::CreateItem &_item) const
    {
        try
        {
            if (!item->Create(_item))
                throw std::runtime_error("Error creating Item");

            dto::GetItem getItem;
            getItem.Code = _item.Code;

            return Get(getItem);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[Item::Add Exception] ") + e.what());
        }
    };

    model::Item Item::Modify(const dto::UpdateItem &_item) const
    {
        try
        {

            if (!this->item->Update(_item))
                throw std::runtime_error("Error updating Item");

            dto::GetItem getItem;
            getItem.Code = _item.Code;

            model::Item item = Get(getItem);

            return item;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[Item::Modify Exception] ") + e.what());
        }
    };
}