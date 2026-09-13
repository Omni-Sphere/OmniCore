#include "Payment/Providers/StripePaymentProvider.hpp"
#include <OmniUtils/Base64.hpp>
#include <OmniUtils/Hasher.hpp>
#include <OmniUtils/Logger.hpp>
#include <boost/json.hpp>
#include <iostream>

namespace json = boost::json;

namespace omnisphere::payment
{
    StripePaymentProvider::StripePaymentProvider(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(dbPool),
          m_stripeService(std::make_shared<omnisphere::services::StripeService>(dbPool)) {}

    StripePaymentProvider::StripePaymentProvider(
        std::shared_ptr<omnisphere::services::StripeService> stripeService,
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_stripeService(std::move(stripeService)), m_dbPool(std::move(dbPool)) {}

    ProviderPaymentIntentResult StripePaymentProvider::CreatePaymentIntent(const omnisphere::models::PayableEntity& entity)
    {
        if (!m_stripeService) return {false, "", "", "", "StripeService is null"};
        omnisphere::models::SecurityContext secCtx;
        secCtx.userCode = "system";

        std::string targetCode = entity.entityCode.empty() ? "PAY-" + std::to_string(std::time(nullptr)) : entity.entityCode;
        auto res = m_stripeService->CreatePaymentIntent(secCtx, targetCode, entity.amount, entity.currency);

        return {
            res.success,
            res.clientSecret,
            res.publishableKey,
            res.paymentIntentId,
            res.errorMessage
        };
    }

    ProviderBankTransferResult StripePaymentProvider::CreateBankTransfer(const omnisphere::models::PayableEntity& entity)
    {
        if (!m_stripeService) return {false, "", "", "", "", 0.0, "mxn", "StripeService is null"};
        omnisphere::models::SecurityContext secCtx;
        secCtx.userCode = "system";

        std::string targetCode = entity.entityCode.empty() ? "PAY-" + std::to_string(std::time(nullptr)) : entity.entityCode;
        auto res = m_stripeService->CreateBankTransferPaymentIntent(secCtx, targetCode, entity.amount, entity.customerName, entity.customerEmail);

        return {
            res.success,
            res.paymentIntentId,
            res.clabe,
            res.bankName,
            res.hostedInstructionsUrl,
            res.amount,
            res.currency,
            res.errorMessage
        };
    }

    ProviderCheckoutResult StripePaymentProvider::CreateCheckoutSession(const omnisphere::models::PayableEntity& entity)
    {
        if (!m_stripeService) return {false, "", "", "StripeService is null"};
        omnisphere::models::SecurityContext secCtx;
        secCtx.userCode = "system";

        std::string targetCode = entity.entityCode.empty() ? "PAY-" + std::to_string(std::time(nullptr)) : entity.entityCode;
        auto res = m_stripeService->CreateCheckoutSession(secCtx, targetCode, entity.amount, entity.seats, entity.successUrl, entity.cancelUrl);

        return {
            res.success,
            res.checkoutUrl,
            res.sessionId,
            res.errorMessage
        };
    }

    ProviderDiagnosticResult StripePaymentProvider::TestIntegration()
    {
        if (!m_stripeService) return {false, false, "StripeService not initialized"};
        omnisphere::models::SecurityContext secCtx;
        secCtx.userCode = "system";
        auto diag = m_stripeService->TestIntegration(secCtx);
        return {
            diag.isConfigured,
            diag.isFunctional,
            diag.message
        };
    }

    bool StripePaymentProvider::CancelPayment(const std::string& transactionOrReferenceId, const std::string& reason)
    {
        if (transactionOrReferenceId.empty()) return false;
        if (!m_stripeService)
        {
            if (m_dbPool)
            {
                m_stripeService = std::make_shared<omnisphere::services::StripeService>(m_dbPool);
            }
            else
            {
                return false;
            }
        }

        if (transactionOrReferenceId.rfind("cs_", 0) == 0)
        {
            return m_stripeService->ExpireCheckoutSession(transactionOrReferenceId);
        }
        else
        {
            return m_stripeService->CancelPaymentIntent(transactionOrReferenceId, reason);
        }
    }

    bool StripePaymentProvider::VerifyWebhookSignature(const omnisphere::net::Request& req) const
    {
        if (!m_dbPool) return true;
        auto repo = std::make_shared<omnisphere::repositories::StripeRepository>(m_dbPool);
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

        if (webhookSecret.empty() || webhookSecret.rfind("whsec_", 0) != 0)
        {
            return true; // Si no hay secreto configurado, se permite en modo desarrollo
        }

        std::string sigHeader = req.Header("Stripe-Signature");
        if (sigHeader.empty()) sigHeader = req.Header("stripe-signature");
        if (sigHeader.empty()) return false;

        std::string timestampStr = "";
        std::vector<std::string> signaturesV1;
        std::stringstream ss(sigHeader);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            auto eqPos = item.find('=');
            if (eqPos != std::string::npos)
            {
                std::string k = item.substr(0, eqPos);
                std::string v = item.substr(eqPos + 1);
                while (!k.empty() && k.front() == ' ') k.erase(0, 1);
                if (k == "t") timestampStr = v;
                else if (k == "v1") signaturesV1.push_back(v);
            }
        }

        if (timestampStr.empty() || signaturesV1.empty()) return false;
        std::string payloadToSign = timestampStr + "." + req.Body();
        std::string expectedSig = omnisphere::utils::Hasher::HmacSha256(payloadToSign, webhookSecret);

        for (const auto& sig : signaturesV1)
        {
            if (sig == expectedSig) return true;
        }
        return false;
    }

    std::optional<PaymentEvent> StripePaymentProvider::ParseWebhookEvent(const omnisphere::net::Request& req) const
    {
        try
        {
            auto rootObj = json::parse(req.Body()).as_object();
            std::string eventType = rootObj.contains("type") ? json::value_to<std::string>(rootObj["type"]) : "";
            std::string eventId = rootObj.contains("id") ? json::value_to<std::string>(rootObj["id"]) : "";

            std::string normalizedEventType = "";
            if (eventType == "payment_intent.succeeded" ||
                eventType == "checkout.session.completed" ||
                eventType == "checkout.session.async_payment_succeeded" ||
                eventType == "charge.succeeded")
            {
                normalizedEventType = "PAYMENT_COMPLETED";
            }
            else if (eventType == "checkout.session.async_payment_failed" ||
                     eventType == "payment_intent.payment_failed" ||
                     eventType == "payment_intent.canceled" ||
                     eventType == "charge.failed" ||
                     eventType == "charge.dispute.created" ||
                     eventType == "radar.early_fraud_warning.created" ||
                     eventType == "review.opened" ||
                     eventType == "review.closed")
            {
                normalizedEventType = "PAYMENT_FAILED";
            }
            else if (eventType == "checkout.session.expired")
            {
                normalizedEventType = "PAYMENT_EXPIRED";
            }
            else
            {
                return std::nullopt;
            }

            PaymentEvent evt;
            evt.eventId = eventId;
            evt.eventType = normalizedEventType;
            evt.provider = "STRIPE";

            if (rootObj.contains("data") && rootObj["data"].is_object())
            {
                auto dataObj = rootObj["data"].as_object();
                if (dataObj.contains("object") && dataObj["object"].is_object())
                {
                    auto obj = dataObj["object"].as_object();

                    // 1. PaymentIntent / Session
                    if (obj.contains("id")) evt.paymentIntentId = json::value_to<std::string>(obj["id"]);
                    if (obj.contains("currency")) evt.currency = json::value_to<std::string>(obj["currency"]);
                    if (obj.contains("amount"))
                    {
                        double rawAmount = obj["amount"].is_double() ? obj["amount"].as_double() : static_cast<double>(obj["amount"].as_int64());
                        evt.amount = rawAmount / 100.0;
                    }

                    // 2. Metadata: entityType, entityCode (or fallback reservationCode)
                    if (obj.contains("metadata") && obj["metadata"].is_object())
                    {
                        auto meta = obj["metadata"].as_object();
                        evt.metadata = meta;

                        if (meta.contains("entityType")) evt.entityType = json::value_to<std::string>(meta["entityType"]);
                        else if (meta.contains("EntityType")) evt.entityType = json::value_to<std::string>(meta["EntityType"]);
                        else evt.entityType = "ROUTE_RESERVATION";

                        if (meta.contains("entityCode")) evt.entityCode = json::value_to<std::string>(meta["entityCode"]);
                        else if (meta.contains("EntityCode")) evt.entityCode = json::value_to<std::string>(meta["EntityCode"]);
                        else if (meta.contains("reservationCode")) evt.entityCode = json::value_to<std::string>(meta["reservationCode"]);
                        else if (meta.contains("ReservationCode")) evt.entityCode = json::value_to<std::string>(meta["ReservationCode"]);
                        else if (meta.contains("client_reference_id")) evt.entityCode = json::value_to<std::string>(meta["client_reference_id"]);
                    }
                    else
                    {
                        evt.entityType = "ROUTE_RESERVATION";
                    }

                    // Fallback para resolver entityCode desde base de datos si no vino en metadata
                    if (evt.entityCode.empty() && !evt.paymentIntentId.empty() && m_dbPool)
                    {
                        try
                        {
                            auto repo = std::make_shared<omnisphere::repositories::StripeRepository>(m_dbPool);
                            auto txOpt = repo->GetTransactionByPaymentIntent(evt.paymentIntentId);
                            if (txOpt.has_value() && !txOpt->reservationCode.empty())
                            {
                                evt.entityCode = txOpt->reservationCode;
                            }
                        }
                        catch (...) {}
                    }

                    // 3. Payment Method detection (Customer Balance / SPEI vs Card)
                    evt.paymentMethod = "CARD";
                    if (obj.contains("payment_method_types") && obj["payment_method_types"].is_array())
                    {
                        auto arr = obj["payment_method_types"].as_array();
                        if (!arr.empty())
                        {
                            std::string firstType = json::value_to<std::string>(arr[0]);
                            if (firstType == "customer_balance") evt.paymentMethod = "TRANSFER";
                        }
                    }

                    if (obj.contains("customer_details") && obj["customer_details"].is_object())
                    {
                        auto cust = obj["customer_details"].as_object();
                        if (cust.contains("email") && !cust["email"].is_null()) evt.customerEmail = json::value_to<std::string>(cust["email"]);
                        if (cust.contains("name") && !cust["name"].is_null()) evt.customerName = json::value_to<std::string>(cust["name"]);
                        if (cust.contains("phone") && !cust["phone"].is_null()) evt.customerPhone = json::value_to<std::string>(cust["phone"]);
                    }

                    return evt;
                }
            }
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("StripePaymentProvider", std::string("Error parsing webhook: ") + ex.what());
        }
        return std::nullopt;
    }
} // namespace omnisphere::payment
