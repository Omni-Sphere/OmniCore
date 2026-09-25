#pragma once
#include "Employee/Models/Employee.hpp"
#include "Employee/DTOs/CreateEmployee.hpp"
#include "Employee/DTOs/UpdateEmployee.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace omnisphere::repositories
{
    struct EmployeeCursorPage
    {
        std::vector<omnisphere::models::Employee> employees;
        int totalCount = 0;
        std::optional<int> nextCursor;
        bool hasPreviousPage = false;
    };

    class Employee
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

        omnisphere::models::Employee MapRow(omnisphere::types::DataTable::Row& row) const;

    public:
        explicit Employee(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~Employee() = default;

        bool Create(const omnisphere::dtos::CreateEmployee& employee, const std::vector<std::string>& mutationFields = {}) const;
        bool Update(const omnisphere::dtos::UpdateEmployee& employee, const std::vector<std::string>& mutationFields = {}) const;
        bool Delete(const std::string& code, const std::vector<std::string>& mutationFields = {}) const;

        std::optional<omnisphere::models::Employee> GetByCode(const std::string& code, const std::vector<std::string>& fields = {}) const;
        std::optional<omnisphere::models::Employee> GetByEntry(int entry, const std::vector<std::string>& fields = {}) const;
        std::vector<omnisphere::models::Employee> GetAll(const std::vector<std::string>& fields = {}) const;
        EmployeeCursorPage GetPage(std::optional<int> afterEntry, int limit, const std::vector<std::string>& fields = {}) const;
    };
} // namespace omnisphere::repositories
