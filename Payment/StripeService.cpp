#include "Payment/StripeService.hpp"
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

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace json = boost::json;
using tcp = net::ip::tcp;

namespace omnisphere::services
{
    StripeService::StripeService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(dbPool)
    {
        if (dbPool)
        {
            m_repository = std::make_shared<omnisphere::repositories::StripeRepository>(dbPool);
        }
    }

    StripeService::StripeService(std::shared_ptr<omnisphere::repositories::StripeRepository> repository)
        : m_repository(std::move(repository)) {}

    std::optional<omnisphere::models::StripeSettings> StripeService::GetSettings(bool decryptKeys) const
    {
        if (!m_repository) return std::nullopt;
        auto dt = m_repository->GetSettings();
        if (dt.RowsCount() == 0) return std::nullopt;

        omnisphere::models::StripeSettings s;
        s.entry = (int)dt[0]["Entry"];
        s.code = (std::string)dt[0]["Code"];
        s.name = (std::string)dt[0]["Name"];
        s.publishableKey = dt[0].HasColumn("PublishableKey") && !dt[0]["PublishableKey"].IsNull() ? (std::string)dt[0]["PublishableKey"] : "";
        s.secretKey = dt[0].HasColumn("SecretKey") && !dt[0]["SecretKey"].IsNull() ? (std::string)dt[0]["SecretKey"] : "";
        s.webhookSecretKey = dt[0].HasColumn("WebhookSecretKey") && !dt[0]["WebhookSecretKey"].IsNull() ? (std::string)dt[0]["WebhookSecretKey"] : "";
        s.apiBaseUrl = dt[0].HasColumn("ApiBaseUrl") && !dt[0]["ApiBaseUrl"].IsNull() ? (std::string)dt[0]["ApiBaseUrl"] : "https://api.stripe.com/v1";
        s.checkoutEndpoint = dt[0].HasColumn("CheckoutEndpoint") && !dt[0]["CheckoutEndpoint"].IsNull() ? (std::string)dt[0]["CheckoutEndpoint"] : "/checkout/sessions";
        s.webhookPath = dt[0].HasColumn("WebhookPath") && !dt[0]["WebhookPath"].IsNull() ? (std::string)dt[0]["WebhookPath"] : "/api/v1/stripe/webhook";
        s.currency = dt[0].HasColumn("Currency") && !dt[0]["Currency"].IsNull() ? (std::string)dt[0]["Currency"] : "mxn";
        s.isTestMode = dt[0].HasColumn("IsTestMode") && !dt[0]["IsTestMode"].IsNull() ? (bool)dt[0]["IsTestMode"] : true;
        s.isActive = dt[0].HasColumn("IsActive") && !dt[0]["IsActive"].IsNull() ? (bool)dt[0]["IsActive"] : true;

        if (decryptKeys)
        {
            auto decryptVal = [](const std::string& val) -> std::string {
                if (val.empty()) return "";
                if (val.rfind("pk_", 0) == 0 || val.rfind("sk_", 0) == 0 || val.rfind("whsec_", 0) == 0) return val;
                try {
                    return omnisphere::utils::Base64::Decode(val);
                } catch (...) {
                    return val;
                }
            };

            s.publishableKey = decryptVal(s.publishableKey);
            s.secretKey = decryptVal(s.secretKey);
            s.webhookSecretKey = decryptVal(s.webhookSecretKey);
        }

        return s;
    }

    bool StripeService::SaveSettings(const omnisphere::models::StripeSettings& settings) const
    {
        if (!m_repository) return false;
        return m_repository->SaveSettings(settings);
    }

    StripeCheckoutResult StripeService::CreateCheckoutSession(
        const omnisphere::models::SecurityContext& ctx,
        const std::string& reservationCode,
        double amount,
        int seats,
        const std::string& successUrl,
        const std::string& cancelUrl
    ) const
    {
        StripeCheckoutResult res;
        if (reservationCode.empty() || amount <= 0.0)
        {
            res.errorMessage = "Parámetros inválidos: El código de reservación y el monto deben ser válidos.";
            return res;
        }

        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            res.errorMessage = "La integración de Stripe no está configurada o se encuentra desactivada.";
            return res;
        }

