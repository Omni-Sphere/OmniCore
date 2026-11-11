#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Company/DTOs/CompanyPortalFeatureDTOs.hpp"
#include <OmniData/DataTable.hpp>
#include <OmniData/Database.hpp>
#include <OmniData/DatabasePool.hpp>

namespace omnisphere::repositories {

class CompanyPortalFeatureRepository {
public:
  explicit CompanyPortalFeatureRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
  explicit CompanyPortalFeatureRepository(std::shared_ptr<omnisphere::data::Database> database);
  ~CompanyPortalFeatureRepository();

  omnisphere::types::DataTable List(const omnisphere::dtos::GetPortalFeatures &dto, const std::vector<std::string>& fields = {}) const;
  bool Create(const omnisphere::dtos::CreatePortalFeature &dto) const;
  bool Update(const omnisphere::dtos::UpdatePortalFeature &dto, const std::vector<std::string>& mutationFields = {}) const;
  bool Delete(const omnisphere::dtos::DeletePortalFeature &dto) const;

private:
  std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
  std::shared_ptr<omnisphere::data::Database> m_legacyDb;
};

} // namespace omnisphere::repositories

