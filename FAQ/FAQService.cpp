#include "FAQ/FAQService.hpp"
#include <iostream>

namespace omnisphere::services
{
    FAQService::FAQService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repo(std::make_shared<omnisphere::repositories::FAQRepository>(std::move(dbPool))) {}

    FAQService::FAQService(std::shared_ptr<omnisphere::repositories::FAQRepository> repo)
        : m_repo(std::move(repo)) {}

    std::optional<omnisphere::models::FAQ> FAQService::CreateFAQ(omnisphere::dtos::CreateFAQ input)
    {
        if (!m_repo) return std::nullopt;
        if (input.name.empty()) {
            input.name = input.question.substr(0, std::min<size_t>(input.question.length(), 50));
        }
        if (m_repo->Create(input)) {
            return m_repo->GetByCode(input.code);
        }
        return std::nullopt;
    }

    std::optional<omnisphere::models::FAQ> FAQService::GetFAQByCode(const std::string& code)
    {
        if (!m_repo) return std::nullopt;
        return m_repo->GetByCode(code);
    }

    std::vector<omnisphere::models::FAQ> FAQService::GetFAQs(
        const std::optional<std::string>& scope,
        const std::optional<std::string>& category,
        const std::optional<bool>& onlyActive
    )
    {
        if (!m_repo) return {};
        return m_repo->GetFAQs(scope, category, onlyActive);
    }

    std::vector<std::string> FAQService::GetCategories(const std::optional<std::string>& scope)
    {
        if (!m_repo) return {};
        return m_repo->GetCategories(scope);
    }

    std::optional<omnisphere::models::FAQ> FAQService::UpdateFAQ(
        const std::string& code,
        const omnisphere::dtos::UpdateFAQ& input,
        const std::string& updatedBy
    )
    {
        if (!m_repo) return std::nullopt;
        if (m_repo->Update(code, input, updatedBy)) {
            return m_repo->GetByCode(code);
        }
        return std::nullopt;
    }

    bool FAQService::DeleteFAQByCode(const std::string& code)
    {
        if (!m_repo) return false;
        return m_repo->DeleteByCode(code);
    }
} // namespace omnisphere::services
