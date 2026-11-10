#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Company/DTOs/CreateCompany.hpp"
#include "Company/DTOs/GetCompany.hpp"
#include "Company/DTOs/UpdateCompany.hpp"

#include <OmniData/DataTable.hpp>
#include <OmniData/Database.hpp>
#include <OmniData/DatabasePool.hpp>

namespace omnisphere::repositories {
class Company {
public:
  explicit Company(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
  explicit Company(std::shared_ptr<omnisphere::data::Database> database);
  ~Company();

  bool Create(const omnisphere::dtos::CreateCompany &_company) const;
  bool Update(const omnisphere::dtos::UpdateCompany &_company, const std::vector<std::string>& mutationFields = {}) const;
  omnisphere::types::DataTable Read(const omnisphere::dtos::GetCompany &_company, const std::vector<std::string>& fields = {}) const;
  bool EnsureDefaultExists() const;

private:
  std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
  std::shared_ptr<omnisphere::data::Database> m_legacyDb;
};
} // namespace omnisphere::repositories

