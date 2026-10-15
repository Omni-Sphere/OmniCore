#include "Payment/MercadoPagoService.hpp"
#include <OmniData/DataMapper.hpp>
#include <OmniUtils/Base64.hpp>
#include <OmniUtils/Logger.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <sstream>
#include <chrono>
#include <random>

namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = boost::asio::ssl;
namespace json = boost::json;
using tcp = boost::asio::ip::tcp;

namespace omnisphere::services
{
    MercadoPagoService::MercadoPagoService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(dbPool)
    {
        if (dbPool)
        {
            m_repository = std::make_shared<omnisphere::repositories::MercadoPagoRepository>(dbPool);
        }
    }

    MercadoPagoService::MercadoPagoService(std::shared_ptr<omnisphere::repositories::MercadoPagoRepository> repository)
        : m_repository(std::move(repository)) {}

    std::optional<omnisphere::models::MercadoPagoSettings> MercadoPagoService::GetSettings(bool decryptKeys) const
    {
        if (!m_repository) return std::nullopt;
        auto dt = m_repository->GetSettings();
        if (dt.IsEmpty())
        {
            omnisphere::models::MercadoPagoSettings emptySettings;
            emptySettings.entry = 0;
            emptySettings.code = "";
            emptySettings.name = "";
            emptySettings.publicKey = "";
            emptySettings.accessToken = "";
            emptySettings.webhookSecretKey = "";
            emptySettings.apiBaseUrl = "https://api.mercadopago.com";
            emptySettings.webhookPath = "/api/v1/mercadopago/webhook";
            emptySettings.currency = "mxn";
            emptySettings.isTestMode = true;
            emptySettings.isActive = false;
            return emptySettings;
        }

        auto s = omnisphere::types::FromDataRow<omnisphere::models::MercadoPagoSettings>(dt[0]);

        if (decryptKeys)
        {
            auto decryptVal = [](const std::string& val) -> std::string {
                if (val.empty()) return "";
                try {
                    return omnisphere::utils::Base64::Decode(val);
                } catch (...) {
                    return val;
                }
            };
            s.publicKey = decryptVal(s.publicKey);
            s.accessToken = decryptVal(s.accessToken);
            s.webhookSecretKey = decryptVal(s.webhookSecretKey);
        }
        return s;
    }

    bool MercadoPagoService::SaveSettings(const omnisphere::models::MercadoPagoSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_repository) return false;

        omnisphere::models::MercadoPagoSettings toSave = settings;

        // Validación en el servicio para decidir la operación del repositorio
        auto existingDt = m_repository->GetSettings({"Entry", "Code", "PublicKey", "AccessToken", "WebhookSecretKey"});
        omnisphere::models::MercadoPagoSettings existingSettings{};
        if (!existingDt.IsEmpty())
        {
            existingSettings = omnisphere::types::FromDataRow<omnisphere::models::MercadoPagoSettings>(existingDt[0]);
        }
        std::string existingPub = existingSettings.publicKey;
        std::string existingSec = existingSettings.accessToken;
        std::string existingWh = existingSettings.webhookSecretKey;

        auto encodeIfNeeded = [](const std::string& input, const std::string& existingVal) -> std::string {
            if (input.empty()) return "";
            if (!existingVal.empty() && input == existingVal) return existingVal;
            return omnisphere::utils::Base64::Encode(input);
        };

        toSave.publicKey = encodeIfNeeded(toSave.publicKey, existingPub);
        toSave.accessToken = encodeIfNeeded(toSave.accessToken, existingSec);
        toSave.webhookSecretKey = encodeIfNeeded(toSave.webhookSecretKey, existingWh);

