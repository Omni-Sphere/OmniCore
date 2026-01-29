#pragma once

#include <memory>
#include <vector>

namespace omnicore::service {
class Database;
}
namespace omnicore::repository {
class Item;
}

#include "DTOs/CreateItem.hpp"
#include "DTOs/GetItem.hpp"
#include "DTOs/SearchItems.hpp"
#include "DTOs/UpdateItem.hpp"
#include "Models/Item.hpp"

namespace omnicore::service {

class Item {
public:
  explicit Item(std::shared_ptr<service::Database> database);

  ~Item();

  model::Item Get(const dto::GetItem &_item) const;

  std::vector<model::Item> GetAll() const;

  std::vector<model::Item> Search(dto::SearchItems &_item) const;

  model::Item Add(const dto::CreateItem &_item) const;

  model::Item Modify(const dto::UpdateItem &_item) const;

private:
  struct Impl;
  std::unique_ptr<Impl> pimpl;
};
} // namespace omnicore::service