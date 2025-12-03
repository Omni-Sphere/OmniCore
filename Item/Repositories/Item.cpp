#include "Item.hpp"

namespace omnicore::repository {
Item::Item(std::shared_ptr<service::Database> database)
    : Database(std::move(database)) {}

Item::~Item() = default;

bool Item::Create(const dto::CreateItem &item) const {
  try {
    std::string sQuery =
        "INSERT INTO Items ("
        "[ItemEntry], "
        "[Code], "
        "[Name], "
        "Description, "
        "Image, "
        "PurchaseItem, "
        "SellItem, "
        "InventoryItem, "
        "Price, "
        "BrandEntry, "
        "GroupEntry, "
        "MinStock, "
        "MaxStock, "
        "MinOrder, "
        "MaxOrder, "
        "MinRequest, "
        "MaxRequest, "
        "CreatedBy, "
        "CreateDate"
        ")"
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    const std::vector<type::SQLParam> params = {
        type::MakeSQLParam(GetCurrentSequence()),
        type::MakeSQLParam(item.Code),
        type::MakeSQLParam(item.Name),
        type::MakeSQLParam(item.Description),
        type::MakeSQLParam(item.Image),
        type::MakeSQLParam(item.PurchaseItem),
        type::MakeSQLParam(item.SellItem),
        type::MakeSQLParam(item.InventoryItem),
        type::MakeSQLParam(item.Price),
        type::MakeSQLParam(item.Brand),
        type::MakeSQLParam(item.Group),
        type::MakeSQLParam(item.MinStock),
        type::MakeSQLParam(item.MaxStock),
        type::MakeSQLParam(item.MinOrder),
        type::MakeSQLParam(item.MaxOrder),
        type::MakeSQLParam(item.MinRequest),
        type::MakeSQLParam(item.MaxRequest),
        type::MakeSQLParam(item.CreatedBy),
        type::MakeSQLParam(item.CreateDate)};

    if (!Database->RunPrepared(sQuery, params))
      throw std::runtime_error("Error creating item");

    UpdateUserSequence();

    return true;
  } catch (const std::exception &e) {
    Database->RollbackTransaction();
    throw std::runtime_error(std::string("[Create Exception]: ") + e.what());
  }
}

bool Item::Update(const dto::UpdateItem &item) const {
  try {
    std::string sQuery = "UPDATE Items SET ";
    std::vector<type::SQLParam> params;

    if (item.Name.has_value()) {
      sQuery += "Name = ?, ";
      params.emplace_back(type::MakeSQLParam(item.Name.value()));
    }

    if (item.Description.has_value()) {
      sQuery += "Description = ?, ";
      params.emplace_back(type::MakeSQLParam(item.Description.value()));
    }

    if (item.Image.has_value()) {
      sQuery += "Image = ?, ";
      params.emplace_back(type::MakeSQLParam(item.Image.value()));
    }

    if (item.PurchaseItem.has_value()) {
      sQuery += "PurchaseItem = ?, ";
      params.emplace_back(type::MakeSQLParam(item.PurchaseItem.value()));
    }

    if (item.SellItem.has_value()) {
      sQuery += "SellItem = ?, ";
      params.emplace_back(type::MakeSQLParam(item.SellItem.value()));
    }

    if (item.InventoryItem.has_value()) {
      sQuery += "InventoryItem = ?, ";
      params.emplace_back(type::MakeSQLParam(item.InventoryItem.value()));
    }

    if (item.Price.has_value()) {
      sQuery += "Price = ?, ";
      params.emplace_back(type::MakeSQLParam(item.Price.value()));
    }

    if (item.Brand.has_value()) {
      sQuery += "Brand = ?, ";
      params.emplace_back(type::MakeSQLParam(item.Brand.value()));
    }

    if (item.Group.has_value()) {
      sQuery += "Group = ?, ";
      params.emplace_back(type::MakeSQLParam(item.Group.value()));
    }

    sQuery += "WHERE ItemEntry = ?";

    if (!Database->RunPrepared(sQuery, params))
      throw std::runtime_error("Error updating item");

    return true;
  } catch (const std::exception &e) {
    Database->RollbackTransaction();
    throw std::runtime_error(std::string("[Update Exception]: ") + e.what());
  }
}

type::Datatable Item::Read(const dto::SearchItems &filter) const {
  try {
    type::Datatable datatable;

    std::string sQuery = "SELECT "
                         "[ItemEntry], "
                         "[Code], "
                         "[Name], "
                         "Description, "
                         "Image, "
                         "IsActive, "
                         "PurchaseItem, "
                         "SellItem, "
                         "InventoryItem, "
                         "Price, "
                         "BrandEntry Brand, "
                         "GroupEntry [Group],"
                         "OnHand, "
                         "OnOrder, "
                         "OnRequest, "
                         "MinStock, "
                         "MaxStock, "
                         "MinOrder, "
                         "MaxOrder, "
                         "MinRequest, "
                         "MaxRequest, "
                         "CreatedBy, "
                         "CreateDate, "
                         "LastUpdatedBy, "
                         "UpdateDate "
                         "FROM Items WHERE ";

    const std::vector<type::SQLParam> params = {
        type::MakeSQLParam(filter.CodeEqualsTo),
        type::MakeSQLParam(filter.CodeContains),
        type::MakeSQLParam(filter.NameEqualsTo),
        type::MakeSQLParam(filter.NameContains),
        type::MakeSQLParam(filter.DescriptionEqualsTo),
        type::MakeSQLParam(filter.DescriptionContains),
        type::MakeSQLParam(filter.IsActive),
        type::MakeSQLParam(filter.PurchaseItem),
        type::MakeSQLParam(filter.SellItem),
        type::MakeSQLParam(filter.InventoryItem),
        type::MakeSQLParam(filter.Price),
        type::MakeSQLParam(filter.OnHandEqualsTo),
        type::MakeSQLParam(filter.OnHandGreaterThan),
        type::MakeSQLParam(filter.OnHandLessThan),
        type::MakeSQLParam(filter.OnRequestEqualsTo),
        type::MakeSQLParam(filter.OnRequestGreaterThan),
        type::MakeSQLParam(filter.OnRequestLessThan),
        type::MakeSQLParam(filter.OnOrderEqualsTo),
        type::MakeSQLParam(filter.OnOrderGreaterThan),
        type::MakeSQLParam(filter.OnOrderLessThan),
        type::MakeSQLParam(filter.MinStockEqualsTo),
        type::MakeSQLParam(filter.MinStockGreaterThan),
        type::MakeSQLParam(filter.MinStockLessThan),
        type::MakeSQLParam(filter.MaxStockEqualsTo),
        type::MakeSQLParam(filter.MaxStockGreaterThan),
        type::MakeSQLParam(filter.MaxStockLessThan),
        type::MakeSQLParam(filter.MinOrderEqualsTo),
        type::MakeSQLParam(filter.MinOrderGreaterThan),
        type::MakeSQLParam(filter.MinOrderLessThan),
        type::MakeSQLParam(filter.MaxOrderEqualsTo),
        type::MakeSQLParam(filter.MaxOrderGreaterThan),
        type::MakeSQLParam(filter.MaxOrderLessThan),
        type::MakeSQLParam(filter.MinRequestEqualsTo),
        type::MakeSQLParam(filter.MinRequestGreaterThan),
        type::MakeSQLParam(filter.MinRequestLessThan),
        type::MakeSQLParam(filter.MaxReqeuestEqualsTo),
        type::MakeSQLParam(filter.MaxRequestGreaterThan),
        type::MakeSQLParam(filter.MaxRequestLessThan)};

    return datatable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[Read Exception]: ") + e.what());
  }
}

type::Datatable Item::Read(const dto::GetItem &filter) const {
  try {
    type::Datatable datatable;

    std::string sQuery = "SELECT "
                         "[ItemEntry], "
                         "[Code], "
                         "[Name], "
                         "Description, "
                         "Image, "
                         "IsActive, "
                         "'N' StockByWarehouse, "
                         "PurchaseItem, "
                         "SellItem, "
                         "InventoryItem, "
                         "Price, "
                         "BrandEntry Brand, "
                         "GroupEntry [Group], "
                         "OnHand, "
                         "OnOrder, "
                         "OnRequest, "
                         "MinStock, "
                         "MaxStock, "
                         "MinOrder, "
                         "MaxOrder, "
                         "MinRequest, "
                         "MaxRequest, "
                         "CreatedBy, "
                         "CreateDate, "
                         "LastUpdatedBy, "
                         "UpdateDate "
                         "FROM Items WHERE ";

    if (filter.Code.has_value()) {
      sQuery += "[Code] = ?";
      datatable = Database->FetchPrepared(sQuery, filter.Code.value());
    }

    if (filter.Name.has_value()) {
      sQuery += "[Name] = ?";
      datatable = Database->FetchPrepared(sQuery, filter.Name.value());
    }

    return datatable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[Read Exception]: ") + e.what());
  }
}

type::Datatable Item::Read() const {
  try {
    type::Datatable datatable;

    std::string sQuery = "SELECT "
                         "[ItemEntry], "
                         "[Code], "
                         "[Name], "
                         "Description, "
                         "Image, "
                         "IsActive, "
                         "PurchaseItem, "
                         "SellItem, "
                         "InventoryItem, "
                         "Price, "
                         "BrandEntry Brand, "
                         "GroupEntry [Group], "
                         "OnHand, "
                         "OnOrder, "
                         "OnRequest, "
                         "MinStock, "
                         "MaxStock, "
                         "MinOrder, "
                         "MaxOrder, "
                         "MinRequest, "
                         "MaxRequest, "
                         "CreatedBy, "
                         "CreateDate, "
                         "LastUpdatedBy, "
                         "UpdateDate "
                         "FROM Items";
    datatable = Database->FetchResults(sQuery);

    return datatable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[Read Exception]: ") + e.what());
  }
}

int Item::GetCurrentSequence() const {
  try {
    const std::string sQuery = "SELECT ISNULL(ItemSequence, 0) + 1 "
                               "ItemSequence FROM Sequences WHERE SeqEntry = 1";

    type::Datatable data = Database->FetchResults(sQuery);

    if (data.RowsCount() == 1)
      return data[0]["ItemSequence"];
    else
      return 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetCurrentSequence Exception] ") +
                             " " + e.what());
  }
};

bool Item::UpdateUserSequence() const {
  try {
    const std::string sQuery =
        "UPDATE Sequences SET ItemSequence = ISNULL(ItemSequence,0) + 1";

    if (!Database->RunStatement(sQuery))
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[UpdateUserSequence Exception] ") +
                             " " + e.what());
  }
};
}; // namespace omnicore::repository