        if (existingDt.IsEmpty())
        {
            if (toSave.code.empty()) toSave.code = "DEFAULT";
            return m_repository->Create(toSave, mutationFields);
        }
        else
        {
            toSave.code = existingSettings.code.empty() ? "DEFAULT" : existingSettings.code;
            return m_repository->Update(toSave, mutationFields);
        }
    }

    std::string MercadoPagoService::ExecuteHttpsRequest(
        const std::string& method,
        const std::string& target,
        const std::string& accessToken,
        const std::string& jsonBody,
        const std::string& idempotencyKey
    ) const
    {
        std::string host = "api.mercadopago.com";
        std::string port = "443";

        std::string maskedToken = accessToken.empty() ? "(vacío)" : (
            accessToken.size() > 14
                ? accessToken.substr(0, 10) + "..." + accessToken.substr(accessToken.size() - 4) + " (longitud: " + std::to_string(accessToken.size()) + ")"
                : accessToken + " (longitud: " + std::to_string(accessToken.size()) + ")"
        );

        try
        {
            boost::asio::io_context ioc;
            ssl::context sslCtx(ssl::context::tlsv12_client);
            sslCtx.set_default_verify_paths();
            sslCtx.set_verify_mode(ssl::verify_peer);

            tcp::resolver resolver(ioc);
            beast::ssl_stream<beast::tcp_stream> stream(ioc, sslCtx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
                throw beast::system_error{ec};
            }

            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            auto const results = resolver.resolve(tcp::v4(), host, port);
            beast::get_lowest_layer(stream).connect(results);
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            stream.handshake(ssl::stream_base::client);

            http::verb httpVerb = http::verb::get;
            if (method == "POST") httpVerb = http::verb::post;
            else if (method == "PUT") httpVerb = http::verb::put;
            else if (method == "DELETE") httpVerb = http::verb::delete_;

            http::request<http::string_body> req{httpVerb, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "OmniSphere-C++/1.0");
            req.set(http::field::content_type, "application/json");
            req.set(http::field::authorization, "Bearer " + accessToken);

            if (!idempotencyKey.empty())
            {
                req.set("X-Idempotency-Key", idempotencyKey);
            }

            if (!jsonBody.empty())
            {
                req.body() = jsonBody;
                req.prepare_payload();
            }

            // Enmascarar números de tarjeta o códigos de seguridad en logs de solicitud
            std::string logPayload = jsonBody;
            if (!logPayload.empty())
            {
                // Si contiene datos de tarjeta, ofuscar número y cvv
                try {
                    auto p = json::parse(logPayload);
                    if (p.is_object()) {
                        auto obj = p.as_object();
                        if (obj.contains("card_number") && obj["card_number"].is_string()) {
                            std::string cn = json::value_to<std::string>(obj["card_number"]);
                            std::string last4 = cn.size() >= 4 ? cn.substr(cn.size() - 4) : "XXXX";
                            obj["card_number"] = "**** **** **** " + last4;
                        }
                        if (obj.contains("security_code")) obj["security_code"] = "***";
                        logPayload = json::serialize(obj);
                    }
                } catch (...) {}
            }

            omnisphere::utils::Logger::LogInfo("MercadoPagoService",
                "--> " + method + " https://" + host + target + "\n" +
                "    [Headers] Authorization: Bearer " + maskedToken + 
                (idempotencyKey.empty() ? "" : (" | X-Idempotency-Key: " + idempotencyKey)) + "\n" +
                (logPayload.empty() ? "    [Body] (empty)" : ("    [Body] " + logPayload)));

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);

            if (res.result_int() >= 400)
            {
                std::string errLog = "<-- HTTP " + std::to_string(res.result_int()) + " " + std::string(res.reason()) + " [ERROR]\n"
                    + "    [Petición] " + method + " https://" + host + target + "\n"
                    + "    [Token] " + maskedToken + "\n"
                    + (logPayload.empty() ? "" : ("    [Body enviado] " + logPayload + "\n"))
                    + "    [Respuesta Mercado Pago] " + res.body();
                omnisphere::utils::Logger::LogError("MercadoPagoService", errLog);
            }
            else
            {
                std::string okLog = "<-- HTTP " + std::to_string(res.result_int()) + " " + std::string(res.reason()) + " [OK]\n"
                    + "    [Petición] " + method + " https://" + host + target + "\n"
                    + "    [Respuesta Mercado Pago] " + res.body();
                omnisphere::utils::Logger::LogInfo("MercadoPagoService", okLog);
            }

            return res.body();
        }
        catch (const std::exception& ex)
        {
            std::string failLog = "Fallo de comunicación en petición HTTPS a Mercado Pago:\n"
                + std::string("  -> Petición: ") + method + " https://" + host + target + "\n"
                + "  -> Token: " + maskedToken + "\n"
                + (jsonBody.empty() ? "" : ("  -> Body enviado: " + jsonBody + "\n"))
                + "  -> Error: " + ex.what();
            omnisphere::utils::Logger::LogError("MercadoPagoService", failLog);
            throw;
        }
    }

    MercadoPagoPaymentResult MercadoPagoService::ProcessCardPayment(
        const omnisphere::models::SecurityContext& ctx,
        const std::string& cardNumber,
        const std::string& cardholderName,
        int expirationMonth,
        int expirationYear,
        const std::string& securityCode,
        double amount,
        const std::string& currency,
        const std::string& customerEmail,
        const std::string& description
    ) const
    {
        MercadoPagoPaymentResult result;
        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            result.errorMessage = "Mercado Pago no está configurado o está inactivo.";
            return result;
        }

        const auto& settings = *settingsOpt;
        if (settings.accessToken.empty())
        {
            result.errorMessage = "Access Token de Mercado Pago no configurado.";
            return result;
        }

        try
        {
            // 1. Limpieza y formato de tarjeta
            std::string cleanCard = "";
            for (char c : cardNumber) { if (std::isdigit(static_cast<unsigned char>(c))) cleanCard += c; }

            if (cleanCard.size() >= 4) {
                result.cardLast4 = cleanCard.substr(cleanCard.size() - 4);
            }

            std::string detectedBrand = "visa";
            if (!cleanCard.empty()) {
                if (cleanCard[0] == '4') detectedBrand = "visa";
                else if (cleanCard[0] == '5' || (cleanCard.size() >= 2 && cleanCard.substr(0, 2) == "22")) detectedBrand = "master";
                else if (cleanCard.size() >= 2 && (cleanCard.substr(0, 2) == "34" || cleanCard.substr(0, 2) == "37")) detectedBrand = "amex";
            }
            result.cardBrand = detectedBrand;

            // 2. Tokenizar la tarjeta directamente contra Mercado Pago API
            json::object tokenReq;
            tokenReq["card_number"] = cleanCard;
            json::object ch;
            ch["name"] = cardholderName.empty() ? "Cliente" : cardholderName;
            tokenReq["cardholder"] = ch;
            tokenReq["expiration_month"] = expirationMonth;
            tokenReq["expiration_year"] = expirationYear < 100 ? (2000 + expirationYear) : expirationYear;
            tokenReq["security_code"] = securityCode;

            std::string tokenBody = json::serialize(tokenReq);
            std::string tokenRes = ExecuteHttpsRequest("POST", "/v1/card_tokens", settings.accessToken, tokenBody);

            auto parsedToken = json::parse(tokenRes);
            if (!parsedToken.is_object() || !parsedToken.as_object().contains("id"))
            {
                std::string err = "Error al validar la tarjeta en Mercado Pago.";
                if (parsedToken.is_object())
                {
                    auto obj = parsedToken.as_object();
                    if (obj.contains("message")) err = json::value_to<std::string>(obj["message"]);
                    else if (obj.contains("cause") && obj["cause"].is_array() && !obj["cause"].as_array().empty())
                    {
                        auto c0 = obj["cause"].as_array()[0];
                        if (c0.is_object() && c0.as_object().contains("description"))
                            err = json::value_to<std::string>(c0.as_object()["description"]);
                    }
                }
                result.errorMessage = err;
                return result;
            }

            auto tokenObj = parsedToken.as_object();
            std::string token = json::value_to<std::string>(tokenObj["id"]);
            if (tokenObj.contains("last_four_digits"))
            {
                result.cardLast4 = json::value_to<std::string>(tokenObj["last_four_digits"]);
            }

            // 3. Ejecutar el cobro directo mediante CreatePayment
            omnisphere::models::PayableEntity entity;
            entity.entityType = "ROUTE_RESERVATION";
            entity.entityCode = "PAY-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            entity.amount = amount;
            entity.currency = currency.empty() ? "mxn" : currency;
            entity.customerName = cardholderName.empty() ? "Cliente" : cardholderName;
            entity.customerEmail = customerEmail.empty() ? "cliente@omnisphere.com" : customerEmail;

            auto payRes = CreatePayment(ctx, entity, token, detectedBrand, "", 1);
            payRes.cardBrand = detectedBrand;
            if (!result.cardLast4.empty()) payRes.cardLast4 = result.cardLast4;
            return payRes;
        }
        catch (const std::exception& ex)
        {
            result.errorMessage = std::string("Excepción al procesar tarjeta en Mercado Pago: ") + ex.what();
            return result;
        }
    }

    MercadoPagoPaymentResult MercadoPagoService::CreatePayment(
        const omnisphere::models::SecurityContext& ctx,
        const omnisphere::models::PayableEntity& entity,
        const std::string& token,
        const std::string& paymentMethodId,
        const std::string& issuerId,
        int installments
    ) const
    {
        MercadoPagoPaymentResult result;
        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            result.errorMessage = "Mercado Pago no está configurado o está inactivo.";
            return result;
        }

        const auto& settings = *settingsOpt;
        if (settings.accessToken.empty())
        {
            result.errorMessage = "Access Token de Mercado Pago no configurado.";
            return result;
        }

        try
        {
            std::string idKey = "mp-" + entity.entityCode + "-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            json::object reqObj;
            reqObj["transaction_amount"] = entity.amount;
            reqObj["token"] = token;
            reqObj["description"] = "Pago de " + entity.entityType + " [" + entity.entityCode + "]";
            reqObj["installments"] = installments > 0 ? installments : 1;
            reqObj["payment_method_id"] = paymentMethodId.empty() ? "visa" : paymentMethodId;
            if (!issuerId.empty())
            {
                reqObj["issuer_id"] = issuerId;
            }

            json::object payerObj;
            payerObj["email"] = entity.customerEmail.empty() ? "cliente@omnisphere.com" : entity.customerEmail;
            reqObj["payer"] = payerObj;

            reqObj["external_reference"] = entity.entityCode;

            std::string jsonBody = json::serialize(reqObj);
            std::string resBody = ExecuteHttpsRequest("POST", "/v1/payments", settings.accessToken, jsonBody, idKey);

            auto parsed = json::parse(resBody);
            if (!parsed.is_object())
            {
                result.errorMessage = "Respuesta no válida de Mercado Pago: " + resBody;
                return result;
            }

            auto obj = parsed.as_object();
            if (obj.contains("status"))
            {
                result.status = json::value_to<std::string>(obj["status"]);
            }
            if (obj.contains("status_detail"))
            {
                result.statusDetail = json::value_to<std::string>(obj["status_detail"]);
            }
            if (obj.contains("id"))
            {
                if (obj["id"].is_int64()) result.paymentId = std::to_string(obj["id"].as_int64());
                else if (obj["id"].is_string()) result.paymentId = json::value_to<std::string>(obj["id"]);
            }

            if (result.status == "approved" || result.status == "in_process" || result.status == "pending")
            {
                result.success = true;
            }
            else
            {
                result.errorMessage = obj.contains("message") ? json::value_to<std::string>(obj["message"]) : ("Pago rechazado: " + result.statusDetail);
            }

            // Persistir transacción en BD
            if (m_repository && !result.paymentId.empty())
            {
                omnisphere::models::MercadoPagoTransaction tx;
                tx.externalReference = entity.entityCode;
                tx.paymentId = result.paymentId;
                tx.amount = entity.amount;
                tx.currency = entity.currency;
                tx.status = result.status;
                tx.statusDetail = result.statusDetail;
                tx.paymentMethodId = paymentMethodId;
                tx.installments = installments;
                tx.createdBy = !ctx.userCode.empty() ? ctx.userCode : "system";
                m_repository->SaveTransaction(tx);
            }
        }
        catch (const std::exception& ex)
        {
            result.errorMessage = std::string("Error al procesar pago en Mercado Pago: ") + ex.what();
            omnisphere::utils::Logger::LogError("MercadoPagoService", result.errorMessage);
        }

        return result;
    }

    MercadoPagoPreferenceResult MercadoPagoService::CreatePreference(
        const omnisphere::models::SecurityContext& /*ctx*/,
        const omnisphere::models::PayableEntity& entity
    ) const
    {
        MercadoPagoPreferenceResult result;
        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            result.errorMessage = "Mercado Pago no está configurado o está inactivo.";
            return result;
        }

        const auto& settings = *settingsOpt;
        try
        {
            json::object reqObj;
            json::array itemsArr;
            json::object itemObj;
            itemObj["id"] = entity.entityCode;
            itemObj["title"] = "Pago " + entity.entityType + " " + entity.entityCode;
            itemObj["quantity"] = 1;
            itemObj["unit_price"] = entity.amount;
            itemObj["currency_id"] = entity.currency.empty() ? "MXN" : entity.currency;
            itemsArr.push_back(itemObj);
            reqObj["items"] = itemsArr;

            json::object payerObj;
            payerObj["email"] = entity.customerEmail.empty() ? "cliente@omnisphere.com" : entity.customerEmail;
            payerObj["name"] = entity.customerName;
            reqObj["payer"] = payerObj;

            if (!entity.successUrl.empty() || !entity.cancelUrl.empty())
            {
                json::object backUrls;
                backUrls["success"] = entity.successUrl;
                backUrls["failure"] = entity.cancelUrl;
                backUrls["pending"] = entity.cancelUrl;
                reqObj["back_urls"] = backUrls;
                reqObj["auto_return"] = "approved";
            }

            reqObj["external_reference"] = entity.entityCode;

            std::string jsonBody = json::serialize(reqObj);
            std::string resBody = ExecuteHttpsRequest("POST", "/checkout/preferences", settings.accessToken, jsonBody);

            auto parsed = json::parse(resBody);
            if (parsed.is_object())
            {
                auto obj = parsed.as_object();
                if (obj.contains("id")) result.preferenceId = json::value_to<std::string>(obj["id"]);
                if (settings.isTestMode && obj.contains("sandbox_init_point"))
                {
                    result.checkoutUrl = json::value_to<std::string>(obj["sandbox_init_point"]);
                }
                else if (obj.contains("init_point"))
                {
                    result.checkoutUrl = json::value_to<std::string>(obj["init_point"]);
                }

                if (!result.checkoutUrl.empty())
                {
                    result.success = true;
                }
                else if (obj.contains("message"))
                {
                    result.errorMessage = json::value_to<std::string>(obj["message"]);
                }
            }
        }
        catch (const std::exception& ex)
        {
            result.errorMessage = std::string("Error al crear preferencia en Mercado Pago: ") + ex.what();
            omnisphere::utils::Logger::LogError("MercadoPagoService", result.errorMessage);
        }

        return result;
    }

    MercadoPagoPaymentResult MercadoPagoService::CreateBankTransfer(
        const omnisphere::models::SecurityContext& ctx,
        const omnisphere::models::PayableEntity& entity,
        const std::string& paymentMethodId
    ) const
    {
        MercadoPagoPaymentResult result;
        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            result.errorMessage = "Mercado Pago no está configurado.";
            return result;
        }

        const auto& settings = *settingsOpt;
        try
        {
            std::string idKey = "mp-bt-" + entity.entityCode + "-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            json::object reqObj;
            reqObj["transaction_amount"] = entity.amount;
            reqObj["description"] = "Transferencia " + entity.entityCode;
            reqObj["payment_method_id"] = paymentMethodId.empty() ? "spei" : paymentMethodId;

            json::object payerObj;
            payerObj["email"] = entity.customerEmail.empty() ? "cliente@omnisphere.com" : entity.customerEmail;
            payerObj["first_name"] = entity.customerName.empty() ? "Cliente" : entity.customerName;
            reqObj["payer"] = payerObj;

            reqObj["external_reference"] = entity.entityCode;

            std::string jsonBody = json::serialize(reqObj);
            std::string resBody = ExecuteHttpsRequest("POST", "/v1/payments", settings.accessToken, jsonBody, idKey);

            auto parsed = json::parse(resBody);
            if (parsed.is_object())
            {
                auto obj = parsed.as_object();
                if (obj.contains("id"))
                {
                    result.paymentId = obj["id"].is_int64() ? std::to_string(obj["id"].as_int64()) : json::value_to<std::string>(obj["id"]);
                }
                if (obj.contains("status")) result.status = json::value_to<std::string>(obj["status"]);
                if (obj.contains("status_detail")) result.statusDetail = json::value_to<std::string>(obj["status_detail"]);

                if (obj.contains("transaction_details") && obj["transaction_details"].is_object())
                {
                    auto td = obj["transaction_details"].as_object();
                    if (td.contains("external_resource_url") && !td["external_resource_url"].is_null())
                    {
                        result.ticketUrl = json::value_to<std::string>(td["external_resource_url"]);
                    }
                }

                if (obj.contains("point_of_interaction") && obj["point_of_interaction"].is_object())
                {
                    auto poi = obj["point_of_interaction"].as_object();
                    if (poi.contains("transaction_data") && poi["transaction_data"].is_object())
                    {
                        auto tData = poi["transaction_data"].as_object();
                        if (tData.contains("ticket_url") && !tData["ticket_url"].is_null())
                        {
                            result.ticketUrl = json::value_to<std::string>(tData["ticket_url"]);
                        }
                    }
                }

                result.success = !result.paymentId.empty();

                if (m_repository && result.success)
                {
                    omnisphere::models::MercadoPagoTransaction tx;
                    tx.externalReference = entity.entityCode;
                    tx.paymentId = result.paymentId;
                    tx.amount = entity.amount;
                    tx.currency = entity.currency;
                    tx.status = result.status;
                    tx.statusDetail = result.statusDetail;
                    tx.paymentMethodId = paymentMethodId;
                    tx.ticketUrl = result.ticketUrl;
                    tx.createdBy = !ctx.userCode.empty() ? ctx.userCode : "system";
                    m_repository->SaveTransaction(tx);
                }
            }
        }
        catch (const std::exception& ex)
        {
            result.errorMessage = std::string("Error al generar transferencia en Mercado Pago: ") + ex.what();
            omnisphere::utils::Logger::LogError("MercadoPagoService", result.errorMessage);
        }

        return result;
    }

    MercadoPagoDiagnosticResult MercadoPagoService::TestIntegration(const omnisphere::models::SecurityContext& /*ctx*/) const
    {
        MercadoPagoDiagnosticResult diag;
        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value())
        {
            diag.message = "No existe registro de configuración para Mercado Pago.";
            return diag;
        }

        const auto& s = *settingsOpt;
        if (s.accessToken.empty())
        {
            diag.message = "Access Token no configurado.";
            return diag;
        }
        diag.isConfigured = true;

        try
        {
            std::string resBody = ExecuteHttpsRequest("GET", "/v1/payment_methods", s.accessToken);
            auto parsed = json::parse(resBody);
            if (parsed.is_array())
            {
                diag.isFunctional = true;
                diag.message = "Conexión exitosa con Mercado Pago. Métodos disponibles: " + std::to_string(parsed.as_array().size());
            }
            else
            {
                diag.message = "Mercado Pago respondió con error: " + resBody;
            }
        }
        catch (const std::exception& ex)
        {
            diag.message = std::string("Fallo de comunicación: ") + ex.what();
        }

        return diag;
    }

    std::optional<boost::json::object> MercadoPagoService::GetPaymentFromMP(const std::string& paymentId) const
    {
        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || settingsOpt->accessToken.empty() || paymentId.empty()) return std::nullopt;

        try
        {
            std::string resBody = ExecuteHttpsRequest("GET", "/v1/payments/" + paymentId, settingsOpt->accessToken);
            auto parsed = json::parse(resBody);
            if (parsed.is_object())
            {
                return parsed.as_object();
            }
        }
        catch (...) {}
        return std::nullopt;
    }
} // namespace omnisphere::services
