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
}
