#include "Notification/WhatsAppService.hpp"
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
#include <ctime>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace json = boost::json;
using tcp = net::ip::tcp;

namespace omnisphere::services
{
    WhatsAppService::WhatsAppService(const omnisphere::dtos::WhatsAppConfig& config)
        : m_config(config) {}

    WhatsAppService::WhatsAppService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
    {
        InitializeFromDatabase(std::move(dbPool));
    }

    bool WhatsAppService::InitializeFromDatabase(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
    {
        if (!dbPool) return false;
        m_repository = std::make_shared<omnisphere::repositories::WhatsAppRepository>(dbPool);
        auto dt = m_repository->GetSettings();
        if (dt.RowsCount() > 0)
        {
            std::string rawPhoneId = (std::string)dt[0]["PhoneId"];
            std::string rawToken = (std::string)dt[0]["ApiToken"];
            std::string rawWebhookToken = (std::string)dt[0]["WebhookVerifyToken"];

            auto isPlain = [](const std::string& val) -> bool {
                if (val.empty()) return false;
                if (val.rfind("omni_", 0) == 0) return true;
                if (val.rfind("EAAG", 0) == 0 || val.rfind("EAAB", 0) == 0) return true;
                bool allDigits = true;
                for (char c : val) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) { allDigits = false; break; }
                }
                if (allDigits && val.length() > 5) return true;
                return false;
            };

            auto decryptVal = [&](const std::string& val) -> std::string {
                if (val.empty()) return "";
                if (isPlain(val)) return val;
                try {
                    return omnisphere::utils::Base64::Decode(val);
                } catch (...) {}
                return val;
            };

            // Decrypt ONLY in backend RAM memory for outgoing HTTP requests to Meta
            m_config.phoneId = decryptVal(rawPhoneId);
            m_config.token = decryptVal(rawToken);
            m_config.webhookVerifyToken = decryptVal(rawWebhookToken);
            m_config.apiVersion = (std::string)dt[0]["ApiVersion"];
            if (m_config.apiVersion.empty()) m_config.apiVersion = "v24.0";
            return true;
        }
        return false;
    }

    void WhatsAppService::SetConfig(const omnisphere::dtos::WhatsAppConfig& config)
    {
        m_config = config;
    }

    omnisphere::dtos::WhatsAppConfig WhatsAppService::GetConfig() const
    {
        return m_config;
    }

    omnisphere::models::WhatsAppSettings WhatsAppService::GetSettings(const std::vector<std::string>& requestedFields) const
    {
        omnisphere::models::WhatsAppSettings settings;
        if (m_repository)
        {
            auto dt = m_repository->GetSettings(requestedFields);
            if (dt.RowsCount() > 0)
            {
                auto getVal = [&](const std::string& colName) -> std::string {
                    try {
                        if (dt[0].HasColumn(colName) && !dt[0][colName].IsNull())
                            return (std::string)dt[0][colName];
                    } catch (...) {}
                    try {
                        std::string lower = colName;
                        lower[0] = std::tolower(lower[0]);
                        if (dt[0].HasColumn(lower) && !dt[0][lower].IsNull())
                            return (std::string)dt[0][lower];
                    } catch (...) {}
                    return "";
                };

                auto isPlain = [](const std::string& val) -> bool {
                    if (val.empty()) return false;
                    if (val.rfind("omni_", 0) == 0) return true;
                    if (val.rfind("EAAG", 0) == 0 || val.rfind("EAAB", 0) == 0) return true;
                    bool allDigits = true;
                    for (char c : val) {
                        if (!std::isdigit(static_cast<unsigned char>(c))) { allDigits = false; break; }
                    }
                    if (allDigits && val.length() > 5) return true;
                    return false;
                };

                auto ensureEncryptedVal = [&](const std::string& colName) -> std::string {
                    std::string val = getVal(colName);
                    if (val.empty()) return "";
                    if (isPlain(val)) {
                        return omnisphere::utils::Base64::Encode(val);
                    }
                    return val;
                };

                try {
                    if (dt[0].HasColumn("Entry") && !dt[0]["Entry"].IsNull()) settings.entry = (int)dt[0]["Entry"];
                    else if (dt[0].HasColumn("entry") && !dt[0]["entry"].IsNull()) settings.entry = (int)dt[0]["entry"];
                } catch (...) {}

                settings.code = getVal("Code");
                if (settings.code.empty()) settings.code = "DEFAULT";

                settings.name = getVal("Name"); if (settings.name.empty()) settings.name = "MetaConfig";

                // ALWAYS return ENCRYPTED credentials over GraphQL / API responses
                settings.phoneId = ensureEncryptedVal("PhoneId");
                settings.apiToken = ensureEncryptedVal("ApiToken");
                settings.businessAccountId = ensureEncryptedVal("BusinessAccountId");
                settings.webhookVerifyToken = ensureEncryptedVal("WebhookVerifyToken");
                settings.apiVersion = getVal("ApiVersion"); if (settings.apiVersion.empty()) settings.apiVersion = "v24.0";

                try {
                    if (dt[0].HasColumn("IsActive") && !dt[0]["IsActive"].IsNull()) settings.isActive = (bool)dt[0]["IsActive"];
                    else if (dt[0].HasColumn("isactive") && !dt[0]["isactive"].IsNull()) settings.isActive = (bool)dt[0]["isactive"];
                } catch (...) {}
            }
        }
        return settings;
    }

    bool WhatsAppService::SaveSettings(const omnisphere::models::WhatsAppSettings& settings)
    {
        if (!m_repository) return false;

        auto isPlain = [](const std::string& val) -> bool {
            if (val.empty()) return false;
            if (val.rfind("omni_", 0) == 0) return true;
            if (val.rfind("EAAG", 0) == 0 || val.rfind("EAAB", 0) == 0) return true;
            bool allDigits = true;
            for (char c : val) {
                if (!std::isdigit(static_cast<unsigned char>(c))) { allDigits = false; break; }
            }
            if (allDigits && val.length() > 5) return true;
            return false;
        };

        auto ensureEncrypted = [&](const std::string& val) -> std::string {
            if (val.empty()) return "";
            if (isPlain(val)) {
                return omnisphere::utils::Base64::Encode(val);
            }
            return val;
        };

        auto decryptVal = [&](const std::string& val) -> std::string {
            if (val.empty()) return "";
            if (isPlain(val)) return val;
            try {
                return omnisphere::utils::Base64::Decode(val);
            } catch (...) {}
            return val;
        };

        omnisphere::models::WhatsAppSettings encSettings = settings;
        if (encSettings.apiVersion.empty()) encSettings.apiVersion = "v24.0";
        encSettings.phoneId = ensureEncrypted(settings.phoneId);
        encSettings.apiToken = ensureEncrypted(settings.apiToken);
        encSettings.businessAccountId = ensureEncrypted(settings.businessAccountId);
        encSettings.webhookVerifyToken = ensureEncrypted(settings.webhookVerifyToken);

        bool ok = m_repository->SaveSettings(encSettings);
        if (ok)
        {
            // Decrypt ONLY in backend RAM memory for outgoing HTTP calls to Meta
            if (!settings.phoneId.empty()) m_config.phoneId = decryptVal(settings.phoneId);
            if (!settings.apiToken.empty()) m_config.token = decryptVal(settings.apiToken);
            if (!settings.webhookVerifyToken.empty()) m_config.webhookVerifyToken = decryptVal(settings.webhookVerifyToken);
            if (!settings.apiVersion.empty()) m_config.apiVersion = settings.apiVersion;
        }
        return ok;
    }

    std::string WhatsAppService::ParseMetaErrorMessage(const std::string& rawPayload)
    {
        if (rawPayload.empty()) return "No se pudo establecer conexión con la API de Meta WhatsApp.";

        try {
            auto parsed = json::parse(rawPayload);
            if (parsed.is_object() && parsed.as_object().contains("error")) {
                auto errObj = parsed.as_object().at("error").as_object();
                int code = 0;
                if (errObj.contains("code") && errObj.at("code").is_int64()) {
                    code = errObj.at("code").as_int64();
                }
                std::string msg;
                if (errObj.contains("message")) {
                    msg = std::string(errObj.at("message").as_string());
                }

                std::string details;
                if (errObj.contains("error_data") && errObj.at("error_data").is_object()) {
                    auto dataObj = errObj.at("error_data").as_object();
                    if (dataObj.contains("details")) {
                        details = std::string(dataObj.at("details").as_string());
                    }
                }

                if (code == 131058 || msg.find("Public Test Numbers") != std::string::npos) {
                    return "La plantilla de prueba por defecto (hello_world) solo puede enviarse desde números de prueba de Meta. Configura una plantilla aprobada para tu número real.";
                }
                if (code == 132001 || msg.find("does not exist") != std::string::npos || details.find("does not exist") != std::string::npos) {
                    return "La plantilla solicitada no está registrada o aprobada en Meta WhatsApp para el idioma configurado.";
                }
                if (code == 100 || details.find("Parameter") != std::string::npos) {
                    return "Los parámetros enviados no coinciden con las variables de la plantilla aprobada en Meta.";
                }
                if (code == 190 || msg.find("OAuth") != std::string::npos || msg.find("token") != std::string::npos) {
                    return "El Token de Acceso (ApiToken) de Meta WhatsApp expiró o es inválido. Por favor actualízalo en la configuración.";
                }
                if (code == 131026) {
                    return "El mensaje no pudo ser entregado. Verifica que el número de teléfono esté registrado y activo en WhatsApp.";
                }
                if (code == 131047) {
                    return "Han transcurrido más de 24 horas desde la última interacción del usuario. Se debe iniciar la conversación enviando una plantilla aprobada.";
                }

                if (!details.empty()) return "Error de Meta API: " + details;
                if (!msg.empty()) return "Error de Meta API: " + msg;
            }
        } catch (...) {}

        return "No fue posible entregar el mensaje a través de WhatsApp. Detalles: " + rawPayload.substr(0, 150);
    }

    bool WhatsAppService::SendRequest(
        const std::string& phoneNumber,
        const std::string& messageType,
        const std::string& templateName,
        const std::string& content,
        const std::string& jsonString
    )
    {
        omnisphere::utils::Logger::LogInfo("WhatsAppService", "Initiating WhatsApp " + messageType + " message dispatch to recipient: " + phoneNumber);

        if (m_config.token.empty() || m_config.phoneId.empty())
        {
            m_lastError = "Configuración incompleta: PhoneId o ApiToken de Meta WhatsApp no están configurados.";
            omnisphere::utils::Logger::LogError("WhatsAppService", m_lastError);
            return false;
        }

        std::string wamidCode = "";
        std::string responseBodyStr = "";
        bool requestSuccess = false;

        try
        {
            std::string host = "graph.facebook.com";
            std::string port = "443";
            std::string versionStr = m_config.apiVersion.empty() ? "v24.0" : m_config.apiVersion;
            std::string target = "/" + versionStr + "/" + m_config.phoneId + "/messages";

            omnisphere::utils::Logger::LogInfo("WhatsAppService", "Sending HTTP POST request to https://" + host + target);

            net::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_peer);

            tcp::resolver resolver(ioc);
            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                throw beast::system_error{ec};
            }

            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(10));
            auto const results = resolver.resolve(tcp::v4(), host, port);
            beast::get_lowest_layer(stream).connect(results);
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(10));
            stream.handshake(ssl::stream_base::client);

            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::content_type, "application/json");
            req.set(http::field::authorization, "Bearer " + m_config.token);
            req.body() = jsonString;
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(stream, buffer, res);

            responseBodyStr = beast::buffers_to_string(res.body().data());
            requestSuccess = (res.result() == http::status::ok || res.result() == http::status::created);

            if (requestSuccess)
            {
                m_lastError = "";
                try
                {
                    auto parsed = json::parse(responseBodyStr);
                    if (parsed.is_object() && parsed.as_object().contains("messages"))
                    {
                        auto msgs = parsed.as_object().at("messages").as_array();
                        if (!msgs.empty() && msgs[0].is_object() && msgs[0].as_object().contains("id"))
                        {
                            wamidCode = std::string(msgs[0].as_object().at("id").as_string());
                        }
                    }
                }
                catch (...) {}
                omnisphere::utils::Logger::LogInfo("WhatsAppService", "Meta API Raw Response: " + responseBodyStr);
                omnisphere::utils::Logger::LogInfo("WhatsAppService", "Meta API Request SUCCESS (HTTP " + std::to_string(res.result_int()) + ")! Message WAMID: " + (wamidCode.empty() ? "N/A" : wamidCode));
            }
            else
            {
                m_lastError = ParseMetaErrorMessage(responseBodyStr);
                omnisphere::utils::Logger::LogError("WhatsAppService", "Meta API Request FAILED (HTTP " + std::to_string(res.result_int()) + "). Payload: " + responseBodyStr);
            }

            beast::error_code ec;
            stream.shutdown(ec);
        }
        catch (const std::exception& ex)
        {
            m_lastError = "Error de conexión HTTP con Meta: " + std::string(ex.what());
            omnisphere::utils::Logger::LogError("WhatsAppService", m_lastError);
            responseBodyStr = ex.what();
        }

        if (m_repository)
        {
            int convEntry = m_repository->GetOrCreateConversation(phoneNumber);
            if (convEntry > 0)
            {
                omnisphere::models::WhatsAppMessage msg;
                msg.code = wamidCode.empty() ? ("ERR-" + std::to_string(std::time(nullptr))) : wamidCode;
                msg.conversationEntry = convEntry;
                msg.senderType = "OUTBOUND";
                msg.messageType = messageType;
                msg.templateName = templateName;
                msg.content = content;
                msg.status = requestSuccess ? "SENT" : "FAILED";
                msg.responsePayload = responseBodyStr;
                msg.sentBy = 1;
                m_repository->LogMessage(msg);
                omnisphere::utils::Logger::LogInfo("WhatsAppService", "Message logged to database (ConvEntry: " + std::to_string(convEntry) + ", Status: " + msg.status + ")");
            }
        }

        return requestSuccess;
    }

    static std::string SanitizePhoneNumber(const std::string& raw)
    {
        std::string clean = "";
        for (char c : raw)
        {
            if (std::isdigit(static_cast<unsigned char>(c)))
            {
                clean += c;
            }
        }
        return clean;
    }

    bool WhatsAppService::SendNotification(
        const std::string& phoneNumber,
        const std::string& templateName,
        const std::vector<std::string>& params
    )
    {
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "template";

        json::object tmpl;
        tmpl["name"] = templateName;
        json::object lang;
        lang["code"] = (templateName == "ticket_confirmation") ? "en" : "es_MX";
        tmpl["language"] = lang;

        if (!params.empty())
        {
            json::array components;
            json::object bodyComp;
            bodyComp["type"] = "body";
            json::array parameters;
            for (const auto& p : params)
            {
                json::object paramObj;
                paramObj["type"] = "text";
                paramObj["text"] = p;
                parameters.push_back(paramObj);
            }
            bodyComp["parameters"] = parameters;
            components.push_back(bodyComp);
            tmpl["components"] = components;
        }
        
        body["template"] = tmpl;

        std::string jsonStr = json::serialize(body);
        std::string contentStr = "Template: " + templateName;
        return SendRequest(cleanPhone, "TEMPLATE", templateName, contentStr, jsonStr);
    }

    bool WhatsAppService::SendNamedNotification(
        const std::string& phoneNumber,
        const std::string& templateName,
        const std::map<std::string, std::string>& params
    )
    {
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "template";

        json::object tmpl;
        tmpl["name"] = templateName;
        json::object lang;
        lang["code"] = (templateName == "ticket_confirmation") ? "en" : "es_MX";
        tmpl["language"] = lang;

        if (!params.empty())
        {
            json::array components;
            json::object bodyComp;
            bodyComp["type"] = "body";
            json::array parameters;
            for (const auto& [key, val] : params)
            {
                json::object paramObj;
                paramObj["type"] = "text";
                paramObj["parameter_name"] = key;
                paramObj["text"] = val;
                parameters.push_back(paramObj);
            }
            bodyComp["parameters"] = parameters;
            components.push_back(bodyComp);
            tmpl["components"] = components;
        }
        body["template"] = tmpl;

        std::string jsonStr = json::serialize(body);
        std::string contentStr = "Named Template: " + templateName;
        return SendRequest(cleanPhone, "TEMPLATE", templateName, contentStr, jsonStr);
    }

    bool WhatsAppService::SendMessage(
        const std::string& phoneNumber,
        const std::string& message
    )
    {
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "text";

        json::object textObj;
        textObj["body"] = message;
        body["text"] = textObj;

        std::string jsonStr = json::serialize(body);
        return SendRequest(cleanPhone, "TEXT", "", message, jsonStr);
    }

    bool WhatsAppService::SendTicketConfirmation(
        const std::string& phoneNumber,
        const std::string& customerName,
        const std::string& ticketNumber,
        const std::string& tripType,
        const std::string& eventName,
        const std::string& eventDate,
        const std::string& eventTime,
        const std::string& departureName,
        const std::string& references,
        const std::string& toleranceTime
    )
    {
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "template";

        json::object tmpl;
        tmpl["name"] = "ticket_confirmation";
        json::object lang;
        lang["code"] = "en";
        tmpl["language"] = lang;

        json::array components;

        // 1. Header Component (1 parameter: customer_name)
        json::object headerComp;
        headerComp["type"] = "header";
        json::array headerParams;
        json::object hParam;
        hParam["type"] = "text";
        hParam["parameter_name"] = "customer_name";
        hParam["text"] = customerName;
        headerParams.push_back(hParam);
        headerComp["parameters"] = headerParams;
        components.push_back(headerComp);

        // 2. Body Component (9 parameters)
        json::object bodyComp;
        bodyComp["type"] = "body";
        json::array bodyParams;

        std::vector<std::pair<std::string, std::string>> paramList = {
            {"customer_name", customerName},
            {"ticket_number", ticketNumber},
            {"trip_type", tripType},
            {"event_name", eventName},
            {"event_date", eventDate},
            {"event_time", eventTime},
            {"departure_name", departureName},
            {"references", references},
            {"tolerance_time", toleranceTime}
        };

        for (const auto& [key, val] : paramList)
        {
            json::object paramObj;
            paramObj["type"] = "text";
            paramObj["parameter_name"] = key;
            paramObj["text"] = val;
            bodyParams.push_back(paramObj);
        }
        bodyComp["parameters"] = bodyParams;
        components.push_back(bodyComp);

        tmpl["components"] = components;
        body["template"] = tmpl;

        std::string jsonStr = json::serialize(body);
        std::string contentStr = "Ticket Confirmation Template";
        return SendRequest(cleanPhone, "TEMPLATE", "ticket_confirmation", contentStr, jsonStr);
    }
} // namespace omnisphere::services
