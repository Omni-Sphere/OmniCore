#pragma once
#include "Payment/Models/StripeSettings.hpp"
#include "Payment/Models/StripeSession.hpp"
#include "Payment/Models/StripeTransaction.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace omnisphere::repositories
{
    class StripeRepository
    {
    public:
        explicit StripeRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);

        omnisphere::types::DataTable GetSettings(const std::vector<std::string>& requestedFields = {}) const;
        bool SaveSettings(const omnisphere::models::StripeSettings& settings) const;

        bool SaveSession(const omnisphere::models::StripeSession& session) const;
        bool UpdateSessionStatus(const std::string& stripeSessionId, const std::string& newStatus, const std::string& paymentIntentId = "") const;
        omnisphere::types::DataTable GetSessionsByReservation(const std::string& reservationCode) const;
        std::optional<omnisphere::models::StripeSession> GetSessionByStripeId(const std::string& stripeSessionId) const;

        bool SaveTransaction(const omnisphere::models::StripeTransaction& tx) const;
        bool UpdateTransactionStatus(const std::string& paymentIntentId, const std::string& newStatus, const std::string& receiptUrl = "") const;
        omnisphere::types::DataTable GetTransactionsByReservation(const std::string& reservationCode) const;
        std::optional<omnisphere::models::StripeTransaction> GetTransactionByPaymentIntent(const std::string& paymentIntentId) const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };
}
