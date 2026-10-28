#include "Feedback/FeedbackService.hpp"
#include <iostream>

namespace omnisphere::services
{
    FeedbackService::FeedbackService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repo(std::make_shared<omnisphere::repositories::FeedbackRepository>(std::move(dbPool))) {}

    FeedbackService::FeedbackService(std::shared_ptr<omnisphere::repositories::FeedbackRepository> repo)
        : m_repo(std::move(repo)) {}

    std::optional<omnisphere::models::FeedbackItem> FeedbackService::CreateFeedback(omnisphere::dtos::CreateFeedback input)
    {
        if (!m_repo) return std::nullopt;
        if (input.name.empty()) {
            input.name = input.type + " - " + input.scope;
        }
        if (m_repo->Create(input)) {
            return m_repo->GetByCode(input.code);
        }
        return std::nullopt;
    }

    std::optional<omnisphere::models::FeedbackItem> FeedbackService::GetFeedbackByCode(const std::string& code)
    {
        if (!m_repo) return std::nullopt;
        return m_repo->GetByCode(code);
    }

    std::vector<omnisphere::models::FeedbackItem> FeedbackService::GetFeedbacks(
        const std::optional<std::string>& scope,
        const std::optional<std::string>& type,
        const std::optional<bool>& onlyUnreviewed,
        int limit,
        int offset
    )
    {
        if (!m_repo) return {};
        return m_repo->GetFeedbacks(scope, type, onlyUnreviewed, limit, offset);
    }

    bool FeedbackService::ToggleReviewedByCode(
        const std::string& code,
        bool isReviewed,
        const std::optional<std::string>& adminNotes,
        const std::string& updatedBy
    )
    {
        if (!m_repo) return false;
        return m_repo->ToggleReviewedByCode(code, isReviewed, adminNotes, updatedBy);
    }

    bool FeedbackService::DeleteFeedbackByCode(const std::string& code)
    {
        if (!m_repo) return false;
        return m_repo->DeleteByCode(code);
    }
} // namespace omnisphere::services
