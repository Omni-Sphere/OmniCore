#include "Company/CompanyBrandingService.hpp"
#include "Authorization/AuthGuard.hpp"
#include <OmniData/DataMapper.hpp>
#include <iostream>

namespace omnisphere::services {

CompanyBrandingService::CompanyBrandingService(
    std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
    std::shared_ptr<omnisphere::services::Authorization> authService)
    : m_brandingRepo(std::make_shared<omnisphere::repositories::CompanyBrandingRepository>(dbPool)),
      m_featureRepo(std::make_shared<omnisphere::repositories::CompanyPortalFeatureRepository>(dbPool)),
      m_authService(authService ? std::move(authService) : std::make_shared<omnisphere::services::Authorization>(dbPool)) {}

CompanyBrandingService::CompanyBrandingService(
    std::shared_ptr<omnisphere::data::Database> database,
    std::shared_ptr<omnisphere::services::Authorization> authService)
    : m_brandingRepo(std::make_shared<omnisphere::repositories::CompanyBrandingRepository>(database)),
      m_featureRepo(std::make_shared<omnisphere::repositories::CompanyPortalFeatureRepository>(database)),
      m_authService(std::move(authService)) {}

CompanyBrandingService::~CompanyBrandingService() = default;

omnisphere::models::CompanyBranding
CompanyBrandingService::Get(const omnisphere::dtos::GetCompanyBranding &dto, const std::vector<std::string>& fields) const {
  auto dt = m_brandingRepo->Read(dto, fields);
  if (dt.IsEmpty()) {
    return omnisphere::models::CompanyBranding{};
  }
  return omnisphere::types::FromDataRow<omnisphere::models::CompanyBranding>(dt[0]);
}

bool CompanyBrandingService::Modify(const omnisphere::models::SecurityContext& ctx,
                                    const omnisphere::dtos::UpdateCompanyBranding &dto,
                                    const std::vector<std::string>& mutationFields) const {
  AUTHORIZE(ctx, "MOD_SETTINGS", "CORE_CONFIG_UPDATE");
  return m_brandingRepo->Update(dto, mutationFields);
}

bool CompanyBrandingService::Modify(const omnisphere::dtos::UpdateCompanyBranding &dto,
                                    const std::vector<std::string>& mutationFields) const {
  return m_brandingRepo->Update(dto, mutationFields);
}

std::vector<omnisphere::models::CompanyPortalFeature>
CompanyBrandingService::ListFeatures(const omnisphere::dtos::GetPortalFeatures &dto, const std::vector<std::string>& fields) const {
  auto dt = m_featureRepo->List(dto, fields);
  return omnisphere::types::DataTableToModels<omnisphere::models::CompanyPortalFeature>(dt);
}

bool CompanyBrandingService::CreateFeature(const omnisphere::models::SecurityContext& ctx,
                                           const omnisphere::dtos::CreatePortalFeature &dto) const {
  AUTHORIZE(ctx, "MOD_SETTINGS", "CORE_CONFIG_UPDATE");
  return m_featureRepo->Create(dto);
}

bool CompanyBrandingService::UpdateFeature(const omnisphere::models::SecurityContext& ctx,
                                           const omnisphere::dtos::UpdatePortalFeature &dto,
                                           const std::vector<std::string>& mutationFields) const {
  AUTHORIZE(ctx, "MOD_SETTINGS", "CORE_CONFIG_UPDATE");
  return m_featureRepo->Update(dto, mutationFields);
}

bool CompanyBrandingService::DeleteFeature(const omnisphere::models::SecurityContext& ctx,
                                           const omnisphere::dtos::DeletePortalFeature &dto) const {
  AUTHORIZE(ctx, "MOD_SETTINGS", "CORE_CONFIG_UPDATE");
  return m_featureRepo->Delete(dto);
}

} // namespace omnisphere::services
