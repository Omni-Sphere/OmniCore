#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Company/DTOs/CompanyBrandingDTOs.hpp"
#include <OmniData/DataTable.hpp>
#include <OmniData/Database.hpp>
#include <OmniData/DatabasePool.hpp>

namespace omnisphere::repositories {

class CompanyBrandingRepository {
public:
  explicit CompanyBrandingRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
  explicit CompanyBrandingRepository(std::shared_ptr<omnisphere::data::Database> database);
  ~CompanyBrandingRepository();

  bool EnsureDefaultExists(int companyEntry = 1) const;
  omnisphere::types::DataTable Read(const omnisphere::dtos::GetCompanyBranding &dto, const std::vector<std::string>& fields = {}) const;
  bool Update(const omnisphere::dtos::UpdateCompanyBranding &dto, const std::vector<std::string>& mutationFields = {}) const;

private:
  std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
  std::shared_ptr<omnisphere::data::Database> m_legacyDb;
};

} // namespace omnisphere::repositories

