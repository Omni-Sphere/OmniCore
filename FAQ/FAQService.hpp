#pragma once
#include "FAQ/Models/FAQ.hpp"
#include "FAQ/DTOs/FAQDTOs.hpp"
#include "FAQ/Repositories/FAQRepository.hpp"
#include <OmniData/DatabasePool.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace omnisphere::services
{
    class FAQService
    {
    private:
        std::shared_ptr<omnisphere::repositories::FAQRepository> m_repo;

    public:
        explicit FAQService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        explicit FAQService(std::shared_ptr<omnisphere::repositories::FAQRepository> repo);
        ~FAQService() = default;

        std::optional<omnisphere::models::FAQ> CreateFAQ(omnisphere::dtos::CreateFAQ input);
        std::optional<omnisphere::models::FAQ> GetFAQByCode(const std::string& code);
        std::vector<omnisphere::models::FAQ> GetFAQs(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<std::string>& category = std::nullopt,
            const std::optional<bool>& onlyActive = std::nullopt
        );
        std::vector<std::string> GetCategories(const std::optional<std::string>& scope = std::nullopt);
        std::optional<omnisphere::models::FAQ> UpdateFAQ(
            const std::string& code,
            const omnisphere::dtos::UpdateFAQ& input,
            const std::string& updatedBy = "system"
        );
        bool DeleteFAQByCode(const std::string& code);
    };
} // namespace omnisphere::services
