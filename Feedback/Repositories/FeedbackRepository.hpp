#pragma once
#include "Feedback/Models/FeedbackItem.hpp"
#include "Feedback/DTOs/CreateFeedback.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace omnisphere::repositories
{
    class FeedbackRepository
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

    public:
        explicit FeedbackRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~FeedbackRepository() = default;

        bool Create(omnisphere::dtos::CreateFeedback& feedback) const;
        std::optional<omnisphere::models::FeedbackItem> GetByCode(const std::string& code) const;
        std::vector<omnisphere::models::FeedbackItem> GetFeedbacks(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<std::string>& type = std::nullopt,
            const std::optional<bool>& onlyUnreviewed = std::nullopt,
            int limit = 50,
            int offset = 0
        ) const;
        bool ToggleReviewedByCode(
            const std::string& code,
            bool isReviewed,
            const std::optional<std::string>& adminNotes = std::nullopt,
            const std::string& updatedBy = "system"
        ) const;
        bool DeleteByCode(const std::string& code) const;
    };
} // namespace omnisphere::repositories
