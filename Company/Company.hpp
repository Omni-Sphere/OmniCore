#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include "Company/DTOs/CreateCompany.hpp"
#include "Company/DTOs/GetCompany.hpp"
#include "Company/DTOs/UpdateCompany.hpp"
#include "Company/Models/Company.hpp"
#include "Company/Repositories/Company.hpp"
#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Authorization.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/DatabasePool.hpp>

namespace omnisphere::services {
class Company {
public:
  explicit Company(std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
                   std::shared_ptr<omnisphere::services::Authorization> authService = nullptr);
  explicit Company(std::shared_ptr<omnisphere::data::Database> database,
                   std::shared_ptr<omnisphere::services::Authorization> authService = nullptr);
  explicit Company(std::shared_ptr<omnisphere::repositories::Company> repository,
                   std::shared_ptr<omnisphere::services::Authorization> authService = nullptr);
  ~Company();

  omnisphere::models::Company Get(const omnisphere::dtos::GetCompany &_company, const std::vector<std::string>& fields = {}) const;
  std::optional<omnisphere::models::Company> GetOptional(const omnisphere::dtos::GetCompany &_company, const std::vector<std::string>& fields = {}) const;
  bool Add(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreateCompany &_company) const;
  bool Modify(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdateCompany &_company, const std::vector<std::string>& mutationFields = {}) const;
  bool Modify(const omnisphere::dtos::UpdateCompany &_company, const std::vector<std::string>& mutationFields = {}) const;

private:
  std::shared_ptr<omnisphere::repositories::Company> m_repository;
  std::shared_ptr<omnisphere::services::Authorization> m_authService;
};

} // namespace omnisphere::services

