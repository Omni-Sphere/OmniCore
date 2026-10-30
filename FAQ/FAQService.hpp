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

        // --- FAQ Category Service Methods ---
        std::optional<omnisphere::models::FAQCategory> CreateCategory(
            omnisphere::dtos::CreateFAQCategoryInput input,
            const std::vector<std::string>& mutationFields = {},
            const std::vector<std::string>& requestedFields = {}
        );
        std::optional<omnisphere::models::FAQCategory> GetCategoryByCode(
            const std::string& code,
            const std::vector<std::string>& requestedFields = {}
        );
        std::vector<omnisphere::models::FAQCategory> GetCategories(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<bool>& onlyActive = std::nullopt,
            const std::vector<std::string>& requestedFields = {}
        );
        std::optional<omnisphere::models::FAQCategory> UpdateCategory(
            const omnisphere::dtos::UpdateFAQCategoryInput& input,
            const std::vector<std::string>& mutationFields = {},
            const std::vector<std::string>& requestedFields = {}
        );
        bool DeleteCategoryByCode(const std::string& code);

        // --- FAQ Service Methods ---
        std::optional<omnisphere::models::FAQ> CreateFAQ(
            omnisphere::dtos::CreateFAQInput input,
            const std::vector<std::string>& mutationFields = {},
            const std::vector<std::string>& requestedFields = {}
        );
        std::optional<omnisphere::models::FAQ> GetFAQByCode(
            const std::string& code,
            const std::vector<std::string>& requestedFields = {}
        );
        std::vector<omnisphere::models::FAQ> GetFAQs(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<std::string>& categoryCode = std::nullopt,
            const std::optional<bool>& onlyActive = std::nullopt,
            const std::vector<std::string>& requestedFields = {}
        );
        std::optional<omnisphere::models::FAQ> UpdateFAQ(
            const omnisphere::dtos::UpdateFAQInput& input,
            const std::vector<std::string>& mutationFields = {},
            const std::vector<std::string>& requestedFields = {}
        );
        bool DeleteFAQByCode(const std::string& code);
    };
} // namespace omnisphere::services
