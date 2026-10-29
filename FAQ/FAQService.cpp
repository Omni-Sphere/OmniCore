#include "FAQ/FAQService.hpp"
#include <iostream>

namespace omnisphere::services
{
    FAQService::FAQService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repo(std::make_shared<omnisphere::repositories::FAQRepository>(std::move(dbPool))) {}

    FAQService::FAQService(std::shared_ptr<omnisphere::repositories::FAQRepository> repo)
        : m_repo(std::move(repo)) {}

    std::optional<omnisphere::models::FAQ> FAQService::CreateFAQ(
        omnisphere::dtos::CreateFAQInput input,
        const std::vector<std::string>& mutationFields,
        const std::vector<std::string>& requestedFields
    )
    {
        if (!m_repo) return std::nullopt;
        if (!m_repo->Create(input, mutationFields))
        {
            return std::nullopt;
        }
        return m_repo->GetByCode(input.code, requestedFields);
    }

    std::optional<omnisphere::models::FAQ> FAQService::GetFAQByCode(
        const std::string& code,
        const std::vector<std::string>& requestedFields
    )
    {
        if (!m_repo) return std::nullopt;
        return m_repo->GetByCode(code, requestedFields);
    }

    std::vector<omnisphere::models::FAQ> FAQService::GetFAQs(
        const std::optional<std::string>& scope,
        const std::optional<std::string>& category,
        const std::optional<bool>& onlyActive,
        const std::vector<std::string>& requestedFields
    )
    {
        if (!m_repo) return {};
        return m_repo->GetFAQs(scope, category, onlyActive, requestedFields);
    }

    std::vector<std::string> FAQService::GetCategories(const std::optional<std::string>& scope)
    {
        if (!m_repo) return {};
        return m_repo->GetCategories(scope);
    }

    std::optional<omnisphere::models::FAQ> FAQService::UpdateFAQ(
        const omnisphere::dtos::UpdateFAQInput& input,
        const std::vector<std::string>& mutationFields,
        const std::vector<std::string>& requestedFields
    )
    {
        if (!m_repo) return std::nullopt;
        if (!m_repo->Update(input, mutationFields))
        {
            return std::nullopt;
        }
        return m_repo->GetByCode(input.code, requestedFields);
    }

    bool FAQService::DeleteFAQByCode(const std::string& code)
    {
        if (!m_repo) return false;
        return m_repo->DeleteByCode(code);
    }
} // namespace omnisphere::services
