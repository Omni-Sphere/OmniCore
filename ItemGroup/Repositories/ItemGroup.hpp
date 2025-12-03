#pragma once

#include "DTOs/CreateItemGroup.hpp"
#include "DTOs/GetItemGroup.hpp"
#include "DTOs/UpdateItemGroup.hpp"
#include "Database.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace omnicore::respository {
class ItemGroup {
private:
  std::shared_ptr<service::Database> database;
  int GetCurrentSequence() const;
  bool UpdateItemGroupsSequence() const;

public:
  explicit ItemGroup(std::shared_ptr<service::Database> Database);
  ~ItemGroup() {};

  bool Create(const dto::CreateItemGroup &createItemGroup) const;
  bool Update(const dto::UpdateItemGroup &updateItemGroup) const;
  type::Datatable ReadAll() const;
  type::Datatable Read(const dto::GetItemGroup) const;
};
} // namespace omnicore::respository