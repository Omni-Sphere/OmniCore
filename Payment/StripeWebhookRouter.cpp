#include "Payment/StripeWebhookRouter.hpp"
#include <OmniUtils/Base64.hpp>
#include <OmniUtils/Hasher.hpp>
#include <OmniUtils/Logger.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <vector>

namespace json = boost::json;

namespace omnisphere::services
{
    void StripeWebhookRouter::RegisterEndpoints(
        std::shared_ptr<omnisphere::net::Router> router,
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
        StripePaymentHandler paymentCompletedHandler
    )
    {
        if (!router || !dbPool) return;

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
            router->Post(p, [repo, paymentCompletedHandler](const omnisphere::net::Request& req) -> omnisphere::net::Response {
                omnisphere::utils::Logger::LogHttpRequest(req);
                try
                {
                    std::string sigHeader = req.Header("Stripe-Signature");
                    if (sigHeader.empty()) sigHeader = req.Header("stripe-signature");

                    omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Incoming POST event. Signature: " + sigHeader);
                    omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Stripe Webhook Payload: " + req.Body());

                    auto parsed = req.Json();
                    if (parsed.is_object())
                    {
                        auto obj = parsed.as_object();
                        std::string eventType = "";
                        if (obj.contains("type") && obj.at("type").is_string())
                        {
                            eventType = std::string(obj.at("type").as_string());
                        }

                        omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Event Type Received: [" + eventType + "]");

                        if (eventType == "checkout.session.completed" || eventType == "payment_intent.succeeded")
                        {
                            std::string sessionId = "";
                            std::string reservationCode = "";
                            std::string paymentIntentId = "";
                            double amount = 0.0;

                            if (obj.contains("data") && obj.at("data").is_object())
                            {
                                auto dataObj = obj.at("data").as_object();
                                if (dataObj.contains("object") && dataObj.at("object").is_object())
                                {
                                    auto sessObj = dataObj.at("object").as_object();
                                    if (sessObj.contains("id") && sessObj.at("id").is_string())
                                        sessionId = std::string(sessObj.at("id").as_string());

                                    if (sessObj.contains("client_reference_id") && sessObj.at("client_reference_id").is_string())
                                        reservationCode = std::string(sessObj.at("client_reference_id").as_string());

                                    if (sessObj.contains("payment_intent") && sessObj.at("payment_intent").is_string())
                                        paymentIntentId = std::string(sessObj.at("payment_intent").as_string());

                                    if (sessObj.contains("amount_total") && sessObj.at("amount_total").is_number())
                                        amount = sessObj.at("amount_total").as_double() / 100.0;
                                }
                            }

                            // Buscar la sesión registrada si reservationCode estaba vacío
                            if (reservationCode.empty() && !sessionId.empty())
                            {
                                auto sessOpt = repo->GetSessionByStripeId(sessionId);
                                if (sessOpt.has_value())
                                {
                                    reservationCode = sessOpt->reservationCode;
                                }
                            }

                            omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Payment Completed Event Processed for Session [" + sessionId + "], Reservation [" + reservationCode + "]");

                            if (!sessionId.empty())
                            {
                                repo->UpdateSessionStatus(sessionId, "complete", paymentIntentId);
                            }

                            if (paymentCompletedHandler)
                            {
                                paymentCompletedHandler(req, sessionId, reservationCode, paymentIntentId, amount);
                            }
                        }
                    }
                }
                catch (const std::exception& ex)
                {
                    omnisphere::utils::Logger::LogError("StripeWebhook", req.TraceContext() + " Exception processing Stripe webhook: " + ex.what());
                }

                boost::json::object resObj;
                resObj["received"] = true;
                return omnisphere::net::Response::Json(resObj);
            });
        }
    }
}