        auto settings = settingsOpt.value();
        if (settings.secretKey.empty())
        {
            res.errorMessage = "Configuración incompleta: Llave secreta de Stripe (SecretKey) no configurada.";
            return res;
        }

        try
        {
            // Parsear host y base path desde ApiBaseUrl (ej. https://api.stripe.com/v1)
            std::string host = "api.stripe.com";
            std::string basePath = "/v1";
            std::string fullUrl = settings.apiBaseUrl;
            std::size_t protoEnd = fullUrl.find("://");
            if (protoEnd != std::string::npos) {
                std::string pathPart = fullUrl.substr(protoEnd + 3);
                std::size_t slashPos = pathPart.find('/');
                if (slashPos != std::string::npos) {
                    host = pathPart.substr(0, slashPos);
                    basePath = pathPart.substr(slashPos);
                } else {
                    host = pathPart;
                    basePath = "";
                }
            }

            std::string target = basePath + settings.checkoutEndpoint;

            int unitAmountCentavos = static_cast<int>(amount * 100.0);
            int qty = seats > 0 ? seats : 1;
            std::string curr = settings.currency.empty() ? "mxn" : settings.currency;

            auto encodeUrlParam = [](const std::string& value) -> std::string {
                std::ostringstream escaped;
                escaped.fill('0');
                escaped << std::hex;
                for (char c : value) {
                    if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                        escaped << c;
                    } else {
                        escaped << '%' << std::setw(2) << std::uppercase << static_cast<int>(static_cast<unsigned char>(c));
                    }
                }
                return escaped.str();
            };

            std::string rawSuccUrl = successUrl.empty() ? "https://omni-house.zapto.org/confirmation?session_id={CHECKOUT_SESSION_ID}" : successUrl;
            std::string rawCancUrl = cancelUrl.empty() ? "https://omni-house.zapto.org/checkout" : cancelUrl;

            // Form-urlencoded payload para Stripe Checkout API
            std::ostringstream ssPayload;
            ssPayload << "payment_method_types[0]=card"
                      << "&line_items[0][price_data][currency]=" << encodeUrlParam(curr)
                      << "&line_items[0][price_data][unit_amount]=" << unitAmountCentavos
                      << "&line_items[0][price_data][product_data][name]=" << encodeUrlParam("Reservacion " + reservationCode)
                      << "&line_items[0][quantity]=" << qty
                      << "&mode=payment"
                      << "&client_reference_id=" << encodeUrlParam(reservationCode)
                      << "&metadata[reservationCode]=" << encodeUrlParam(reservationCode)
                      << "&success_url=" << encodeUrlParam(rawSuccUrl)
                      << "&cancel_url=" << encodeUrlParam(rawCancUrl);

            std::string payload = ssPayload.str();

