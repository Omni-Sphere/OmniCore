#pragma once
#include <OmniData/DatabasePool.hpp>
#include <atomic>
#include <memory>
#include <thread>

namespace omnisphere::payment
{
    class PaymentExpiryDaemon
    {
    public:
        explicit PaymentExpiryDaemon(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~PaymentExpiryDaemon();

        void Start(int checkIntervalSeconds = 10);
        void Stop();

    private:
        void RunLoop(int checkIntervalSeconds);

        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
        std::atomic<bool> m_running{false};
        std::unique_ptr<std::thread> m_workerThread;
    };
} // namespace omnisphere::payment
