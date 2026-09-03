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

            auto decryptVal = [](const std::string& val) -> std::string {
                if (val.empty()) return "";
                try {
                    std::string decoded = omnisphere::utils::Base64::Decode(val);
                    if (omnisphere::utils::Base64::Encode(decoded) == val) {
                        return decoded;
                    }
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

                auto ensureEncryptedVal = [&](const std::string& colName) -> std::string {
                    std::string val = getVal(colName);
                    if (val.empty()) return "";
                    try {
                        std::string decoded = omnisphere::utils::Base64::Decode(val);
                        if (omnisphere::utils::Base64::Encode(decoded) == val) {
                            return val; // Already encrypted in DB
                        }
                    } catch (...) {}
                    return omnisphere::utils::Base64::Encode(val);
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

        auto ensureEncrypted = [](const std::string& val) -> std::string {
            if (val.empty()) return "";
            try {
                std::string decoded = omnisphere::utils::Base64::Decode(val);
                if (omnisphere::utils::Base64::Encode(decoded) == val) {
                    return val;
                }
            } catch (...) {}
            return omnisphere::utils::Base64::Encode(val);
        };

        auto decryptVal = [](const std::string& val) -> std::string {
            if (val.empty()) return "";
            try {
                std::string decoded = omnisphere::utils::Base64::Decode(val);
                if (omnisphere::utils::Base64::Encode(decoded) == val) {
                    return decoded;
                }
            } catch (...) {}
            return val;
        };

        omnisphere::models::WhatsAppSettings encSettings = settings;
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
            omnisphere::utils::Logger::LogError("WhatsAppService", "Cannot dispatch WhatsApp message: ApiToken or PhoneId is empty/unconfigured.");
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

            auto const results = resolver.resolve(host, port);
            beast::get_lowest_layer(stream).connect(results);
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
                omnisphere::utils::Logger::LogError("WhatsAppService", "Meta API Request FAILED (HTTP " + std::to_string(res.result_int()) + "). Payload: " + responseBodyStr);
            }

            beast::error_code ec;
            stream.shutdown(ec);
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("WhatsAppService", "WhatsApp HTTP Exception: " + std::string(ex.what()));
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
        lang["code"] = "es_MX";
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
        lang["code"] = "es_MX";
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
        lang["code"] = "es_MX";
        tmpl["language"] = lang;

        json::array components;

        // Header Component (customer_name)
        json::object headerComp;
        headerComp["type"] = "header";
        json::array headerParams;
        json::object hp1;
        hp1["type"] = "text";
        hp1["parameter_name"] = "customer_name";
        hp1["text"] = customerName;
        headerParams.push_back(hp1);
        headerComp["parameters"] = headerParams;
        components.push_back(headerComp);

        // Body Component
        json::object bodyComp;
        bodyComp["type"] = "body";
        json::array bodyParams;

        auto addBodyParam = [&](const std::string& paramName, const std::string& paramVal) {
            json::object p;
            p["type"] = "text";
            p["parameter_name"] = paramName;
            p["text"] = paramVal;
            bodyParams.push_back(p);
        };

        addBodyParam("customer_name", customerName);
        addBodyParam("ticket_number", ticketNumber);
        addBodyParam("trip_type", tripType);
        addBodyParam("event_name", eventName);
        addBodyParam("event_date", eventDate);
        addBodyParam("event_time", eventTime);
        addBodyParam("departure_name", departureName);
        addBodyParam("references", references);
        addBodyParam("tolerance_time", toleranceTime);

        bodyComp["parameters"] = bodyParams;
        components.push_back(bodyComp);

        tmpl["components"] = components;
        body["template"] = tmpl;

        std::string jsonStr = json::serialize(body);
        std::string contentStr = "Ticket Confirmation for " + customerName + " (" + ticketNumber + ")";
        return SendRequest(cleanPhone, "TEMPLATE", "ticket_confirmation", contentStr, jsonStr);
    }
} // namespace omnisphere::services
