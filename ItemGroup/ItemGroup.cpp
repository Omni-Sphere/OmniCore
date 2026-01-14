#include "ItemGroup.hpp"
#include "Database.hpp"
#include "DataTable.hpp"
#include "Models/ItemBrand.hpp" // Added based on instruction
#include "Models/ItemGroup.hpp"
#include "Models/ItemGroup.hpp" // Added based on provided snippet, though it's a duplicate
#include "Repositories/ItemGroup.hpp"

namespace omnicore::service {

struct ItemGroup::Impl {
  std::shared_ptr<repository::ItemGroup> itemGroupRepository;
  explicit Impl(std::shared_ptr<service::Database> db)
      : itemGroupRepository(std::make_shared<repository::ItemGroup>(db)) {}
};

ItemGroup::ItemGroup(std::shared_ptr<service::Database> db)
    : pimpl(std::make_unique<Impl>(db)) {}

ItemGroup::~ItemGroup() = default;

bool ItemGroup::Add(const dto::CreateItemGroup &createItemGroup) const {
  try {
    return pimpl->itemGroupRepository->Create(createItemGroup);
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[AddItemGroup Exception] ") +
                             e.what());
  }
}

bool ItemGroup::Modify(const dto::UpdateItemGroup &updateItemGroup) const {
  try {
    return pimpl->itemGroupRepository->Update(updateItemGroup);
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ModifyItemGroup Exception] ") +
                             e.what());
  }
}

std::vector<model::ItemGroup> ItemGroup::GetAll() const {
  try {
    std::vector<model::ItemGroup> itemGroups;
    type::DataTable dataTable = pimpl->itemGroupRepository->ReadAll();

    for (int i = 0; i < dataTable.RowsCount(); i++)
      itemGroups.emplace_back(model::ItemGroup{
          dataTable[i]["Entry"], dataTable[i]["Code"], dataTable[i]["Name"],
          dataTable[i]["CreatedBy"], dataTable[i]["CreateDate"],
          dataTable[i]["LastUpdatedBy"].GetOptional<int>(),
          dataTable[i]["UpdateDate"].GetOptional<std::string>()});

    return itemGroups;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetAllItemGroups Exception] ") +
                             e.what());
  }
}

model::ItemGroup ItemGroup::Get(const dto::GetItemGroup &getItemGroup) const {
  try {
    type::DataTable dataTable = pimpl->itemGroupRepository->Read(getItemGroup);

    if (dataTable.RowsCount() == 0)
      throw std::runtime_error("No ItemGroups found");

    model::ItemGroup itemGroup{
        dataTable[0]["Entry"],
        dataTable[0]["Code"],
        dataTable[0]["Name"],
        dataTable[0]["CreatedBy"],
        dataTable[0]["CreateDate"],
        dataTable[0]["LastUpdatedBy"].GetOptional<int>(),
        dataTable[0]["UpdateDate"].GetOptional<std::string>()};

    return itemGroup;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetItemGroup Exception] ") +
                             e.what());
  }
}
} // namespace omnicore::service