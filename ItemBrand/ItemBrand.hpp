#pragma once

#include <memory>
#include <vector>

namespace omnicore::service {
class Database;
}
namespace omnicore::repository {
class ItemBrand;
}
namespace omnicore::dto {
class CreateItemBrand;
class UpdateItemBrand;
class GetItemBrand;
} // namespace omnicore::dto
namespace omnicore::model {
class ItemBrand;
}

namespace omnicore::service {

class ItemBrand {
private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;

public:
  explicit ItemBrand(std::shared_ptr<service::Database> db);

  ~ItemBrand();

  bool Add(const dto::CreateItemBrand &createItemBrand) const;

  bool Modify(const dto::UpdateItemBrand &updateItemBrand) const;

  std::vector<model::ItemBrand> GetAll() const;

  model::ItemBrand Get(const dto::GetItemBrand &getItemBrand) const;
};
} // namespace omnicore::service