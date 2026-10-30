#pragma once
#include "FAQ/Models/FAQ.hpp"
#include "FAQ/Models/FAQCategory.hpp"
#include "FAQ/DTOs/FAQDTOs.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace omnisphere::repositories
{
    class FAQRepository
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

    public:
        explicit FAQRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~FAQRepository() = default;

        // --- FAQ Category Methods ---
        bool CreateCategory(omnisphere::dtos::CreateFAQCategoryInput& category, const std::vector<std::string>& mutationFields = {}) const;
        std::optional<omnisphere::models::FAQCategory> GetCategoryByCode(const std::string& code, const std::vector<std::string>& fields = {}) const;
        std::vector<omnisphere::models::FAQCategory> GetCategories(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<bool>& onlyActive = std::nullopt,
            const std::vector<std::string>& fields = {}
        ) const;
        bool UpdateCategory(
            const omnisphere::dtos::UpdateFAQCategoryInput& input,
            const std::vector<std::string>& mutationFields = {}
        ) const;
        bool DeleteCategoryByCode(const std::string& code) const;

        // --- FAQ Methods ---
        bool Create(omnisphere::dtos::CreateFAQInput& faq, const std::vector<std::string>& mutationFields = {}) const;
        std::optional<omnisphere::models::FAQ> GetByCode(const std::string& code, const std::vector<std::string>& fields = {}) const;
        std::vector<omnisphere::models::FAQ> GetFAQs(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<std::string>& categoryCode = std::nullopt,
            const std::optional<bool>& onlyActive = std::nullopt,
            const std::vector<std::string>& fields = {}
        ) const;
        bool Update(
            const omnisphere::dtos::UpdateFAQInput& input,
            const std::vector<std::string>& mutationFields = {}
        ) const;
        bool DeleteByCode(const std::string& code) const;
    };
} // namespace omnisphere::repositories
