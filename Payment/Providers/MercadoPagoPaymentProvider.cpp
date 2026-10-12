#include "Payment/Providers/MercadoPagoPaymentProvider.hpp"
#include <OmniUtils/Hasher.hpp>
#include <OmniUtils/Logger.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <sstream>

namespace json = boost::json;

namespace omnisphere::payment
{
    MercadoPagoPaymentProvider::MercadoPagoPaymentProvider(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(dbPool),
          m_service(std::make_shared<omnisphere::services::MercadoPagoService>(dbPool)) {}

    MercadoPagoPaymentProvider::MercadoPagoPaymentProvider(
        std::shared_ptr<omnisphere::services::MercadoPagoService> service,
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool
    )
        : m_service(std::move(service)), m_dbPool(std::move(dbPool)) {}

    ProviderPaymentIntentResult MercadoPagoPaymentProvider::CreatePaymentIntent(const omnisphere::models::PayableEntity& entity)
    {
        if (!m_service) return {false, "", "", "", "MercadoPagoService no inicializado"};

        omnisphere::models::SecurityContext secCtx;
        secCtx.userCode = "system";

        auto settingsOpt = m_service->GetSettings(true);
        std::string pubKey = settingsOpt.has_value() ? settingsOpt->publicKey : "";

        std::string token = "";
        std::string paymentMethodId = "visa";
        std::string issuerId = "";
        int installments = 1;

        if (entity.metadata.contains("token") && entity.metadata.at("token").is_string())
        {
            token = json::value_to<std::string>(entity.metadata.at("token"));
        }
        if (entity.metadata.contains("paymentMethodId") && entity.metadata.at("paymentMethodId").is_string())
        {
            paymentMethodId = json::value_to<std::string>(entity.metadata.at("paymentMethodId"));
        }
        if (entity.metadata.contains("issuerId") && entity.metadata.at("issuerId").is_string())
        {
            issuerId = json::value_to<std::string>(entity.metadata.at("issuerId"));
        }
        if (entity.metadata.contains("installments") && entity.metadata.at("installments").is_int64())
        {
            installments = static_cast<int>(entity.metadata.at("installments").as_int64());
        }

        // Si se envió un token de tarjeta desde el front (Checkout API), procesar el cobro inmediatamente
        if (!token.empty())
        {
            auto res = m_service->CreatePayment(secCtx, entity, token, paymentMethodId, issuerId, installments);
            return {
                res.success,
                res.status,
                pubKey,
                res.paymentId,
                res.errorMessage
            };
        }

        // Si aún no viene token, devolver la llave pública para que el front inicialice el formulario/tokenización
        return {
            true,
            "ready_for_token",
            pubKey,
            "mp_intent_" + entity.entityCode,
            ""
        };
    }

    ProviderBankTransferResult MercadoPagoPaymentProvider::CreateBankTransfer(const omnisphere::models::PayableEntity& entity)
    {
        if (!m_service) return {false, "", "", "", "", 0.0, "mxn", "MercadoPagoService no inicializado"};

        omnisphere::models::SecurityContext secCtx;
        secCtx.userCode = "system";

        std::string method = "spei";
        if (entity.metadata.contains("paymentMethodId") && entity.metadata.at("paymentMethodId").is_string())
        {
            method = json::value_to<std::string>(entity.metadata.at("paymentMethodId"));
        }

        auto res = m_service->CreateBankTransfer(secCtx, entity, method);
        return {
            res.success,
            res.paymentId,
            res.clabe,
            "Mercado Pago / STP",
            res.ticketUrl,
            entity.amount,
            entity.currency,
            res.errorMessage
        };
    }

    ProviderCheckoutResult MercadoPagoPaymentProvider::CreateCheckoutSession(const omnisphere::models::PayableEntity& entity)
    {
        if (!m_service) return {false, "", "", "MercadoPagoService no inicializado"};

        omnisphere::models::SecurityContext secCtx;
        secCtx.userCode = "system";

        auto res = m_service->CreatePreference(secCtx, entity);
        return {
            res.success,
            res.checkoutUrl,
            res.preferenceId,
            res.errorMessage
        };
    }

    ProviderDiagnosticResult MercadoPagoPaymentProvider::TestIntegration()
    {
        if (!m_service) return {false, false, "MercadoPagoService no inicializado"};

        omnisphere::models::SecurityContext secCtx;
        secCtx.userCode = "system";

        auto diag = m_service->TestIntegration(secCtx);
        return {
            diag.isConfigured,
            diag.isFunctional,
            diag.message
        };
    }

    bool MercadoPagoPaymentProvider::CancelPayment(const std::string& /*transactionOrReferenceId*/, const std::string& /*reason*/)
    {
        return false;
    }

    bool MercadoPagoPaymentProvider::VerifyWebhookSignature(const omnisphere::net::Request& req) const
    {
        if (!m_service) return false;
        auto settingsOpt = m_service->GetSettings(true);
        if (!settingsOpt.has_value()) return false;

        std::string secret = settingsOpt->webhookSecretKey;
        if (secret.empty())
        {
            // En entornos de prueba sin webhook secret configurado, permitir tráfico
            return true;
        }

        std::string sigHeader = req.Header("x-signature");
        if (sigHeader.empty()) sigHeader = req.Header("X-Signature");
        if (sigHeader.empty()) return false;

        std::string ts = "";
        std::string hashV1 = "";

        std::stringstream ss(sigHeader);
        std::string part;
        while (std::getline(ss, part, ','))
        {
            auto eq = part.find('=');
            if (eq != std::string::npos)
            {
                std::string k = part.substr(0, eq);
                std::string v = part.substr(eq + 1);
                while (!k.empty() && k.front() == ' ') k.erase(0, 1);
                if (k == "ts") ts = v;
                else if (k == "v1") hashV1 = v;
            }
        }

        if (ts.empty() || hashV1.empty()) return false;

        std::string requestId = req.Header("x-request-id");
        if (requestId.empty()) requestId = req.Header("X-Request-Id");

        // Formato oficial de firma Mercado Pago:
        // "id:[data_id];request-id:[x-request-id];ts:[ts];"
        std::string dataId = "";
        try {
            auto bodyObj = json::parse(req.Body()).as_object();
            if (bodyObj.contains("data") && bodyObj["data"].is_object()) {
                auto dataObj = bodyObj["data"].as_object();
                if (dataObj.contains("id")) {
                    dataId = dataObj["id"].is_string() ? json::value_to<std::string>(dataObj["id"]) : std::to_string(dataObj["id"].as_int64());
                }
            }
        } catch (...) {}

        std::string manifest = "id:" + dataId + ";request-id:" + requestId + ";ts:" + ts + ";";
        std::string calculated = omnisphere::utils::Hasher::HmacSha256(manifest, secret);

        if (calculated == hashV1) return true;

        // Fallback: verificar con body completo si no coincidió el manifest
        std::string fallbackManifest = ts + "." + req.Body();
        return omnisphere::utils::Hasher::HmacSha256(fallbackManifest, secret) == hashV1;
    }

    std::optional<PaymentEvent> MercadoPagoPaymentProvider::ParseWebhookEvent(const omnisphere::net::Request& req) const
    {
        try
        {
            auto rootObj = json::parse(req.Body()).as_object();
            std::string action = rootObj.contains("action") ? json::value_to<std::string>(rootObj["action"]) : "";
            std::string type = rootObj.contains("type") ? json::value_to<std::string>(rootObj["type"]) : "";

            std::string paymentId = "";
            if (rootObj.contains("data") && rootObj["data"].is_object())
            {
                auto dataObj = rootObj["data"].as_object();
                if (dataObj.contains("id"))
                {
                    paymentId = dataObj["id"].is_string() ? json::value_to<std::string>(dataObj["id"]) : std::to_string(dataObj["id"].as_int64());
                }
            }

            if (paymentId.empty()) return std::nullopt;

            PaymentEvent evt;
            evt.eventId = rootObj.contains("id") ? (rootObj["id"].is_string() ? json::value_to<std::string>(rootObj["id"]) : std::to_string(rootObj["id"].as_int64())) : paymentId;
            evt.paymentIntentId = paymentId;
            evt.provider = "MERCADOPAGO";
            evt.entityType = "ROUTE_RESERVATION";

            // Consultar el estado real y detalles desde la API de Mercado Pago
            if (m_service)
            {
                auto mpDataOpt = m_service->GetPaymentFromMP(paymentId);
                if (mpDataOpt.has_value())
                {
                    const auto& mpObj = *mpDataOpt;
                    std::string status = mpObj.contains("status") ? json::value_to<std::string>(mpObj.at("status")) : "";
                    std::string statusDetail = mpObj.contains("status_detail") ? json::value_to<std::string>(mpObj.at("status_detail")) : "";

                    if (status == "approved")
                    {
                        evt.eventType = "PAYMENT_COMPLETED";
                    }
                    else if (status == "rejected" || status == "cancelled")
                    {
                        evt.eventType = "PAYMENT_FAILED";
                    }
                    else
                    {
                        return std::nullopt; // pending o in_process
                    }

                    if (mpObj.contains("external_reference") && !mpObj.at("external_reference").is_null())
                    {
                        evt.entityCode = json::value_to<std::string>(mpObj.at("external_reference"));
                    }

                    if (mpObj.contains("transaction_amount"))
                    {
                        evt.amount = mpObj.at("transaction_amount").is_double() ? mpObj.at("transaction_amount").as_double() : static_cast<double>(mpObj.at("transaction_amount").as_int64());
                    }

                    if (mpObj.contains("currency_id"))
                    {
                        evt.currency = json::value_to<std::string>(mpObj.at("currency_id"));
                    }

                    if (mpObj.contains("payment_type_id"))
                    {
                        std::string pType = json::value_to<std::string>(mpObj.at("payment_type_id"));
                        evt.paymentMethod = (pType == "bank_transfer" || pType == "ticket") ? "TRANSFER" : "CARD";
                    }

                    if (mpObj.contains("payer") && mpObj.at("payer").is_object())
                    {
                        auto payer = mpObj.at("payer").as_object();
                        if (payer.contains("email") && !payer.at("email").is_null()) evt.customerEmail = json::value_to<std::string>(payer.at("email"));
                    }

                    // Actualizar estado en repositorio local
                    if (m_dbPool)
                    {
                        auto repo = std::make_shared<omnisphere::repositories::MercadoPagoRepository>(m_dbPool);
                        repo->UpdateTransactionStatus(paymentId, status, statusDetail);
                    }
                }
            }

            return evt;
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("MercadoPagoPaymentProvider", std::string("Error parsing webhook: ") + ex.what());
        }
        return std::nullopt;
    }
} // namespace omnisphere::payment
