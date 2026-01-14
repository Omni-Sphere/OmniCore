#include "ItemGroup.hpp"

namespace omnicore::repository {
ItemGroup::ItemGroup(std::shared_ptr<service::Database> Database)
    : database(std::move(Database)) {}

int ItemGroup::GetCurrentSequence() const {
  try {
    const std::string sQuery =
        "SELECT ISNULL(ItemGroupsSequence, 0) + 1 ItemGroupsSequence FROM "
        "Sequences WHERE SeqEntry = 1";

    type::DataTable data = database->FetchResults(sQuery);

    if (data.RowsCount() == 1)
      return data[0]["ItemGroupsSequence"];
    else
      return 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetCurrentSequence Exception] ") +
                             " " + e.what());
  }
}

bool ItemGroup::UpdateItemGroupsSequence() const {
  try {
    const std::string sQuery = "UPDATE Sequences SET ItemGroupsSequence = "
                               "ISNULL(ItemGroupsSequence,0) + 1";

    if (!database->RunStatement(sQuery))
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ItemGroupsSequence Exception] ") +
                             " " + e.what());
  }
}

bool ItemGroup::Create(const dto::CreateItemGroup &createItemGroup) const {
  try {
    std::string sQuery = "INSERT INTO ItemGroups (ItGEntry, Code, Name, "
                         "CreatedBy, CreateDate) VALUES (?, ?, ?, ?, ?);";

    std::vector<type::SQLParam> params = {
        GetCurrentSequence(), createItemGroup.Code, createItemGroup.Name,
        createItemGroup.CreatedBy, createItemGroup.CreateDate};

    if (!database->RunPrepared(sQuery, params))
      throw std::runtime_error("[RunPrepared exception]");

    if (!UpdateItemGroupsSequence())
      throw std::runtime_error("Error updating item group sequence");

    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw(std::runtime_error(std::string("[CreateItemGroup Exception]") +
                             e.what()));
  }
}

bool ItemGroup::Update(const dto::UpdateItemGroup &updateItemGroup) const {
  try {
    std::string sQuery = "UPDATE ItemGroups SET Name = ?, LastUpdatedBy = ?, "
                         "UpdateDate = ? WHERE Code = ?;";

    std::vector<type::SQLParam> params = {
        updateItemGroup.Name.value(), updateItemGroup.LastUpdatedBy,
        updateItemGroup.UpdateDate, updateItemGroup.Code};

    if (!database->RunPrepared(sQuery, params))
      throw std::runtime_error("[RunPrepared exception]");

    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw(std::runtime_error(std::string("[UpdateItemGroup Exception]") +
                             e.what()));
  }
}

type::DataTable ItemGroup::ReadAll() const {
  try {
    std::string sQuery =
        "SELECT ItGEntry [Entry], Code, Name, CreatedBy, CreateDate, "
        "LastUpdatedBy, UpdateDate FROM ItemGroups";

    type::DataTable data = database->FetchResults(sQuery);

    return {data};
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadAllItemGroups Exception] ") +
                             e.what());
  }
}

type::DataTable ItemGroup::Read(const dto::GetItemGroup itemGroup) const {
  try {
    std::string sQuery =
        "SELECT ItGEntry [Entry], Code, Name, CreatedBy, CreateDate, "
        "LastUpdatedBy, UpdateDate FROM ItemGroups WHERE ";
    std::vector<type::SQLParam> params;

    if (itemGroup.Entry.has_value()) {
      sQuery += "ItGEntry = ? ";
      params.emplace_back(type::MakeSQLParam(itemGroup.Entry.value()));
    } else if (itemGroup.Code.has_value()) {
      sQuery += "Code = ? ";
      params.emplace_back(type::MakeSQLParam(itemGroup.Code.value()));
    } else if (itemGroup.Name.has_value()) {
      sQuery += "Name = ? ";
      params.emplace_back(type::MakeSQLParam(itemGroup.Name.value()));
    } else {
      throw std::runtime_error(
          "At least one filter must be provided to read ItemGroup");
    }

    type::DataTable data = database->FetchPrepared(sQuery, params);

    return data;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadItemGroup Exception] ") +
                             e.what());
  }
}
} // namespace omnicore::repository