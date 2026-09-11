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

                    // 1. Verificación de Firma Criptográfica HMAC-SHA256 si existe WebhookSecretKey configurado (whsec_...)
                    std::string webhookSecret = "";
                    try
                    {
                        auto dtSettings = repo->GetSettings();
                        if (dtSettings.RowsCount() > 0 && dtSettings[0].HasColumn("WebhookSecretKey") && !dtSettings[0]["WebhookSecretKey"].IsNull())
                        {
                            webhookSecret = (std::string)dtSettings[0]["WebhookSecretKey"];
                            if (webhookSecret.rfind("whsec_", 0) != 0)
                            {
                                try { webhookSecret = omnisphere::utils::Base64::Decode(webhookSecret); } catch (...) {}
                            }
                        }
                    }
                    catch (...) {}

                    if (!webhookSecret.empty() && webhookSecret.rfind("whsec_", 0) == 0)
                    {
                        if (sigHeader.empty())
                        {
                            omnisphere::utils::Logger::LogError("StripeWebhook", req.TraceContext() + " Rejected: Missing Stripe-Signature header.");
                            return omnisphere::net::Response(400, "application/json", R"({"error":"Missing Stripe-Signature header"})");
                        }

                        std::string timestamp = "";
                        std::string signatureV1 = "";
                        std::stringstream ss(sigHeader);
                        std::string item;
                        while (std::getline(ss, item, ','))
                        {
                            size_t eqPos = item.find('=');
                            if (eqPos != std::string::npos)
                            {
                                std::string k = item.substr(0, eqPos);
                                std::string v = item.substr(eqPos + 1);
                                while (!k.empty() && k.front() == ' ') k.erase(k.begin());
                                if (k == "t") timestamp = v;
                                else if (k == "v1") signatureV1 = v;
                            }
                        }

                        if (timestamp.empty() || signatureV1.empty())
                        {
                            omnisphere::utils::Logger::LogError("StripeWebhook", req.TraceContext() + " Rejected: Invalid Stripe-Signature header format.");
                            return omnisphere::net::Response(400, "application/json", R"({"error":"Invalid Stripe-Signature header format"})");
                        }

                        std::string signedPayload = timestamp + "." + req.Body();
                        std::string computedSig = omnisphere::utils::Hasher::HmacSha256(signedPayload, webhookSecret);

                        if (computedSig != signatureV1)
                        {
                            omnisphere::utils::Logger::LogError("StripeWebhook", req.TraceContext() + " Rejected: HMAC-SHA256 signature mismatch.");
                            return omnisphere::net::Response(401, "application/json", R"({"error":"Invalid signature"})");
                        }

                        omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Signature verified successfully via HMAC-SHA256.");
                    }

                    auto parsed = req.Json();
                    if (parsed.is_object())
                    {
                        auto obj = parsed.as_object();
                        std::string eventType = "";
                        if (obj.contains("type") && obj.at("type").is_string())
                        {
                            eventType = std::string(obj.at("type").as_string());
                        }

                        omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Webhook Event Received: [" + eventType + "]");
                        omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Payload Body: " + req.Body());

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
                                    {
                                        std::string rawId = std::string(sessObj.at("id").as_string());
                                        if (rawId.rfind("cs_", 0) == 0) sessionId = rawId;
                                        else if (rawId.rfind("pi_", 0) == 0) paymentIntentId = rawId;
                                    }

                                    if (sessObj.contains("client_reference_id") && sessObj.at("client_reference_id").is_string())
                                        reservationCode = std::string(sessObj.at("client_reference_id").as_string());

                                    if (sessObj.contains("payment_intent") && sessObj.at("payment_intent").is_string())
                                        paymentIntentId = std::string(sessObj.at("payment_intent").as_string());

                                    // Extract order_reference (Stripe Checkout Session ID) if inside payment_details
                                    if (sessObj.contains("payment_details") && sessObj.at("payment_details").is_object())
                                    {
                                        auto pd = sessObj.at("payment_details").as_object();
                                        if (pd.contains("order_reference") && pd.at("order_reference").is_string())
                                        {
                                            sessionId = std::string(pd.at("order_reference").as_string());
                                        }
                                    }

                                    // Extract metadata if present
                                    if (sessObj.contains("metadata") && sessObj.at("metadata").is_object())
                                    {
                                        auto meta = sessObj.at("metadata").as_object();
                                        if (meta.contains("reservationCode") && meta.at("reservationCode").is_string())
                                            reservationCode = std::string(meta.at("reservationCode").as_string());
                                        else if (meta.contains("client_reference_id") && meta.at("client_reference_id").is_string())
                                            reservationCode = std::string(meta.at("client_reference_id").as_string());
                                    }

                                    if (sessObj.contains("amount_total") && sessObj.at("amount_total").is_number())
                                        amount = sessObj.at("amount_total").as_double() / 100.0;
                                    else if (sessObj.contains("amount") && sessObj.at("amount").is_number())
                                        amount = sessObj.at("amount").as_double() / 100.0;
                                }
                            }

                            omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Parsed Event -> SessionId: [" + sessionId + "], PaymentIntent: [" + paymentIntentId + "], ReservationCode: [" + reservationCode + "], Amount: $" + std::to_string(amount));

                            // Buscar la sesión registrada si reservationCode estaba vacío
                            if (reservationCode.empty() && !sessionId.empty())
                            {
                                omnisphere::utils::Logger::LogWarning("StripeWebhook", req.TraceContext() + " ReservationCode empty in event payload. Searching database for SessionId [" + sessionId + "]...");
                                auto sessOpt = repo->GetSessionByStripeId(sessionId);
                                if (sessOpt.has_value())
                                {
                                    reservationCode = sessOpt->reservationCode;
                                    omnisphere::utils::Logger::LogInfo("StripeWebhook", req.TraceContext() + " Successfully resolved ReservationCode [" + reservationCode + "] from database session.");
                                }
                                else
                                {
                                    omnisphere::utils::Logger::LogError("StripeWebhook", req.TraceContext() + " Could not find reservation in database for SessionId [" + sessionId + "].");
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
