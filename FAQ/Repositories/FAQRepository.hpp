#pragma once
#include "FAQ/Models/FAQ.hpp"
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

        bool Create(omnisphere::dtos::CreateFAQ& faq) const;
        std::optional<omnisphere::models::FAQ> GetByCode(const std::string& code) const;
        std::vector<omnisphere::models::FAQ> GetFAQs(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<std::string>& category = std::nullopt,
            const std::optional<bool>& onlyActive = std::nullopt
        ) const;
        std::vector<std::string> GetCategories(const std::optional<std::string>& scope = std::nullopt) const;
        bool Update(
            const std::string& code,
            const omnisphere::dtos::UpdateFAQ& input,
            const std::string& updatedBy = "system"
        ) const;
        bool DeleteByCode(const std::string& code) const;
    };
} // namespace omnisphere::repositories
