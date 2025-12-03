#include "ItemBrand.hpp"

namespace omnicore::respository {
ItemBrand::ItemBrand(std::shared_ptr<service::Database> _database)
    : database(std::move(_database)) {}

int ItemBrand::GetCurrentSequence() const {
  try {
    const std::string sQuery =
        "SELECT ISNULL(ItemBrandsSequence, 0) + 1 ItemBrandsSequence FROM "
        "Sequences WHERE SeqEntry = 1";

    type::Datatable data = database->FetchResults(sQuery);

    if (data.RowsCount() == 1)
      return data[0]["ItemBrandsSequence"];
    else
      return 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetCurrentSequence Exception] ") +
                             " " + e.what());
  }
}

bool ItemBrand::UpdateItemBrandsSequence() const {
  try {
    const std::string sQuery = "UPDATE Sequences SET ItemBrandsSequence = "
                               "ISNULL(ItemBrandsSequence,0) + 1";

    if (!database->RunStatement(sQuery))
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ItemBrandsSequence Exception] ") +
                             " " + e.what());
  }
}

bool ItemBrand::Create(const dto::CreateItemBrand &createItemBrand) const {
  try {
    std::string sQuery = "INSERT INTO ItemBrands (ItBEntry, Code, Name, "
                         "CreatedBy, CreateDate) VALUES (?, ?, ?, ?, ?);";

    std::vector<type::SQLParam> params = {
        GetCurrentSequence(), createItemBrand.Code, createItemBrand.Name,
        createItemBrand.CreatedBy, createItemBrand.CreateDate};

    if (!database->RunPrepared(sQuery, params))
      throw std::runtime_error("[RunPrepared exception]");

    if (!UpdateItemBrandsSequence())
      throw std::runtime_error("Error updating ItemBrands sequence");

    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw(std::runtime_error(std::string("[CreateItemBrand Exception]") +
                             e.what()));
  }
}

bool ItemBrand::Update(const dto::UpdateItemBrand &updateItemBrand) const {
  try {
    std::string sQuery = "UPDATE ItemBrands SET Name = ?, LastUpdatedBy = ?, "
                         "UpdateDate = ? WHERE Code = ?;";

    std::vector<type::SQLParam> params = {
        updateItemBrand.Name.value(), updateItemBrand.LastUpdatedBy,
        updateItemBrand.UpdateDate, updateItemBrand.Code};

    if (!database->RunPrepared(sQuery, params))
      throw std::runtime_error("[RunPrepared exception]");

    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw(std::runtime_error(std::string("[UpdateItemBrand Exception]") +
                             e.what()));
  }
}

type::Datatable ItemBrand::ReadAll() const {
  try {
    std::string sQuery = "SELECT "
                         "ItBEntry [Entry], "
                         "[Code], "
                         "[Name], "
                         "CreatedBy, "
                         "CreateDate, "
                         "LastUpdatedBy, "
                         "UpdateDate "
                         "FROM ItemBrands";

    type::Datatable dataTable = database->FetchResults(sQuery);

    return dataTable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadAllItemBrands Exception] ") +
                             " " + e.what());
  }
}

type::Datatable ItemBrand::Read(const dto::GetItemBrand itemBrand) const {
  try {
    std::string sQuery = "SELECT "
                         "ItBEntry [Entry], "
                         "[Code], "
                         "[Name], "
                         "CreatedBy, "
                         "CreateDate, "
                         "LastUpdatedBy, "
                         "UpdateDate "
                         "FROM ItemBrands WHERE ";

    std::vector<type::SQLParam> parameters;

    if (itemBrand.Entry.has_value()) {
      sQuery += "ItBEntry = ?";
      parameters.emplace_back(type::MakeSQLParam(itemBrand.Entry.value()));
    }

    if (itemBrand.Code.has_value()) {
      sQuery += "Code = ?";
      parameters.emplace_back(type::MakeSQLParam(itemBrand.Code.value()));
    }

    if (itemBrand.Name.has_value()) {
      sQuery += "Name = ?";
      parameters.emplace_back(type::MakeSQLParam(itemBrand.Name.value()));
    }

    type::Datatable dataTable = database->FetchPrepared(sQuery, parameters);

    return dataTable;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadItemBrand Exception] ") + " " +
                             e.what());
  }
}
} // namespace omnicore::respository