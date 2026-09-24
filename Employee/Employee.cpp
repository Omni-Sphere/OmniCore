#include "Employee/Employee.hpp"

namespace omnisphere::services
{
    Employee::Employee(std::shared_ptr<omnisphere::repositories::Employee> repository)
        : m_repository(std::move(repository)) {}

    Employee::Employee(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repository(std::make_shared<omnisphere::repositories::Employee>(std::move(dbPool))) {}

    bool Employee::Create(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreateEmployee& employee) const
    {
        if (!m_repository) return false;
        auto mutableDto = employee;
        if (ctx.isAuthenticated() && !ctx.userCode.empty())
        {
            try { mutableDto.createdBy = std::stoi(ctx.userCode); } catch (...) { mutableDto.createdBy = 1; }
        }
        return m_repository->Create(mutableDto);
    }

    bool Employee::Update(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdateEmployee& employee) const
    {
        if (!m_repository) return false;
        auto mutableDto = employee;
        if (ctx.isAuthenticated() && !ctx.userCode.empty())
        {
            try { mutableDto.updatedBy = std::stoi(ctx.userCode); } catch (...) { mutableDto.updatedBy = 1; }
        }
        return m_repository->Update(mutableDto);
    }

    bool Employee::Delete(const omnisphere::models::SecurityContext& /*ctx*/, const std::string& code) const
    {
        if (!m_repository) return false;
        return m_repository->Delete(code);
    }

    std::optional<omnisphere::models::Employee> Employee::GetByCode(const std::string& code, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return std::nullopt;
        return m_repository->GetByCode(code, fields);
    }

    std::optional<omnisphere::models::Employee> Employee::GetByEntry(int entry, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return std::nullopt;
        return m_repository->GetByEntry(entry, fields);
    }

    std::vector<omnisphere::models::Employee> Employee::GetAll(const std::vector<std::string>& fields) const
    {
        if (!m_repository) return {};
        return m_repository->GetAll(fields);
    }

    omnisphere::repositories::EmployeeCursorPage Employee::GetPage(std::optional<int> afterEntry, int limit, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return {};
        return m_repository->GetPage(afterEntry, limit, fields);
    }
} // namespace omnisphere::services
