#include "Payment/StripeWebhookHandler.hpp"
#include "Payment/Hooks/PaymentHook.hpp"
#include "License/Services/LicenseService.hpp"
#include <OmniUtils/Base64.hpp>
#include <OmniUtils/Hasher.hpp>
#include <OmniUtils/Logger.hpp>
#include <boost/json.hpp>
#include <sstream>
#include <iostream>

namespace json = boost::json;

namespace omnisphere::services
{
    StripeWebhookHandler::StripeWebhookHandler(
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
        StripePaymentHandler paymentCompletedHandler
    )
        : m_dbPool(std::move(dbPool)),
          m_paymentCompletedHandler(std::move(paymentCompletedHandler))
    {
        if (m_dbPool)
        {
            m_repo = std::make_shared<omnisphere::repositories::StripeRepository>(m_dbPool);
        }
    }

    std::string StripeWebhookHandler::RetrieveWebhookSecret() const
    {
        if (!m_repo) return "";
        try
        {
            auto dtSettings = m_repo->GetSettings();
            if (dtSettings.RowsCount() > 0 && dtSettings[0].HasColumn("WebhookSecretKey") && !dtSettings[0]["WebhookSecretKey"].IsNull())
            {
                std::string webhookSecret = (std::string)dtSettings[0]["WebhookSecretKey"];
                if (webhookSecret.rfind("whsec_", 0) != 0)
                {
                    try { webhookSecret = omnisphere::utils::Base64::Decode(webhookSecret); } catch (...) {}
                }
                return webhookSecret;
            }
        }
        catch (...) {}
        return "";
    }

    bool StripeWebhookHandler::VerifySignature(const omnisphere::net::Request& req, const std::string& webhookSecret) const
    {
        std::string sigHeader = req.Header("Stripe-Signature");
        if (sigHeader.empty()) sigHeader = req.Header("stripe-signature");

        if (sigHeader.empty())
        {
            omnisphere::utils::Logger::LogError("StripeWebhookHandler", req.TraceContext() + " Rejected: Missing Stripe-Signature header.");
            return false;
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
            omnisphere::utils::Logger::LogError("StripeWebhookHandler", req.TraceContext() + " Rejected: Invalid Stripe-Signature header format.");
            return false;
        }

        std::string signedPayload = timestamp + "." + req.Body();
        std::string computedSig = omnisphere::utils::Hasher::HmacSha256(signedPayload, webhookSecret);

        if (computedSig != signatureV1)
        {
            omnisphere::utils::Logger::LogError("StripeWebhookHandler", req.TraceContext() + " Rejected: HMAC-SHA256 signature mismatch.");
            return false;
        }

        omnisphere::utils::Logger::LogInfo("StripeWebhookHandler", req.TraceContext() + " Signature verified successfully via HMAC-SHA256.");
        return true;
    }

    omnisphere::net::Response StripeWebhookHandler::HandleWebhook(const omnisphere::net::Request& req) const
    {
        omnisphere::utils::Logger::LogHttpRequest(req);

        auto lic = omnisphere::services::LicenseService::GetSharedInstance();
        if (lic && !lic->IsModuleLicensed(omnisphere::license::MODULE_STRIPE))
        {
            omnisphere::utils::Logger::LogWarning("StripeWebhookHandler",
                req.TraceContext() + " Stripe Webhook ignored: Stripe integration is disabled by license.");
            return omnisphere::net::Response(403, "application/json", R"({"error":"Stripe integration is disabled by license"})");
        }

        std::string webhookSecret = RetrieveWebhookSecret();
        if (!webhookSecret.empty() && webhookSecret.rfind("whsec_", 0) == 0)
        {
            if (!VerifySignature(req, webhookSecret))
            {
                return omnisphere::net::Response(401, "application/json", R"({"error":"Invalid signature"})");
            }
        }

        omnisphere::utils::Logger::LogInfo("StripeWebhookHandler",
            req.TraceContext() + " Enqueueing Stripe Webhook Event to [StripeWorkerThread]...");

        m_stripeWorker.Enqueue([this, req]() {
            ProcessWebhookAsync(req);
        });

        return omnisphere::net::Response::Json(boost::json::object{{"received", true}});
    }

