#pragma once

#include "DTOs/CreateItemBrand.hpp"
#include "DTOs/GetItemBrand.hpp"
#include "DTOs/UpdateItemBrand.hpp"
#include "Database.hpp"

namespace omnicore::repository {
class ItemBrand {
private:
  std::shared_ptr<service::Database> database;
  int GetCurrentSequence() const;
  bool UpdateItemBrandsSequence() const;

public:
  explicit ItemBrand(std::shared_ptr<service::Database> Database);
  ~ItemBrand() {};
  bool Create(const dto::CreateItemBrand &createItemBrand) const;
  bool Update(const dto::UpdateItemBrand &updateItemBrand) const;
  type::DataTable ReadAll() const;
  type::DataTable Read(const dto::GetItemBrand) const;
};
} // namespace omnicore::repository