#include "Payment/PaymentExpiryDaemon.hpp"
#include "Payment/Hooks/PaymentHook.hpp"
#include <OmniUtils/Logger.hpp>
#include <chrono>

namespace omnisphere::payment
{
    PaymentExpiryDaemon::PaymentExpiryDaemon(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    PaymentExpiryDaemon::~PaymentExpiryDaemon()
    {
        Stop();
    }

    void PaymentExpiryDaemon::Start(int checkIntervalSeconds)
    {
        if (m_running.load()) return;
        m_running.store(true);
        m_workerThread = std::make_unique<std::thread>(&PaymentExpiryDaemon::RunLoop, this, checkIntervalSeconds);
        omnisphere::utils::Logger::LogInfo("PaymentExpiryDaemon", "Universal Payment Expiry Daemon started (interval: " + std::to_string(checkIntervalSeconds) + "s).");
    }

    void PaymentExpiryDaemon::Stop()
    {
        if (!m_running.load()) return;
        m_running.store(false);
        if (m_workerThread && m_workerThread->joinable())
        {
            m_workerThread->join();
        }
        omnisphere::utils::Logger::LogInfo("PaymentExpiryDaemon", "Universal Payment Expiry Daemon stopped.");
    }

    void PaymentExpiryDaemon::RunLoop(int checkIntervalSeconds)
    {
        while (m_running.load())
        {
            try
            {
                if (m_dbPool)
                {
                    auto conn = m_dbPool->Acquire();
                    // Buscar transacciones / órdenes con estado PENDING expiradas
                    std::string sql = R"(
                        SELECT "Entry", "EntityType", "EntityCode", "Provider", "Amount", "Currency", "ExpiresAt"
                        FROM "PaymentTransactions"
                        WHERE "Status" = 'PENDING'
                          AND "ExpiresAt" IS NOT NULL
                          AND "ExpiresAt" <= NOW()
                          AND "IsActive" = true
                    )";

                    std::vector<omnisphere::types::SQLParam> emptyParams;
                    auto dt = conn->FetchPrepared(sql, emptyParams);
                    for (const auto& row : dt)
                    {
                        int entry = row["Entry"].GetOptional<int>().value_or(0);
                        std::string entityType = row["EntityType"].GetOptional<std::string>().value_or("ROUTE_RESERVATION");
                        std::string entityCode = row["EntityCode"].GetOptional<std::string>().value_or("");
                        std::string provider = row["Provider"].GetOptional<std::string>().value_or("STRIPE");
                        double amount = row["Amount"].GetOptional<double>().value_or(0.0);

                        // Marcar transacción como EXPIRED en BD
                        conn->RunPrepared(
                            "UPDATE \"PaymentTransactions\" SET \"Status\" = 'EXPIRED' WHERE \"Entry\" = ?",
                            {omnisphere::types::MakeSQLParam(entry)}
                        );

                        // Despachar evento de expiración a los hooks de dominio registrados
                        PaymentEvent event;
                        event.eventType = "PAYMENT_EXPIRED";
                        event.entityType = entityType;
                        event.entityCode = entityCode;
                        event.provider = provider;
                        event.amount = amount;

                        omnisphere::utils::Logger::LogInfo("PaymentExpiryDaemon",
                            "Order expired for EntityType: [" + entityType + "], EntityCode: [" + entityCode + "]. Dispatching OnPaymentExpired.");

                        PaymentHookRegistry::Instance().DispatchExpired(event);
                    }
                }
            }
            catch (const std::exception& ex)
            {
                omnisphere::utils::Logger::LogError("PaymentExpiryDaemon", std::string("Error during expiration check: ") + ex.what());
            }
            catch (...) {}

            std::this_thread::sleep_for(std::chrono::seconds(checkIntervalSeconds));
        }
    }
} // namespace omnisphere::payment
