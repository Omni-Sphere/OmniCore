#pragma once
#include "Notification/Models/WhatsAppSettings.hpp"
#include "Notification/Models/WhatsAppConversation.hpp"
#include "Notification/Models/WhatsAppMessage.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include <memory>
#include <vector>
#include <string>

namespace omnisphere::repositories
{
    class WhatsAppRepository
    {
    public:
        explicit WhatsAppRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);

        omnisphere::types::DataTable GetSettings(const std::vector<std::string>& requestedFields = {}) const;
        bool SaveSettings(const omnisphere::models::WhatsAppSettings& settings) const;

        int GetOrCreateConversation(const std::string& customerPhone, const std::string& customerName = "") const;
        omnisphere::types::DataTable ReadConversations(int limit = 50) const;
        omnisphere::types::DataTable ReadMessages(int conversationEntry, int limit = 100) const;

        bool LogMessage(const omnisphere::models::WhatsAppMessage& msg) const;
        bool UpdateMessageStatus(const std::string& wamidCode, const std::string& newStatus, const std::string& responsePayload = "") const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };
}
