#include "Item.hpp" // Keep the original header for the service class
#include "DTOs/CreateItem.hpp"
#include "DTOs/GetItem.hpp"
#include "DTOs/SearchItems.hpp"
#include "Database.hpp" // Add new include for database
#include "DataTable.hpp"
#include "Repositories/Item.hpp" // Add new include for repository

namespace omnicore::service {
// Define the Impl struct for Pimpl idiom
struct Item::Impl {
  std::shared_ptr<repository::Item> item;
  explicit Impl(std::shared_ptr<service::Database> database)
      : item(std::make_shared<repository::Item>(database)) {}
};

// Update constructor to initialize pimpl
Item::Item(std::shared_ptr<service::Database> database)
    : pimpl(std::make_unique<Impl>(database)) {}

// Define destructor
Item::~Item() = default;

model::Item Item::Get(const dto::GetItem &_item) const {
  try {
    // Use pimpl to access the repository
    type::DataTable data = pimpl->item->Read(_item);

    if (data.RowsCount() == 0)
      throw std::runtime_error("Item doesn't exists");

    model::Item item(
        data[0]["ItemEntry"], data[0]["Code"], data[0]["Name"],
        data[0]["Description"].GetOptional<std::string>(),
        data[0]["Image"].GetOptional<std::string>(), data[0]["IsActive"],
        data[0]["PurchaseItem"], data[0]["SellItem"], data[0]["InventoryItem"],
        data[0]["Price"], data[0]["Brand"].GetOptional<int>(),
        data[0]["Group"].GetOptional<int>(), data[0]["OnHand"],
        data[0]["OnOrder"].GetOptional<double>(),
        data[0]["OnRequest"].GetOptional<double>(),
        data[0]["MinStock"].GetOptional<double>(),
        data[0]["MaxStock"].GetOptional<double>(),
        data[0]["MinOrder"].GetOptional<double>(),
        data[0]["MaxOrder"].GetOptional<double>(),
        data[0]["MinRequest"].GetOptional<double>(),
        data[0]["MaxRequest"].GetOptional<double>(), data[0]["CreatedBy"],
        data[0]["CreateDate"], data[0]["LastUpdatedBy"].GetOptional<int>(),
        data[0]["UpdateDate"].GetOptional<std::string>());

    return item;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetItem Exception] ") + e.what());
  }
};

std::vector<model::Item> Item::GetAll() const {
  try {
    std::vector<model::Item> items;
    // Use pimpl to access the repository and call Read (alias for ReadAll)
    type::DataTable data = pimpl->item->Read();

    for (int i = 0; i < data.RowsCount(); i++) {
      items.emplace_back(
          data[i]["ItemEntry"], data[i]["Code"], data[i]["Name"],
          data[i]["Description"].GetOptional<std::string>(),
          data[i]["Image"].GetOptional<std::string>(), data[i]["IsActive"],
          data[i]["PurchaseItem"], data[i]["SellItem"],
          data[i]["InventoryItem"], data[i]["Price"],
          data[i]["Brand"].GetOptional<int>(), data[i]["Group"].GetOptional<int>(),
          data[i]["OnHand"], data[i]["OnOrder"].GetOptional<double>(),
          data[i]["OnRequest"].GetOptional<double>(),
          data[i]["MinStock"].GetOptional<double>(),
          data[i]["MaxStock"].GetOptional<double>(),
          data[i]["MinOrder"].GetOptional<double>(),
          data[i]["MaxOrder"].GetOptional<double>(),
          data[i]["MinRequest"].GetOptional<double>(),
          data[i]["MaxRequest"].GetOptional<double>(), data[i]["CreatedBy"],
          data[i]["CreateDate"], data[i]["LastUpdatedBy"].GetOptional<int>(),
          data[i]["UpdateDate"].GetOptional<std::string>());
    }

    return items;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetAllItems Exception] ") +
                             e.what());
  }
}

std::vector<model::Item> Item::Search(dto::SearchItems &_item) const {
  try {
    std::vector<model::Item> items;
    // Use pimpl to access the repository
    type::DataTable data = pimpl->item->Read(_item);

    for (int i = 0; i < data.RowsCount(); i++) {
      items.emplace_back(
          data[i]["ItemEntry"], data[i]["Code"], data[i]["Name"],
          data[i]["Description"].GetOptional<std::string>(),
          data[i]["Image"].GetOptional<std::string>(), data[i]["IsActive"],
          data[i]["PurchaseItem"], data[i]["SellItem"],
          data[i]["InventoryItem"], data[i]["Price"],
          data[i]["Brand"].GetOptional<int>(), data[i]["Group"].GetOptional<int>(),
          data[i]["OnHand"], data[i]["OnOrder"].GetOptional<double>(),
          data[i]["OnRequest"].GetOptional<double>(),
          data[i]["MinStock"].GetOptional<double>(),
          data[i]["MaxStock"].GetOptional<double>(),
          data[i]["MinOrder"].GetOptional<double>(),
          data[i]["MaxOrder"].GetOptional<double>(),
          data[i]["MinRequest"].GetOptional<double>(),
          data[i]["MaxRequest"].GetOptional<double>(), data[i]["CreatedBy"],
          data[i]["CreateDate"], data[i]["LastUpdatedBy"].GetOptional<int>(),
          data[i]["UpdateDate"].GetOptional<std::string>());
    }

    return items;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[SearchItems Exception] ") +
                             e.what());
  }
}

model::Item Item::Add(const dto::CreateItem &_item) const {
  try {
    // Use pimpl to access the repository
    if (pimpl->item->Create(_item)) {
      // Read the newly created item using its code
      dto::GetItem getItem;
      getItem.Code = _item.Code;
      type::DataTable data = pimpl->item->Read(getItem);

      model::Item item(
          data[0]["ItemEntry"], data[0]["Code"], data[0]["Name"],
          data[0]["Description"].GetOptional<std::string>(),
          data[0]["Image"].GetOptional<std::string>(), data[0]["IsActive"],
          data[0]["PurchaseItem"], data[0]["SellItem"],
          data[0]["InventoryItem"], data[0]["Price"],
          data[0]["Brand"].GetOptional<int>(), data[0]["Group"].GetOptional<int>(),
          data[0]["OnHand"], data[0]["OnOrder"].GetOptional<double>(),
          data[0]["OnRequest"].GetOptional<double>(),
          data[0]["MinStock"].GetOptional<double>(),
          data[0]["MaxStock"].GetOptional<double>(),
          data[0]["MinOrder"].GetOptional<double>(),
          data[0]["MaxOrder"].GetOptional<double>(),
          data[0]["MinRequest"].GetOptional<double>(),
          data[0]["MaxRequest"].GetOptional<double>(), data[0]["CreatedBy"],
          data[0]["CreateDate"], data[0]["LastUpdatedBy"].GetOptional<int>(),
          data[0]["UpdateDate"].GetOptional<std::string>());

      return item;
    } else
      throw std::runtime_error("Error creating item ");
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[AddItem Exception] ") + e.what());
  }
};

model::Item Item::Modify(const dto::UpdateItem &_item) const {
  try {

    dto::GetItem getItem;
    getItem.Code = _item.Code;

    model::Item item = Get(getItem);

    return item;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[Item::Modify Exception] ") +
                             e.what());
  }
};
} // namespace omnicore::service