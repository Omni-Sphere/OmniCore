#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include "Company/DTOs/CompanyBrandingDTOs.hpp"
#include "Company/DTOs/CompanyPortalFeatureDTOs.hpp"
#include "Company/Models/CompanyBranding.hpp"
#include "Company/Models/CompanyPortalFeature.hpp"
#include "Company/Repositories/CompanyBrandingRepository.hpp"
#include "Company/Repositories/CompanyPortalFeatureRepository.hpp"
#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Authorization.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/DatabasePool.hpp>

namespace omnisphere::services {

class CompanyBrandingService {
public:
  explicit CompanyBrandingService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
                                  std::shared_ptr<omnisphere::services::Authorization> authService = nullptr);
  explicit CompanyBrandingService(std::shared_ptr<omnisphere::data::Database> database,
                                  std::shared_ptr<omnisphere::services::Authorization> authService = nullptr);
  ~CompanyBrandingService();

  omnisphere::models::CompanyBranding Get(const omnisphere::dtos::GetCompanyBranding &dto, const std::vector<std::string>& fields = {}) const;
  bool Modify(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdateCompanyBranding &dto, const std::vector<std::string>& mutationFields = {}) const;
  bool Modify(const omnisphere::dtos::UpdateCompanyBranding &dto, const std::vector<std::string>& mutationFields = {}) const;

  std::vector<omnisphere::models::CompanyPortalFeature> ListFeatures(const omnisphere::dtos::GetPortalFeatures &dto, const std::vector<std::string>& fields = {}) const;
  bool CreateFeature(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreatePortalFeature &dto) const;
  bool UpdateFeature(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdatePortalFeature &dto, const std::vector<std::string>& mutationFields = {}) const;
  bool DeleteFeature(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::DeletePortalFeature &dto) const;

private:
  std::shared_ptr<omnisphere::repositories::CompanyBrandingRepository> m_brandingRepo;
  std::shared_ptr<omnisphere::repositories::CompanyPortalFeatureRepository> m_featureRepo;
  std::shared_ptr<omnisphere::services::Authorization> m_authService;
};

} // namespace omnisphere::services

