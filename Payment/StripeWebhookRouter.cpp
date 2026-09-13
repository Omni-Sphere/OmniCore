#include "Payment/StripeWebhookRouter.hpp"
#include "Payment/StripeWebhookHandler.hpp"
#include <vector>

namespace omnisphere::services
{
    void StripeWebhookRouter::RegisterEndpoints(
        std::shared_ptr<omnisphere::net::Router> router,
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
        StripePaymentHandler paymentCompletedHandler
    )
    {
        if (!router || !dbPool) return;

        // Instancia del controlador con RAII seguro
        auto handler = std::make_shared<StripeWebhookHandler>(dbPool, paymentCompletedHandler);

        auto repo = std::make_shared<omnisphere::repositories::StripeRepository>(dbPool);

        // Obtener la ruta configurada en la BD (o usar la ruta por defecto)
        std::string configuredPath = "/api/v1/stripe/webhook";
        try
        {
            auto dt = repo->GetSettings();
            if (dt.RowsCount() > 0 && dt[0].HasColumn("WebhookPath") && !dt[0]["WebhookPath"].IsNull())
            {
                std::string dbPath = (std::string)dt[0]["WebhookPath"];
                if (!dbPath.empty()) configuredPath = dbPath;
            }
        }
        catch (...) {}

        std::vector<std::string> pathsToRegister = { configuredPath };
        if (configuredPath != "/webhook") pathsToRegister.push_back("/webhook");
        if (configuredPath != "/stripe/webhook") pathsToRegister.push_back("/stripe/webhook");
        if (configuredPath != "/api/v1/stripe/webhook") pathsToRegister.push_back("/api/v1/stripe/webhook");

        for (const auto& p : pathsToRegister)
        {
            router->Post(p, [handler](const omnisphere::net::Request& req) -> omnisphere::net::Response {
                return handler->HandleWebhook(req);
            });
        }
    }
}
