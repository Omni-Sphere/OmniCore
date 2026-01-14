#pragma once

#include <memory>
#include <vector>

namespace omnicore::service {
class Database;
}
namespace omnicore::repository {
class ItemGroup;
}
namespace omnicore::dto {
class CreateItemGroup;
class UpdateItemGroup;
class GetItemGroup;
} // namespace omnicore::dto
namespace omnicore::model {
class ItemGroup;
}

namespace omnicore::service {

class ItemGroup {
private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;

public:
  explicit ItemGroup(std::shared_ptr<service::Database> db);

  ~ItemGroup();

  bool Add(const dto::CreateItemGroup &createItemGroup) const;

  bool Modify(const dto::UpdateItemGroup &updateItemGroup) const;

  std::vector<model::ItemGroup> GetAll() const;

  model::ItemGroup Get(const dto::GetItemGroup &getItemGroup) const;
};
} // namespace omnicore::service