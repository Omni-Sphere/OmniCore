#pragma once
#include "Feedback/Models/FeedbackItem.hpp"
#include "Feedback/DTOs/CreateFeedback.hpp"
#include "Feedback/Repositories/FeedbackRepository.hpp"
#include <OmniData/DatabasePool.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace omnisphere::services
{
    class FeedbackService
    {
    private:
        std::shared_ptr<omnisphere::repositories::FeedbackRepository> m_repo;

    public:
        explicit FeedbackService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        explicit FeedbackService(std::shared_ptr<omnisphere::repositories::FeedbackRepository> repo);
        ~FeedbackService() = default;

        std::optional<omnisphere::models::FeedbackItem> CreateFeedback(omnisphere::dtos::CreateFeedback input);
        std::optional<omnisphere::models::FeedbackItem> GetFeedbackByCode(const std::string& code);
        std::vector<omnisphere::models::FeedbackItem> GetFeedbacks(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<std::string>& type = std::nullopt,
            const std::optional<bool>& onlyUnreviewed = std::nullopt,
            int limit = 50,
            int offset = 0
        );
        bool ToggleReviewedByCode(
            const std::string& code,
            bool isReviewed,
            const std::optional<std::string>& adminNotes = std::nullopt,
            const std::string& updatedBy = "system"
        );
        bool DeleteFeedbackByCode(const std::string& code);
    };
} // namespace omnisphere::services