            omnisphere::utils::Logger::LogInfo("StripeService", "Creating Checkout Session for reservation [" + reservationCode + "] via https://" + host + target);

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

            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(10));
            auto const results = resolver.resolve(tcp::v4(), host, "443");
            beast::get_lowest_layer(stream).connect(results);
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(10));
            stream.handshake(ssl::stream_base::client);

            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::content_type, "application/x-www-form-urlencoded");
            req.set(http::field::authorization, "Bearer " + settings.secretKey);
            req.body() = payload;
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> response;
            http::read(stream, buffer, response);

            std::string responseBodyStr = beast::buffers_to_string(response.body().data());
            omnisphere::utils::Logger::LogInfo("StripeService", "Stripe API Response Code: " + std::to_string(response.result_int()));

            if (response.result() == http::status::ok || response.result() == http::status::created)
            {
                auto parsed = json::parse(responseBodyStr);
                if (parsed.is_object())
                {
                    auto obj = parsed.as_object();
                    if (obj.contains("url") && obj.at("url").is_string())
                        res.checkoutUrl = std::string(obj.at("url").as_string());
                    if (obj.contains("id") && obj.at("id").is_string())
                        res.sessionId = std::string(obj.at("id").as_string());

                    if (!res.checkoutUrl.empty())
                    {
                        res.success = true;

                        // Persistir sesión creada en StripeSessions
                        omnisphere::models::StripeSession dbSession;
                        dbSession.code = "STR-SESS-" + (res.sessionId.length() > 12 ? res.sessionId.substr(0, 12) : res.sessionId);
                        dbSession.reservationCode = reservationCode;
                        dbSession.stripeSessionId = res.sessionId;
                        dbSession.checkoutUrl = res.checkoutUrl;
                        dbSession.amount = amount;
                        dbSession.currency = curr;
                        dbSession.status = "open";
                        dbSession.createdBy = 1;

                        if (m_repository)
                        {
                            m_repository->SaveSession(dbSession);
                        }
                    }
                }
            }

            if (!res.success)
            {
                res.errorMessage = "Error al crear la sesión en Stripe. Respuesta: " + responseBodyStr;
                omnisphere::utils::Logger::LogError("StripeService", res.errorMessage);
            }
        }
        catch (const std::exception& ex)
        {
            res.errorMessage = std::string("Excepción en StripeService: ") + ex.what();
            omnisphere::utils::Logger::LogError("StripeService", res.errorMessage);
        }

        return res;
    }

    StripePaymentIntentResult StripeService::CreatePaymentIntent(
        const omnisphere::models::SecurityContext& ctx,
        const std::string& reservationCode,
        double amount,
        const std::string& currency
    ) const
    {
        StripePaymentIntentResult res;
        auto settingsOpt = GetSettings(true);

        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            res.errorMessage = "La integración de Stripe no está configurada o se encuentra desactivada.";
            return res;
        }

        auto settings = settingsOpt.value();
        if (settings.secretKey.empty())
        {
            res.errorMessage = "Configuración incompleta: Llave secreta (SecretKey) de Stripe no configurada.";
            return res;
        }

        res.publishableKey = settings.publishableKey;

        try
        {
            std::string host = "api.stripe.com";
            std::string port = "443";
            std::string target = "/v1/payment_intents";

            boost::asio::io_context ioc;
            ssl::context sslCtx(ssl::context::tlsv12_client);
            sslCtx.set_default_verify_paths();

            tcp::resolver resolver(ioc);
            ssl::stream<tcp::socket> stream(ioc, sslCtx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                res.errorMessage = "Error configurando SNI para SSL Stripe.";
                return res;
            }

            auto const results = resolver.resolve(host, port);
            boost::asio::connect(stream.next_layer(), results.begin(), results.end());
            stream.handshake(ssl::stream_base::client);

            int amountCents = static_cast<int>(std::round(amount * 100.0));
            std::string curr = currency.empty() ? settings.currency : currency;

            auto urlEncode = [](const std::string& value) -> std::string {
                std::ostringstream escaped;
                escaped.fill('0');
                escaped << std::hex;
                for (char c : value) {
                    if (std::isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
                        escaped << c;
                    } else {
                        escaped << '%' << std::setw(2) << std::uppercase << (int)(unsigned char)c;
                    }
                }
                return escaped.str();
            };

            std::string reqBody = "amount=" + std::to_string(amountCents)
                                + "&currency=" + urlEncode(curr)
                                + "&automatic_payment_methods[enabled]=true"
                                + "&metadata[reservationCode]=" + urlEncode(reservationCode);

            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "OmniSphere-C++/1.0");
            req.set(http::field::content_type, "application/x-www-form-urlencoded");
            req.set(http::field::authorization, "Bearer " + settings.secretKey);
            req.body() = reqBody;
            req.prepare_payload();

            omnisphere::utils::Logger::LogInfo("StripeService", "Creating PaymentIntent for reservation [" + reservationCode + "] amount $" + std::to_string(amount));
            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> response;
            http::read(stream, buffer, response);

            std::string responseBodyStr = beast::buffers_to_string(response.body().data());
            omnisphere::utils::Logger::LogInfo("StripeService", "Stripe PaymentIntent Response Code: " + std::to_string(response.result_int()));

            if (response.result() == http::status::ok || response.result() == http::status::created)
            {
                auto parsed = json::parse(responseBodyStr);
                if (parsed.is_object())
                {
                    auto obj = parsed.as_object();
                    if (obj.contains("client_secret") && obj.at("client_secret").is_string())
                        res.clientSecret = std::string(obj.at("client_secret").as_string());
                    if (obj.contains("id") && obj.at("id").is_string())
                        res.paymentIntentId = std::string(obj.at("id").as_string());

                    if (!res.clientSecret.empty())
                    {
                        res.success = true;
                    }
                }
            }

            if (!res.success)
            {
                res.errorMessage = "Error al crear el PaymentIntent en Stripe. Respuesta: " + responseBodyStr;
                omnisphere::utils::Logger::LogError("StripeService", res.errorMessage);
            }
        }
        catch (const std::exception& ex)
        {
            res.errorMessage = std::string("Excepción creando PaymentIntent: ") + ex.what();
            omnisphere::utils::Logger::LogError("StripeService", res.errorMessage);
        }

        return res;
    }

    StripeBankTransferResult StripeService::CreateBankTransferPaymentIntent(
        const omnisphere::models::SecurityContext& ctx,
        const std::string& reservationCode,
        double amount,
        const std::string& customerName,
        const std::string& customerEmail
    ) const
    {
        StripeBankTransferResult res;
        res.amount = amount;
        res.currency = "mxn";

        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            res.errorMessage = "La integración de Stripe no está configurada o se encuentra desactivada.";
            return res;
        }

        auto settings = settingsOpt.value();
        if (settings.secretKey.empty())
        {
            res.errorMessage = "Configuración incompleta: Llave secreta (SecretKey) de Stripe no configurada.";
            return res;
        }

        try
        {
            std::string host = "api.stripe.com";
            std::string port = "443";

            auto urlEncode = [](const std::string& value) -> std::string {
                std::ostringstream escaped;
                escaped.fill('0');
                escaped << std::hex;
                for (char c : value) {
                    if (std::isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
                        escaped << c;
                    } else {
                        escaped << '%' << std::setw(2) << std::uppercase << (int)(unsigned char)c;
                    }
                }
                return escaped.str();
            };

            auto sendStripePost = [&](const std::string& target, const std::string& body) -> std::pair<int, std::string> {
                boost::asio::io_context ioc;
                ssl::context sslCtx(ssl::context::tlsv12_client);
                sslCtx.set_default_verify_paths();

                tcp::resolver resolver(ioc);
                ssl::stream<tcp::socket> stream(ioc, sslCtx);

                if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
                    throw std::runtime_error("Error al configurar SNI SSL para Stripe.");
                }

                auto const results = resolver.resolve(host, port);
                boost::asio::connect(stream.next_layer(), results.begin(), results.end());
                stream.handshake(ssl::stream_base::client);

                http::request<http::string_body> req{http::verb::post, target, 11};
                req.set(http::field::host, host);
                req.set(http::field::user_agent, "OmniSphere-C++/1.0");
                req.set(http::field::content_type, "application/x-www-form-urlencoded");
                req.set(http::field::authorization, "Bearer " + settings.secretKey);
                req.body() = body;
                req.prepare_payload();

                http::write(stream, req);

                beast::flat_buffer buffer;
                http::response<http::dynamic_body> response;
                http::read(stream, buffer, response);

                return { static_cast<int>(response.result_int()), beast::buffers_to_string(response.body().data()) };
            };

            // Paso 1: Crear o vincular Customer en Stripe para recibir transferencia SPEI (customer_balance)
            std::string clientName = customerName.empty() ? ("Cliente " + reservationCode) : customerName;
            std::string custBody = "name=" + urlEncode(clientName) + "&metadata[reservationCode]=" + urlEncode(reservationCode);
            if (!customerEmail.empty()) {
                custBody += "&email=" + urlEncode(customerEmail);
            }

            omnisphere::utils::Logger::LogInfo("StripeService", "[BankTransfer] Creating Stripe Customer for reservation: " + reservationCode);
            auto [custStatus, custResp] = sendStripePost("/v1/customers", custBody);

            std::string customerId = "";
            if (custStatus == 200 || custStatus == 201) {
                auto parsedCust = json::parse(custResp);
                if (parsedCust.is_object() && parsedCust.as_object().contains("id")) {
                    customerId = std::string(parsedCust.as_object().at("id").as_string());
                }
            }

            if (customerId.empty()) {
                res.errorMessage = "No se pudo crear el cliente en Stripe para la transferencia SPEI. Respuesta: " + custResp;
                omnisphere::utils::Logger::LogError("StripeService", res.errorMessage);
                return res;
            }

            // Paso 2: Crear y confirmar PaymentIntent con mx_bank_transfer
            int amountCents = static_cast<int>(std::round(amount * 100.0));
            std::ostringstream piBody;
            piBody << "amount=" << amountCents
                   << "&currency=mxn"
                   << "&customer=" << urlEncode(customerId)
                   << "&payment_method_types[0]=customer_balance"
                   << "&payment_method_data[type]=customer_balance"
                   << "&payment_method_options[customer_balance][funding_type]=bank_transfer"
                   << "&payment_method_options[customer_balance][bank_transfer][type]=mx_bank_transfer"
                   << "&confirm=true"
                   << "&metadata[reservationCode]=" << urlEncode(reservationCode)
                   << "&description=" << urlEncode("Pago de reservación " + reservationCode);

            omnisphere::utils::Logger::LogInfo("StripeService", "[BankTransfer] Creating and Confirming SPEI PaymentIntent for reservation: " + reservationCode + " ($" + std::to_string(amount) + ")");
            auto [piStatus, piResp] = sendStripePost("/v1/payment_intents", piBody.str());

            if (piStatus == 200 || piStatus == 201) {
                auto parsedPI = json::parse(piResp);
                if (parsedPI.is_object()) {
                    auto const& piObj = parsedPI.as_object();
                    if (piObj.contains("id") && piObj.at("id").is_string()) {
                        res.paymentIntentId = std::string(piObj.at("id").as_string());
                    }

                    if (piObj.contains("next_action") && piObj.at("next_action").is_object()) {
                        auto const& nextAction = piObj.at("next_action").as_object();
                        if (nextAction.contains("display_bank_transfer_instructions") && nextAction.at("display_bank_transfer_instructions").is_object()) {
                            auto const& instr = nextAction.at("display_bank_transfer_instructions").as_object();
                            if (instr.contains("hosted_instructions_url") && instr.at("hosted_instructions_url").is_string()) {
                                res.hostedInstructionsUrl = std::string(instr.at("hosted_instructions_url").as_string());
                            }

                            if (instr.contains("financial_addresses") && instr.at("financial_addresses").is_array()) {
                                for (auto const& faVal : instr.at("financial_addresses").as_array()) {
                                    if (faVal.is_object()) {
                                        auto const& fa = faVal.as_object();
                                        if (fa.contains("spei") && fa.at("spei").is_object()) {
                                            auto const& spei = fa.at("spei").as_object();
                                            if (spei.contains("clabe") && spei.at("clabe").is_string()) {
                                                res.clabe = std::string(spei.at("clabe").as_string());
                                            }
                                            if (spei.contains("bank_name") && spei.at("bank_name").is_string()) {
                                                res.bankName = std::string(spei.at("bank_name").as_string());
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (res.bankName.empty()) res.bankName = "STP";

                    if (!res.clabe.empty()) {
                        res.success = true;
                        omnisphere::utils::Logger::LogInfo("StripeService", "[BankTransfer] Successfully generated virtual CLABE [" + res.clabe + "] for reservation [" + reservationCode + "]");

                        if (m_repository) {
                            omnisphere::models::StripeTransaction tx;
                            tx.code = "TX-SPEI-" + (res.paymentIntentId.length() > 12 ? res.paymentIntentId.substr(0, 12) : res.paymentIntentId);
                            tx.reservationCode = reservationCode;
                            tx.stripePaymentIntentId = res.paymentIntentId;
                            tx.amount = amount;
                            tx.currency = "mxn";
                            tx.status = "requires_action";
                            tx.paymentMethodType = "spei";
                            tx.clabe = res.clabe;
                            tx.bankName = res.bankName;
                            tx.hostedInstructionsUrl = res.hostedInstructionsUrl;
                            tx.createdBy = 1;

                            m_repository->SaveTransaction(tx);
                        }
                    }
                }
            }

            if (!res.success) {
                res.errorMessage = "Error al generar la transferencia bancaria SPEI en Stripe. Respuesta: " + piResp;
                omnisphere::utils::Logger::LogError("StripeService", res.errorMessage);
            }
        }
        catch (const std::exception& ex)
        {
            res.errorMessage = std::string("Excepción creando transferencia bancaria SPEI: ") + ex.what();
            omnisphere::utils::Logger::LogError("StripeService", res.errorMessage);
        }

        return res;
    }

    StripeTestIntegrationResult StripeService::TestIntegration(
        const omnisphere::models::SecurityContext& ctx
    ) const
    {
        StripeTestIntegrationResult res;
        auto settingsOpt = GetSettings(true);

        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            res.isConfigured = false;
            res.isFunctional = false;
            res.message = "La integración de Stripe no está configurada o se encuentra desactivada.";
            return res;
        }

        auto settings = settingsOpt.value();
        if (settings.secretKey.empty() || settings.publishableKey.empty())
        {
            res.isConfigured = false;
            res.isFunctional = false;
            res.message = "Configuración incompleta: Faltan las llaves SecretKey o PublishableKey.";
            return res;
        }

        res.isConfigured = true;

        try
        {
            std::string host = "api.stripe.com";
            std::string port = "443";
            std::string target = "/v1/account";

            boost::asio::io_context ioc;
            ssl::context sslCtx(ssl::context::tlsv12_client);
            sslCtx.set_default_verify_paths();

            tcp::resolver resolver(ioc);
            ssl::stream<tcp::socket> stream(ioc, sslCtx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                res.isFunctional = false;
                res.message = "Error configurando SNI para prueba SSL.";
                return res;
            }

            auto const results = resolver.resolve(host, port);
            boost::asio::connect(stream.next_layer(), results.begin(), results.end());
            stream.handshake(ssl::stream_base::client);

            http::request<http::empty_body> req{http::verb::get, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "OmniSphere-C++/1.0");
            req.set(http::field::authorization, "Bearer " + settings.secretKey);
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> response;
            http::read(stream, buffer, response);

            if (response.result() == http::status::ok)
            {
                res.isFunctional = true;
                res.message = "Conexión exitosa con la API de Stripe. Integración activa y lista para operar.";
            }
            else
            {
                res.isFunctional = false;
                std::string body = beast::buffers_to_string(response.body().data());
                res.message = "Stripe devolvió un código de error (" + std::to_string(response.result_int()) + "): " + body;
            }
        }
        catch (const std::exception& ex)
        {
            res.isFunctional = false;
            res.message = std::string("Excepción al probar conexión con Stripe: ") + ex.what();
        }

        return res;
    }

    bool StripeService::CancelPaymentIntent(const std::string& paymentIntentId, const std::string& reason) const
    {
        if (paymentIntentId.empty()) return false;

        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || !settingsOpt->isActive || settingsOpt->secretKey.empty())
        {
            return false;
        }

        auto settings = settingsOpt.value();
        try
        {
            std::string host = "api.stripe.com";
            std::string port = "443";
            std::string target = "/v1/payment_intents/" + paymentIntentId + "/cancel";
            std::string body = "cancellation_reason=" + (reason.empty() ? "abandoned" : reason);

            boost::asio::io_context ioc;
            ssl::context sslCtx(ssl::context::tlsv12_client);
            sslCtx.set_default_verify_paths();

            tcp::resolver resolver(ioc);
            ssl::stream<tcp::socket> stream(ioc, sslCtx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                return false;
            }

            auto const results = resolver.resolve(host, port);
            boost::asio::connect(stream.next_layer(), results.begin(), results.end());
            stream.handshake(ssl::stream_base::client);

            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "OmniSphere-C++/1.0");
            req.set(http::field::content_type, "application/x-www-form-urlencoded");
            req.set(http::field::authorization, "Bearer " + settings.secretKey);
            req.body() = body;
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> response;
            http::read(stream, buffer, response);

            int status = static_cast<int>(response.result_int());
            if (status == 200 || status == 201)
            {
                omnisphere::utils::Logger::LogInfo("StripeService",
                    "PaymentIntent [" + paymentIntentId + "] canceled successfully in Stripe.");
                if (m_repository)
                {
                    m_repository->UpdateTransactionStatus(paymentIntentId, "canceled");
                }
                return true;
            }
            else
            {
                std::string respBody = beast::buffers_to_string(response.body().data());
                omnisphere::utils::Logger::LogWarning("StripeService",
                    "Failed to cancel PaymentIntent [" + paymentIntentId + "] in Stripe (Status " + std::to_string(status) + "): " + respBody);
                return false;
            }
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("StripeService",
                "Exception canceling PaymentIntent [" + paymentIntentId + "]: " + std::string(ex.what()));
            return false;
        }
    }

    bool StripeService::ExpireCheckoutSession(const std::string& sessionId) const
    {
        if (sessionId.empty()) return false;

        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || !settingsOpt->isActive || settingsOpt->secretKey.empty())
        {
            return false;
        }

        auto settings = settingsOpt.value();
        try
        {
            std::string host = "api.stripe.com";
            std::string port = "443";
            std::string target = "/v1/checkout/sessions/" + sessionId + "/expire";

            boost::asio::io_context ioc;
            ssl::context sslCtx(ssl::context::tlsv12_client);
            sslCtx.set_default_verify_paths();

            tcp::resolver resolver(ioc);
            ssl::stream<tcp::socket> stream(ioc, sslCtx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                return false;
            }

            auto const results = resolver.resolve(host, port);
            boost::asio::connect(stream.next_layer(), results.begin(), results.end());
            stream.handshake(ssl::stream_base::client);

            http::request<http::empty_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "OmniSphere-C++/1.0");
            req.set(http::field::authorization, "Bearer " + settings.secretKey);
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> response;
            http::read(stream, buffer, response);

            int status = static_cast<int>(response.result_int());
            if (status == 200 || status == 201)
            {
                omnisphere::utils::Logger::LogInfo("StripeService",
                    "CheckoutSession [" + sessionId + "] expired successfully in Stripe.");
                if (m_repository)
                {
                    m_repository->UpdateSessionStatus(sessionId, "expired");
                }
                return true;
            }
            else
            {
                std::string respBody = beast::buffers_to_string(response.body().data());
                omnisphere::utils::Logger::LogWarning("StripeService",
                    "Failed to expire CheckoutSession [" + sessionId + "] in Stripe (Status " + std::to_string(status) + "): " + respBody);
                return false;
            }
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("StripeService",
                "Exception expiring CheckoutSession [" + sessionId + "]: " + std::string(ex.what()));
            return false;
        }
    }
}
