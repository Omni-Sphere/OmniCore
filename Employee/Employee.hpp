#pragma once
#include "Employee/Repositories/Employee.hpp"
#include "Authorization/Models/SecurityContext.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace omnisphere::services
{
    class Employee
    {
    private:
        std::shared_ptr<omnisphere::repositories::Employee> m_repository;

    public:
        explicit Employee(std::shared_ptr<omnisphere::repositories::Employee> repository);
        explicit Employee(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~Employee() = default;

        bool Create(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreateEmployee& employee) const;
        bool Update(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdateEmployee& employee) const;
        bool Delete(const omnisphere::models::SecurityContext& ctx, const std::string& code) const;

        std::optional<omnisphere::models::Employee> GetByCode(const std::string& code, const std::vector<std::string>& fields = {}) const;
        std::optional<omnisphere::models::Employee> GetByEntry(int entry, const std::vector<std::string>& fields = {}) const;
        std::vector<omnisphere::models::Employee> GetAll(const std::vector<std::string>& fields = {}) const;
        omnisphere::repositories::EmployeeCursorPage GetPage(std::optional<int> afterEntry, int limit, const std::vector<std::string>& fields = {}) const;
    };
} // namespace omnisphere::services
