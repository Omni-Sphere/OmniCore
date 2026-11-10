#include "Company/Company.hpp"
#include "Authorization/AuthGuard.hpp"
#include <OmniData/DataTable.hpp>
#include <OmniData/DataMapper.hpp>
#include <stdexcept>

namespace omnisphere::services {

Company::Company(std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
                 std::shared_ptr<omnisphere::services::Authorization> authService)
    : m_repository(std::make_shared<omnisphere::repositories::Company>(dbPool)),
      m_authService(authService ? std::move(authService) : std::make_shared<omnisphere::services::Authorization>(dbPool)) {}

Company::Company(std::shared_ptr<omnisphere::data::Database> database,
                 std::shared_ptr<omnisphere::services::Authorization> authService)
    : m_repository(std::make_shared<omnisphere::repositories::Company>(database)),
      m_authService(std::move(authService)) {}

Company::Company(std::shared_ptr<omnisphere::repositories::Company> repository,
                 std::shared_ptr<omnisphere::services::Authorization> authService)
    : m_repository(std::move(repository)),
      m_authService(std::move(authService)) {}

Company::~Company() = default;

omnisphere::models::Company
Company::Get(const omnisphere::dtos::GetCompany &_company, const std::vector<std::string>& fields) const {
  if (!m_repository) {
    throw std::runtime_error("Company repository is not initialized.");
  }
  omnisphere::types::DataTable dataTable = m_repository->Read(_company, fields);

  if (dataTable.IsEmpty()) {
    throw std::runtime_error("Business Configuration not found.");
  }

  return omnisphere::types::FromDataRow<omnisphere::models::Company>(dataTable[0]);
}

std::optional<omnisphere::models::Company>
Company::GetOptional(const omnisphere::dtos::GetCompany &_company, const std::vector<std::string>& fields) const {
  if (!m_repository) return std::nullopt;
  omnisphere::types::DataTable dataTable = m_repository->Read(_company, fields);
  if (dataTable.IsEmpty()) return std::nullopt;
  return omnisphere::types::FromDataRow<omnisphere::models::Company>(dataTable[0]);
}

bool Company::Add(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreateCompany &_company) const {
  AUTHORIZE(ctx, "MOD_SETTINGS", "CORE_CONFIG_UPDATE");
  if (!m_repository) return false;
  return m_repository->Create(_company);
}

bool Company::Modify(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdateCompany &_company, const std::vector<std::string>& mutationFields) const {
  AUTHORIZE(ctx, "MOD_SETTINGS", "CORE_CONFIG_UPDATE");
  if (!m_repository) return false;
  auto mutableCompany = _company;
  if (ctx.isAuthenticated() && !mutableCompany.LastUpdatedBy.has_value()) {
    try {
      mutableCompany.LastUpdatedBy = std::stoi(ctx.userCode);
    } catch (...) {}
  }
  return m_repository->Update(mutableCompany, mutationFields);
}

bool Company::Modify(const omnisphere::dtos::UpdateCompany &_company, const std::vector<std::string>& mutationFields) const {
  if (!m_repository) return false;
  return m_repository->Update(_company, mutationFields);
}

} // namespace omnisphere::services

