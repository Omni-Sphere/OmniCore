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

    void WhatsAppService::SetLicenseService(std::shared_ptr<omnisphere::services::LicenseService> licenseService)
    {
        m_licenseService = std::move(licenseService);
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
            std::string rawWabaId = "";
            try {
                if (dt[0].HasColumn("BusinessAccountId") && !dt[0]["BusinessAccountId"].IsNull())
                    rawWabaId = (std::string)dt[0]["BusinessAccountId"];
            } catch (...) {}
            std::string rawWebhookToken = (std::string)dt[0]["WebhookVerifyToken"];

            auto isPlain = [](const std::string& val) -> bool {
                if (val.empty()) return false;
                if (val.rfind("omni_", 0) == 0) return true;
                if (val.rfind("EAA", 0) == 0) return true;
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
            m_config.businessAccountId = decryptVal(rawWabaId);
            m_config.webhookVerifyToken = decryptVal(rawWebhookToken);
            m_config.apiVersion = (std::string)dt[0]["ApiVersion"];
            if (m_config.apiVersion.empty()) m_config.apiVersion = "v24.0";
            try {
                if (dt[0].HasColumn("IsActive") && !dt[0]["IsActive"].IsNull()) m_config.isActive = (bool)dt[0]["IsActive"];
                else if (dt[0].HasColumn("isactive") && !dt[0]["isactive"].IsNull()) m_config.isActive = (bool)dt[0]["isactive"];
                else m_config.isActive = true;
            } catch (...) { m_config.isActive = true; }
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
                    if (val.rfind("EAA", 0) == 0) return true;
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
            if (val.rfind("EAA", 0) == 0) return true;
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
            m_config.isActive = settings.isActive;
        }
        return ok;
    }

    static std::string CleanString(const std::string& raw)
    {
        std::string clean = "";
        for (char c : raw)
        {
            if (c != '\r' && c != '\n' && c != '\t' && c != '"' && c != '\'')
            {
                clean += c;
            }
        }
        size_t start = clean.find_first_not_of(" ");
        if (start == std::string::npos) return "";
        size_t end = clean.find_last_not_of(" ");
        return clean.substr(start, end - start + 1);
    }

    std::string WhatsAppService::ParseMetaErrorMessage(const std::string& rawPayload)
    {
        if (rawPayload.empty()) return "No se pudo establecer conexión con la API de Meta WhatsApp.";

        if (rawPayload.find("<!DOCTYPE") != std::string::npos || rawPayload.find("<html") != std::string::npos || rawPayload.find("4xx Client Error") != std::string::npos)
        {
            return "Error 4xx HTTP devuelto por el servidor de Meta. Verifica que el Token de Acceso (ApiToken) y PhoneId de Meta WhatsApp estén configurados correctamente.";
        }

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

                if (!msg.empty()) return msg;
                if (!details.empty()) return details;
            }
        } catch (...) {}

        return "Error devuelto por la API de Meta WhatsApp: " + rawPayload;
    }

    bool WhatsAppService::SendRequest(
        const std::string& phoneNumber,
        const std::string& messageType,
        const std::string& templateName,
        const std::string& content,
        const std::string& jsonString
    )
    {
        std::string cleanPhoneId = CleanString(m_config.phoneId);
        std::string cleanToken = CleanString(m_config.token);
        std::string cleanApiVersion = CleanString(m_config.apiVersion.empty() ? "v24.0" : m_config.apiVersion);

        if (!m_config.isActive)
        {
            m_lastError = "La integración de Meta WhatsApp API está desactivada en la configuración.";
            omnisphere::utils::Logger::LogInfo("WhatsAppService", "Envío de mensaje cancelado: La integración de WhatsApp está desactivada (IsActive = false).");
            return false;
        }

        if (cleanPhoneId.empty() || cleanToken.empty())
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
            std::string target = "/" + cleanApiVersion + "/" + cleanPhoneId + "/messages";

            omnisphere::utils::Logger::LogInfo("WhatsAppService", "Sending HTTP POST request to https://" + host + target + "\nOutgoing Payload:\n" + jsonString);

            boost::asio::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_peer);

            tcp::resolver resolver(ioc);
            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
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
            req.set(http::field::authorization, "Bearer " + cleanToken);
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
                msg.whatsAppId = wamidCode.empty() ? ("ERR-" + std::to_string(std::time(nullptr))) : wamidCode;
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
        if (clean.length() == 10)
        {
            clean = "52" + clean;
        }
        else if (clean.length() == 13 && clean.rfind("521", 0) == 0)
        {
            clean = "52" + clean.substr(3);
        }
        return clean;
    }

    bool WhatsAppService::SendNotification(
        const std::string& phoneNumber,
        const std::string& templateName,
        const std::vector<std::string>& params
    )
    {
        if (m_licenseService) m_licenseService->RequireModule(omnisphere::license::MODULE_WHATSAPP);
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
        if (m_licenseService) m_licenseService->RequireModule(omnisphere::license::MODULE_WHATSAPP);
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

    static std::string DecodeUnicodeEscapes(const std::string& input)
    {
        std::string result;
        result.reserve(input.size());

        for (size_t i = 0; i < input.size(); ++i)
        {
            if (input[i] == '\\' && i + 5 < input.size() && input[i + 1] == 'u')
            {
                // Check if it's 5 hex digits \uXXXXX (e.g. \u1F68C)
                if (i + 6 < input.size())
                {
                    char c6 = input[i + 6];
                    if ((c6 >= '0' && c6 <= '9') || (c6 >= 'a' && c6 <= 'f') || (c6 >= 'A' && c6 <= 'F'))
                    {
                        uint32_t cp5 = 0;
                        for (int k = 2; k <= 6; ++k)
                        {
                            char c = input[i + k];
                            cp5 <<= 4;
                            if (c >= '0' && c <= '9') cp5 |= (c - '0');
                            else if (c >= 'a' && c <= 'f') cp5 |= (c - 'a' + 10);
                            else if (c >= 'A' && c <= 'F') cp5 |= (c - 'A' + 10);
                        }
                        if (cp5 >= 0x10000 && cp5 <= 0x10FFFF)
                        {
                            result += static_cast<char>(0xF0 | ((cp5 >> 18) & 0x07));
                            result += static_cast<char>(0x80 | ((cp5 >> 12) & 0x3F));
                            result += static_cast<char>(0x80 | ((cp5 >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp5 & 0x3F));
                            i += 6; // skip \uXXXXX
                            continue;
                        }
                    }
                }

                // 4 hex digits \uXXXX
                uint32_t codepoint = 0;
                bool valid = true;
                for (int k = 2; k < 6; ++k)
                {
                    char c = input[i + k];
                    codepoint <<= 4;
                    if (c >= '0' && c <= '9') codepoint |= (c - '0');
                    else if (c >= 'a' && c <= 'f') codepoint |= (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') codepoint |= (c - 'A' + 10);
                    else { valid = false; break; }
                }

                if (valid)
                {
                    // Check for surrogate pair \uD8xx\uDCxx
                    if (codepoint >= 0xD800 && codepoint <= 0xDBFF && i + 11 < input.size() && input[i + 6] == '\\' && input[i + 7] == 'u')
                    {
                        uint32_t lowSurrogate = 0;
                        bool lowValid = true;
                        for (int k = 8; k < 12; ++k)
                        {
                            char c = input[i + k];
                            lowSurrogate <<= 4;
                            if (c >= '0' && c <= '9') lowSurrogate |= (c - '0');
                            else if (c >= 'a' && c <= 'f') lowSurrogate |= (c - 'a' + 10);
                            else if (c >= 'A' && c <= 'F') lowSurrogate |= (c - 'A' + 10);
                            else { lowValid = false; break; }
                        }
                        if (lowValid && lowSurrogate >= 0xDC00 && lowSurrogate <= 0xDFFF)
                        {
                            codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (lowSurrogate - 0xDC00);
                            i += 11;
                        }
                        else
                        {
                            i += 5;
                        }
                    }
                    else
                    {
                        i += 5;
                    }

                    if (codepoint <= 0x7F)
                    {
                        result += static_cast<char>(codepoint);
                    }
                    else if (codepoint <= 0x7FF)
                    {
                        result += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else if (codepoint <= 0xFFFF)
                    {
                        result += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
                        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    else if (codepoint <= 0x10FFFF)
                    {
                        result += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
                        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (codepoint & 0x3F));
                    }
                    continue;
                }
            }
            result += input[i];
        }
        return result;
    }

    bool WhatsAppService::SendMessage(
        const std::string& phoneNumber,
        const std::string& message
    )
    {
        if (m_licenseService) m_licenseService->RequireModule(omnisphere::license::MODULE_WHATSAPP);
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        std::string decodedMessage = DecodeUnicodeEscapes(message);
        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "text";

        json::object textObj;
        textObj["body"] = decodedMessage;
        body["text"] = textObj;

        std::string jsonStr = json::serialize(body);
        return SendRequest(cleanPhone, "TEXT", "", decodedMessage, jsonStr);
    }

    bool WhatsAppService::MarkAsRead(const std::string& wamid) const
    {
        if (wamid.empty() || m_config.phoneId.empty() || m_config.token.empty()) return false;
        try
        {
            json::object body;
            body["messaging_product"] = "whatsapp";
            body["status"] = "read";
            body["message_id"] = wamid;

            std::string jsonStr = json::serialize(body);
            return const_cast<WhatsAppService*>(this)->SendRequest("", "READ_STATUS", "", "", jsonStr);
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("WhatsAppService", "Error marking message [" + wamid + "] as read: " + std::string(ex.what()));
            return false;
        }
    }

    bool WhatsAppService::SendInteractiveButtons(
        const std::string& phoneNumber,
        const std::string& bodyText,
        const std::vector<std::pair<std::string, std::string>>& buttons
    )
    {
        if (m_licenseService) m_licenseService->RequireModule(omnisphere::license::MODULE_WHATSAPP);
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        std::string decodedBody = DecodeUnicodeEscapes(bodyText);
        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "interactive";

        json::object interactiveObj;
        interactiveObj["type"] = "button";

        json::object bodyObj;
        bodyObj["text"] = decodedBody;
        interactiveObj["body"] = bodyObj;

        json::object actionObj;
        json::array btnArray;
        for (const auto& [btnId, btnTitle] : buttons)
        {
            json::object btnObj;
            btnObj["type"] = "reply";

            json::object replyObj;
            replyObj["id"] = btnId;
            replyObj["title"] = DecodeUnicodeEscapes(btnTitle);

            btnObj["reply"] = replyObj;
            btnArray.push_back(btnObj);
        }
        actionObj["buttons"] = btnArray;
        interactiveObj["action"] = actionObj;

        body["interactive"] = interactiveObj;

        std::string jsonStr = json::serialize(body);
        return SendRequest(cleanPhone, "INTERACTIVE", "", decodedBody, jsonStr);
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
        if (m_licenseService) m_licenseService->RequireModule(omnisphere::license::MODULE_WHATSAPP);
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

    std::optional<omnisphere::models::CustomMessage> WhatsAppService::GetCustomMessage(const std::string& messageCode) const
    {
        if (!m_repository) return std::nullopt;
        return m_repository->GetCustomMessageByCode(messageCode);
    }

    bool WhatsAppService::SendCustomMessage(
        const std::string& phoneNumber,
        const std::string& messageCode,
        const std::map<std::string, std::string>& placeholders
    )
    {
        if (m_licenseService) m_licenseService->RequireModule(omnisphere::license::MODULE_WHATSAPP);
        auto msgOpt = GetCustomMessage(messageCode);
        if (!msgOpt.has_value())
        {
            m_lastError = "Custom message code [" + messageCode + "] not found or inactive in database.";
            omnisphere::utils::Logger::LogError("WhatsAppService", m_lastError);
            return false;
        }

        auto msg = msgOpt.value();
        std::string bodyText = msg.bodyTemplate;

        // 1. Crear mapa consolidado con valores por defecto de los parámetros definidos
        std::map<std::string, std::string> finalPlaceholders;
        for (const auto& param : msg.parameters)
        {
            if (param.defaultValue.has_value() && !param.defaultValue.value().empty())
            {
                finalPlaceholders[param.paramKey] = param.defaultValue.value();
            }
        }

        // 2. Sobrescribir con los valores proporcionados explícitamente
        for (const auto& [key, val] : placeholders)
        {
            finalPlaceholders[key] = val;
        }

        // 3. Interpolación de variables dinámicas {key} -> val
        for (const auto& [key, val] : finalPlaceholders)
        {
            std::string token = "{" + key + "}";
            size_t pos = 0;
            while ((pos = bodyText.find(token, pos)) != std::string::npos)
            {
                bodyText.replace(pos, token.length(), val);
                pos += val.length();
            }
        }

        bool success = false;
        if (msg.messageType == "TEMPLATE")
        {
            success = SendNamedNotification(phoneNumber, !msg.bodyTemplate.empty() ? msg.bodyTemplate : messageCode, finalPlaceholders);
        }
        else if (msg.messageType == "INTERACTIVE_BUTTON" && !msg.buttons.empty())
        {
            std::vector<std::pair<std::string, std::string>> btnPairs;
            for (const auto& btn : msg.buttons)
            {
                std::string btnId = btn.buttonId;
                std::string btnTitle = btn.title;
                for (const auto& [key, val] : finalPlaceholders)
                {
                    std::string token = "{" + key + "}";
                    size_t pos = 0;
                    while ((pos = btnId.find(token, pos)) != std::string::npos)
                    {
                        btnId.replace(pos, token.length(), val);
                        pos += val.length();
                    }
                    pos = 0;
                    while ((pos = btnTitle.find(token, pos)) != std::string::npos)
                    {
                        btnTitle.replace(pos, token.length(), val);
                        pos += val.length();
                    }
                }
                btnPairs.push_back({btnId, btnTitle});
            }
            success = SendInteractiveButtons(phoneNumber, bodyText, btnPairs);
        }
        else
        {
            success = SendMessage(phoneNumber, bodyText);
        }

        if (!success && (m_lastError.find("131047") != std::string::npos || m_lastError.find("24 hours") != std::string::npos))
        {
            omnisphere::utils::Logger::LogWarning("WhatsAppService",
                "Custom message failed due to Meta 24h window (131047). Falling back to official Meta template 'ticket_confirmation' for " + phoneNumber);

            std::string name = finalPlaceholders.count("nombre_registrado") ? finalPlaceholders["nombre_registrado"] : (finalPlaceholders.count("nombre_cliente") ? finalPlaceholders["nombre_cliente"] : "Pasajero");
            std::string folio = finalPlaceholders.count("folio") ? finalPlaceholders["folio"] : "RSV000000";
            std::string seats = finalPlaceholders.count("numero_asientos") ? finalPlaceholders["numero_asientos"] : "1 lugar";
            std::string event = finalPlaceholders.count("evento") ? finalPlaceholders["evento"] : "Evento";
            std::string pickup = finalPlaceholders.count("parada_inicial") ? finalPlaceholders["parada_inicial"] : "Punto de Abordaje";
            std::string depTime = finalPlaceholders.count("hora_salida") ? finalPlaceholders["hora_salida"] : "Por confirmar";

            return SendTicketConfirmation(phoneNumber, name, folio, seats, event, "Por confirmar", depTime, pickup, "Sin referencias", "15 minutos");
        }

        return success;
    }

    bool WhatsAppService::HasRecentWelcomeCard(const std::string& phoneNumber, int minutesWindow) const
    {
        if (!m_repository) return false;
        return m_repository->HasRecentWelcomeCard(phoneNumber, minutesWindow);
    }

    omnisphere::dtos::CreateMetaTemplateResult WhatsAppService::CreateMetaTemplate(
        const omnisphere::dtos::CreateMetaTemplateInput& input
    )
    {
        omnisphere::dtos::CreateMetaTemplateResult result;
        result.messageCode = input.name;

        if (input.name.empty() || input.bodyText.empty())
        {
            result.errorMessage = "El nombre de la plantilla y el texto del cuerpo (bodyText) son obligatorios.";
            return result;
        }

        std::string cleanWabaId = CleanString(m_config.businessAccountId);
        std::string cleanToken = CleanString(m_config.token);
        std::string cleanApiVersion = CleanString(m_config.apiVersion.empty() ? "v24.0" : m_config.apiVersion);

        if (cleanWabaId.empty() || cleanToken.empty())
        {
            auto settings = GetSettings({"BusinessAccountId", "ApiToken", "ApiVersion"});
            if (cleanWabaId.empty()) cleanWabaId = CleanString(settings.businessAccountId);
            if (cleanToken.empty()) cleanToken = CleanString(settings.apiToken);
            if (m_config.apiVersion.empty()) cleanApiVersion = CleanString(settings.apiVersion.empty() ? "v24.0" : settings.apiVersion);
        }

        if (cleanWabaId.empty() || cleanToken.empty())
        {
            result.errorMessage = "Configuración incompleta: BusinessAccountId (WABA ID) o ApiToken no están configurados en el sistema.";
            omnisphere::utils::Logger::LogError("WhatsAppService", result.errorMessage);
            return result;
        }

        std::string metaName = input.name;
        std::transform(metaName.begin(), metaName.end(), metaName.begin(), [](unsigned char c) {
            if (c == ' ' || c == '-') return '_';
            return static_cast<char>(std::tolower(c));
        });

        json::object payloadObj;
        payloadObj["name"] = metaName;
        payloadObj["language"] = input.language.empty() ? "es_MX" : input.language;
        payloadObj["category"] = input.category.empty() ? "UTILITY" : input.category;

        json::array componentsArr;

        if (input.headerType == "TEXT" && input.headerText.has_value() && !input.headerText->empty())
        {
            json::object headerObj;
            headerObj["type"] = "HEADER";
            headerObj["format"] = "TEXT";
            headerObj["text"] = *input.headerText;
            componentsArr.push_back(headerObj);
        }

        json::object bodyObj;
        bodyObj["type"] = "BODY";
        bodyObj["text"] = input.bodyText;
        componentsArr.push_back(bodyObj);

        if (input.footerText.has_value() && !input.footerText->empty())
        {
            json::object footerObj;
            footerObj["type"] = "FOOTER";
            footerObj["text"] = *input.footerText;
            componentsArr.push_back(footerObj);
        }

        if (!input.buttons.empty())
        {
            json::object buttonsComponentObj;
            buttonsComponentObj["type"] = "BUTTONS";

            json::array metaButtonsArr;
            for (const auto& btn : input.buttons)
            {
                json::object btnObj;
                if (btn.type == "QUICK_REPLY")
                {
                    btnObj["type"] = "QUICK_REPLY";
                    btnObj["text"] = btn.text;
                }
                else if (btn.type == "URL")
                {
                    btnObj["type"] = "URL";
                    btnObj["text"] = btn.text;
                    btnObj["url"] = btn.url.value_or("");
                }
                else if (btn.type == "PHONE_NUMBER")
                {
                    btnObj["type"] = "PHONE_NUMBER";
                    btnObj["text"] = btn.text;
                    btnObj["phone_number"] = btn.phoneNumber.value_or("");
                }
                metaButtonsArr.push_back(btnObj);
            }
            buttonsComponentObj["buttons"] = metaButtonsArr;
            componentsArr.push_back(buttonsComponentObj);
        }

        payloadObj["components"] = componentsArr;
        std::string jsonString = json::serialize(payloadObj);

        std::string responseBodyStr = "";
        bool requestSuccess = false;

        try
        {
            std::string host = "graph.facebook.com";
            std::string port = "443";
            std::string target = "/" + cleanApiVersion + "/" + cleanWabaId + "/message_templates";

            omnisphere::utils::Logger::LogInfo("WhatsAppService", "Creating Meta Template POST https://" + host + target + "\nOutgoing Payload:\n" + jsonString);

            boost::asio::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_peer);

            tcp::resolver resolver(ioc);
            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

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

            http::request<http::string_body> req{http::verb::post, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::content_type, "application/json");
            req.set(http::field::authorization, "Bearer " + cleanToken);
            req.body() = jsonString;
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(stream, buffer, res);

            responseBodyStr = beast::buffers_to_string(res.body().data());
            requestSuccess = (res.result() == http::status::ok || res.result() == http::status::created);

            beast::error_code ec;
            stream.shutdown(ec);

            if (requestSuccess)
            {
                omnisphere::utils::Logger::LogInfo("WhatsAppService", "Meta Template Creation SUCCESS: " + responseBodyStr);
                try
                {
                    auto parsed = json::parse(responseBodyStr);
                    if (parsed.is_object())
                    {
                        auto pObj = parsed.as_object();
                        if (pObj.contains("id")) result.metaTemplateId = std::string(pObj.at("id").as_string());
                        if (pObj.contains("status")) result.metaStatus = std::string(pObj.at("status").as_string());
                        if (pObj.contains("category")) result.metaCategory = std::string(pObj.at("category").as_string());
                    }
                }
                catch (...) {}

                if (result.metaStatus.empty()) result.metaStatus = "PENDING";
                if (result.metaCategory.empty()) result.metaCategory = input.category;
                result.success = true;

                if (m_repository)
                {
                    omnisphere::models::CustomMessage msg;
                    msg.code = input.name;
                    msg.title = input.title.empty() ? input.name : input.title;
                    msg.messageType = "TEMPLATE";
                    msg.headerType = input.headerType;
                    msg.headerContent = input.headerText;
                    msg.bodyTemplate = input.bodyText;
                    msg.footerText = input.footerText;
                    msg.metaTemplateId = result.metaTemplateId;
                    msg.metaStatus = result.metaStatus;
                    msg.metaCategory = result.metaCategory;
                    msg.isActive = true;

                    int sortIdx = 1;
                    for (const auto& btn : input.buttons)
                    {
                        omnisphere::models::CustomButton cBtn;
                        cBtn.buttonId = "btn_" + std::to_string(sortIdx);
                        cBtn.title = btn.text;
                        cBtn.actionType = btn.type;
                        if (btn.type == "URL") cBtn.actionPayload = btn.url;
                        else if (btn.type == "PHONE_NUMBER") cBtn.actionPayload = btn.phoneNumber;
                        cBtn.sortOrder = sortIdx++;
                        msg.buttons.push_back(cBtn);
                    }

                    m_repository->SaveCustomMessage(msg);
                }
            }
            else
            {
                result.success = false;
                result.errorMessage = ParseMetaErrorMessage(responseBodyStr);
                omnisphere::utils::Logger::LogError("WhatsAppService", "Meta Template Creation FAILED (HTTP " + std::to_string(res.result_int()) + "): " + responseBodyStr);
            }
        }
        catch (const std::exception& ex)
        {
            result.success = false;
            result.errorMessage = "Error de conexión HTTP al crear plantilla en Meta: " + std::string(ex.what());
            omnisphere::utils::Logger::LogError("WhatsAppService", result.errorMessage);
        }

        return result;
    }
} // namespace omnisphere::services