    void StripeWebhookHandler::ProcessWebhookAsync(omnisphere::net::Request req) const
    {
        try
        {
            auto parsed = req.Json();
            if (parsed.is_object())
            {
                auto obj = parsed.as_object();
                std::string eventType = obj.contains("type") && obj.at("type").is_string() ? std::string(obj.at("type").as_string()) : "";
                std::string eventId = obj.contains("id") && obj.at("id").is_string() ? std::string(obj.at("id").as_string()) : "";

                omnisphere::utils::Logger::LogInfo("StripeWebhookHandler",
                    req.TraceContext() + " [StripeWorkerThread] Processing Event: [" + eventType + "] (ID: " + eventId + ")");

                bool isSuccessEvent = (eventType == "checkout.session.completed" ||
                                       eventType == "checkout.session.async_payment_succeeded" ||
                                       eventType == "payment_intent.succeeded" ||
                                       eventType == "charge.succeeded");

                bool isFailedEvent = (eventType == "checkout.session.async_payment_failed" ||
                                      eventType == "payment_intent.payment_failed" ||
                                      eventType == "payment_intent.canceled" ||
                                      eventType == "charge.failed" ||
                                      eventType == "charge.dispute.created" ||
                                      eventType == "radar.early_fraud_warning.created" ||
                                      eventType == "review.opened" ||
                                      eventType == "review.closed");

                bool isExpiredEvent = (eventType == "checkout.session.expired");

                if (isSuccessEvent || isFailedEvent || isExpiredEvent)
                {
                    std::string sessionId = "";
                    std::string reservationCode = "";
                    std::string paymentIntentId = "";
                    double amount = 0.0;
                    std::string currency = "mxn";

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

                            if (sessObj.contains("payment_intent") && sessObj.at("payment_intent").is_string())
                            {
                                paymentIntentId = std::string(sessObj.at("payment_intent").as_string());
                            }

                            if (sessObj.contains("currency") && sessObj.at("currency").is_string())
                            {
                                currency = std::string(sessObj.at("currency").as_string());
                            }

                            if (sessObj.contains("client_reference_id") && sessObj.at("client_reference_id").is_string())
                            {
                                reservationCode = std::string(sessObj.at("client_reference_id").as_string());
                            }

                            if (sessObj.contains("payment_details") && sessObj.at("payment_details").is_object())
                            {
                                auto pd = sessObj.at("payment_details").as_object();
                                if (pd.contains("order_reference") && pd.at("order_reference").is_string())
                                {
                                    sessionId = std::string(pd.at("order_reference").as_string());
                                }
                            }

                            if (reservationCode.empty() && sessObj.contains("metadata") && sessObj.at("metadata").is_object())
                            {
                                auto meta = sessObj.at("metadata").as_object();
                                if (meta.contains("ReservationCode") && meta.at("ReservationCode").is_string())
                                    reservationCode = std::string(meta.at("ReservationCode").as_string());
                                else if (meta.contains("reservationCode") && meta.at("reservationCode").is_string())
                                    reservationCode = std::string(meta.at("reservationCode").as_string());
                                else if (meta.contains("EntityCode") && meta.at("EntityCode").is_string())
                                    reservationCode = std::string(meta.at("EntityCode").as_string());
                                else if (meta.contains("client_reference_id") && meta.at("client_reference_id").is_string())
                                    reservationCode = std::string(meta.at("client_reference_id").as_string());
                            }

                            auto getDoubleFromValue = [](const boost::json::value& v) -> double {
                                if (v.is_double()) return v.as_double();
                                if (v.is_int64()) return static_cast<double>(v.as_int64());
                                if (v.is_uint64()) return static_cast<double>(v.as_uint64());
                                return 0.0;
                            };

                            if (sessObj.contains("amount_total") && sessObj.at("amount_total").is_number())
                                amount = getDoubleFromValue(sessObj.at("amount_total")) / 100.0;
                            else if (sessObj.contains("amount_received") && sessObj.at("amount_received").is_number())
                                amount = getDoubleFromValue(sessObj.at("amount_received")) / 100.0;
                            else if (sessObj.contains("amount") && sessObj.at("amount").is_number())
                                amount = getDoubleFromValue(sessObj.at("amount")) / 100.0;
                        }
                    }

                    // Fallback para resolver reservationCode desde la BD si no vino en el webhook
                    if (reservationCode.empty() && m_repo)
                    {
                        if (!sessionId.empty())
                        {
                            omnisphere::utils::Logger::LogWarning("StripeWebhookHandler", req.TraceContext() + " [StripeWorkerThread] ReservationCode empty in event payload. Searching database for SessionId [" + sessionId + "]...");
                            auto sessOpt = m_repo->GetSessionByStripeId(sessionId);
                            if (sessOpt.has_value() && !sessOpt->reservationCode.empty())
                            {
                                reservationCode = sessOpt->reservationCode;
                                omnisphere::utils::Logger::LogInfo("StripeWebhookHandler", req.TraceContext() + " [StripeWorkerThread] Successfully resolved ReservationCode [" + reservationCode + "] from database session.");
                            }
                        }
                        if (reservationCode.empty() && !paymentIntentId.empty())
                        {
                            omnisphere::utils::Logger::LogWarning("StripeWebhookHandler", req.TraceContext() + " [StripeWorkerThread] ReservationCode empty in event payload. Searching database for PaymentIntentId [" + paymentIntentId + "]...");
                            auto txOpt = m_repo->GetTransactionByPaymentIntent(paymentIntentId);
                            if (txOpt.has_value() && !txOpt->reservationCode.empty())
                            {
                                reservationCode = txOpt->reservationCode;
                                omnisphere::utils::Logger::LogInfo("StripeWebhookHandler", req.TraceContext() + " [StripeWorkerThread] Successfully resolved ReservationCode [" + reservationCode + "] from database transaction.");
                            }
                        }
                    }

                    omnisphere::payment::PaymentEvent hookEvent;
                    hookEvent.eventId = eventId;
                    hookEvent.entityType = "ROUTE_RESERVATION";
                    hookEvent.entityCode = reservationCode;
                    hookEvent.paymentIntentId = paymentIntentId;
                    hookEvent.sessionId = sessionId;
                    hookEvent.provider = "STRIPE";
                    hookEvent.amount = amount;
                    hookEvent.currency = currency;

                    if (isSuccessEvent)
                    {
                        omnisphere::utils::Logger::LogInfo("StripeWebhookHandler",
                            req.TraceContext() + " [StripeWorkerThread] Payment SUCCEEDED for EntityCode: [" + reservationCode +
                            "], SessionID: [" + sessionId + "], PaymentIntentID: [" + paymentIntentId +
                            "], Amount: $" + std::to_string(amount) + " MXN");

                        if (m_repo)
                        {
                            if (!sessionId.empty()) m_repo->UpdateSessionStatus(sessionId, "complete", paymentIntentId);
                            if (!paymentIntentId.empty()) m_repo->UpdateTransactionStatus(paymentIntentId, "succeeded");
                        }

                        hookEvent.eventType = "PAYMENT_COMPLETED";
                        omnisphere::payment::PaymentHookRegistry::Instance().DispatchCompleted(hookEvent);

                        if (m_paymentCompletedHandler)
                        {
                            m_paymentCompletedHandler(req, sessionId, reservationCode, paymentIntentId, amount);
                        }
                    }
                    else if (isFailedEvent)
                    {
                        omnisphere::utils::Logger::LogWarning("StripeWebhookHandler",
                            req.TraceContext() + " [StripeWorkerThread] Payment FAILED/DECLINED/FRAUDULENT [" + eventType + "] for EntityCode: [" + reservationCode +
                            "], SessionID: [" + sessionId + "], PaymentIntentID: [" + paymentIntentId + "]");

                        if (m_repo)
                        {
                            if (!sessionId.empty()) m_repo->UpdateSessionStatus(sessionId, "failed", paymentIntentId);
                            if (!paymentIntentId.empty()) m_repo->UpdateTransactionStatus(paymentIntentId, "failed");
                        }

                        hookEvent.eventType = "PAYMENT_FAILED";
                        omnisphere::payment::PaymentHookRegistry::Instance().DispatchFailed(hookEvent);
                    }
                    else if (isExpiredEvent)
                    {
                        omnisphere::utils::Logger::LogWarning("StripeWebhookHandler",
                            req.TraceContext() + " [StripeWorkerThread] Checkout Session EXPIRED for EntityCode: [" + reservationCode +
                            "], SessionID: [" + sessionId + "]");

                        if (m_repo)
                        {
                            if (!sessionId.empty()) m_repo->UpdateSessionStatus(sessionId, "expired", paymentIntentId);
                        }

                        hookEvent.eventType = "PAYMENT_EXPIRED";
                        omnisphere::payment::PaymentHookRegistry::Instance().DispatchExpired(hookEvent);
                    }
                }
            }
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("StripeWebhookHandler",
                req.TraceContext() + " [StripeWorkerThread] Exception processing Stripe webhook: " + ex.what());
            std::cerr << "[StripeWebhookHandler Error - StripeWorkerThread] " << ex.what() << std::endl;
        }
    }
} // namespace omnisphere::services
