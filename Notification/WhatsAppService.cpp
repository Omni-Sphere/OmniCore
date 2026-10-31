#include "Notification/WhatsAppService.hpp"
#include <OmniData/DataMapper.hpp>
#include <OmniData/Database.hpp>
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
#include <fstream>
#include <filesystem>
#include <ctime>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace json = boost::json;
namespace fs = std::filesystem;
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

    void WhatsAppService::CheckLicense() const
    {
        auto lic = m_licenseService ? m_licenseService : LicenseService::GetSharedInstance();
        if (!lic || !lic->IsModuleLicensed(omnisphere::license::MODULE_WHATSAPP))
        {
            throw omnisphere::services::LicenseException(
                "Acceso denegado: El módulo 'MODULE_WHATSAPP' no está incluido en la licencia activa de OmniSphere o se encuentra desactivado."
            );
        }
    }

    bool WhatsAppService::InitializeFromDatabase(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
    {
        if (!dbPool) return false;
        m_repository = std::make_shared<omnisphere::repositories::WhatsAppRepository>(dbPool);
        auto dt = m_repository->GetSettings();
        if (!dt.IsEmpty())
        {
            const auto& row = dt[0];
            std::string rawPhoneId = row["PhoneId"].GetOptional<std::string>().value_or("");
            std::string rawToken = row["ApiToken"].GetOptional<std::string>().value_or("");
            std::string rawWabaId = row["BusinessAccountId"].GetOptional<std::string>().value_or("");
            std::string rawWebhookToken = row["WebhookVerifyToken"].GetOptional<std::string>().value_or("");

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
            m_config.apiVersion = row["ApiVersion"].GetOptional<std::string>().value_or("v24.0");
            m_config.isActive = row["IsActive"].GetOptional<bool>().value_or(true);
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
            if (!dt.IsEmpty())
            {
                settings = omnisphere::types::FromDataRow<omnisphere::models::WhatsAppSettings>(dt[0]);

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

                auto ensureEncryptedVal = [&](std::string& val) {
                    if (!val.empty() && isPlain(val)) {
                        val = omnisphere::utils::Base64::Encode(val);
                    }
                };

                ensureEncryptedVal(settings.phoneId);
                ensureEncryptedVal(settings.apiToken);
                ensureEncryptedVal(settings.businessAccountId);
                ensureEncryptedVal(settings.webhookVerifyToken);
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

                // Si Meta envía un mensaje explicativo directo al usuario (ej. plantilla en proceso de eliminación)
                std::string userTitle;
                std::string userMsg;
                if (errObj.contains("error_user_title") && errObj.at("error_user_title").is_string()) {
                    userTitle = std::string(errObj.at("error_user_title").as_string());
                }
                if (errObj.contains("error_user_msg") && errObj.at("error_user_msg").is_string()) {
                    userMsg = std::string(errObj.at("error_user_msg").as_string());
                }
                if (!userMsg.empty()) {
                    return !userTitle.empty() ? (userTitle + ": " + userMsg) : userMsg;
                }

                if (code == 131058 || msg.find("Public Test Numbers") != std::string::npos) {
                    return "La plantilla de prueba por defecto (hello_world) solo puede enviarse desde números de prueba de Meta. Configura una plantilla aprobada para tu número real.";
                }
                if (code == 132001 || msg.find("does not exist") != std::string::npos || details.find("does not exist") != std::string::npos) {
                    return "La plantilla solicitada no está registrada o aprobada en Meta WhatsApp para el idioma configurado.";
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
                if (code == 100) {
                    if (!details.empty()) return details;
                    if (!msg.empty() && msg != "Invalid parameter") return msg;
                    return "Parámetros inválidos en la plantilla enviada a Meta.";
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
        try
        {
            CheckLicense();
        }
        catch (const std::exception& ex)
        {
            m_lastError = "Licencia: " + std::string(ex.what());
            omnisphere::utils::Logger::LogWarning("WhatsAppService", "Envío de mensaje cancelado: " + m_lastError);
            return false;
        }

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
        return SendNotification(phoneNumber, templateName, params, std::nullopt);
    }

    bool WhatsAppService::SendNotification(
        const std::string& phoneNumber,
        const std::string& templateName,
        const std::vector<std::string>& params,
        const std::optional<WhatsAppMediaHeader>& mediaHeader
    )
    {
        CheckLicense();
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "template";

        std::string langCode = "es_MX";
        if (m_dbPool)
        {
            auto waRepo = std::make_shared<omnisphere::repositories::WhatsAppRepository>(m_dbPool);
            auto tplOpt = waRepo->GetWhatsAppTemplateByCode(templateName);
            if (tplOpt.has_value() && !tplOpt->language.empty())
            {
                langCode = tplOpt->language;
            }
        }

        json::object tmpl;
        tmpl["name"] = templateName;
        json::object lang;
        lang["code"] = langCode;
        tmpl["language"] = lang;

        json::array components;

        // 1. Header Component (Media: IMAGE, VIDEO, DOCUMENT, LOCATION)
        if (mediaHeader.has_value())
        {
            json::object headerComp;
            headerComp["type"] = "header";
            json::array parameters;

            std::string hType = mediaHeader->type;
            if (hType == "IMAGE")
            {
                json::object paramObj;
                paramObj["type"] = "image";
                json::object imgObj;
                imgObj["link"] = mediaHeader->mediaUrl;
                paramObj["image"] = imgObj;
                parameters.push_back(paramObj);
            }
            else if (hType == "VIDEO")
            {
                json::object paramObj;
                paramObj["type"] = "video";
                json::object vidObj;
                vidObj["link"] = mediaHeader->mediaUrl;
                paramObj["video"] = vidObj;
                parameters.push_back(paramObj);
            }
            else if (hType == "DOCUMENT")
            {
                json::object paramObj;
                paramObj["type"] = "document";
                json::object docObj;
                docObj["link"] = mediaHeader->mediaUrl;
                if (!mediaHeader->filename.empty())
                {
                    docObj["filename"] = mediaHeader->filename;
                }
                paramObj["document"] = docObj;
                parameters.push_back(paramObj);
            }
            else if (hType == "LOCATION")
            {
                json::object paramObj;
                paramObj["type"] = "location";
                json::object locObj;
                locObj["latitude"] = std::to_string(mediaHeader->latitude);
                locObj["longitude"] = std::to_string(mediaHeader->longitude);
                if (!mediaHeader->locationName.empty()) locObj["name"] = mediaHeader->locationName;
                if (!mediaHeader->locationAddress.empty()) locObj["address"] = mediaHeader->locationAddress;
                paramObj["location"] = locObj;
                parameters.push_back(paramObj);
            }

            if (!parameters.empty())
            {
                headerComp["parameters"] = parameters;
                components.push_back(headerComp);
            }
        }

        // 2. Body Component (Positional Variables)
        if (!params.empty())
        {
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
        }
        
        if (!components.empty())
        {
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
        const std::map<std::string, std::string>& params,
        const std::string& interpolatedContent,
        const std::optional<WhatsAppMediaHeader>& mediaHeader
    )
    {
        CheckLicense();
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "template";

        std::string langCode = "es_MX";
        if (m_dbPool)
        {
            auto waRepo = std::make_shared<omnisphere::repositories::WhatsAppRepository>(m_dbPool);
            auto tplOpt = waRepo->GetWhatsAppTemplateByCode(templateName);
            if (tplOpt.has_value() && !tplOpt->language.empty())
            {
                langCode = tplOpt->language;
            }
        }

        json::object tmpl;
        tmpl["name"] = templateName;
        json::object lang;
        lang["code"] = langCode;
        tmpl["language"] = lang;

        json::array components;

        // 1. Header Component (Media: IMAGE, VIDEO, DOCUMENT, LOCATION)
        if (mediaHeader.has_value())
        {
            json::object headerComp;
            headerComp["type"] = "header";
            json::array parameters;

            std::string hType = mediaHeader->type;
            if (hType == "IMAGE")
            {
                json::object paramObj;
                paramObj["type"] = "image";
                json::object imgObj;
                imgObj["link"] = mediaHeader->mediaUrl;
                paramObj["image"] = imgObj;
                parameters.push_back(paramObj);
            }
            else if (hType == "VIDEO")
            {
                json::object paramObj;
                paramObj["type"] = "video";
                json::object vidObj;
                vidObj["link"] = mediaHeader->mediaUrl;
                paramObj["video"] = vidObj;
                parameters.push_back(paramObj);
            }
            else if (hType == "DOCUMENT")
            {
                json::object paramObj;
                paramObj["type"] = "document";
                json::object docObj;
                docObj["link"] = mediaHeader->mediaUrl;
                if (!mediaHeader->filename.empty())
                {
                    docObj["filename"] = mediaHeader->filename;
                }
                paramObj["document"] = docObj;
                parameters.push_back(paramObj);
            }
            else if (hType == "LOCATION")
            {
                json::object paramObj;
                paramObj["type"] = "location";
                json::object locObj;
                locObj["latitude"] = std::to_string(mediaHeader->latitude);
                locObj["longitude"] = std::to_string(mediaHeader->longitude);
                if (!mediaHeader->locationName.empty()) locObj["name"] = mediaHeader->locationName;
                if (!mediaHeader->locationAddress.empty()) locObj["address"] = mediaHeader->locationAddress;
                paramObj["location"] = locObj;
                parameters.push_back(paramObj);
            }

            if (!parameters.empty())
            {
                headerComp["parameters"] = parameters;
                components.push_back(headerComp);
            }
        }

        // 2. Body Component (Named Variables)
        if (!params.empty())
        {
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
        }

        if (!components.empty())
        {
            tmpl["components"] = components;
        }

        body["template"] = tmpl;

        std::string jsonStr = json::serialize(body);
        std::string contentStr = !interpolatedContent.empty() ? interpolatedContent : ("Named Template: " + templateName);
        return SendRequest(cleanPhone, "TEMPLATE", templateName, contentStr, jsonStr);
    }

    bool WhatsAppService::SendMetaTemplate(
        const std::string& phoneNumber,
        const omnisphere::models::WhatsAppTemplate& tpl,
        const std::map<std::string, std::string>& availableValues,
        const std::optional<WhatsAppMediaHeader>& mediaHeader
    )
    {
        CheckLicense();
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);

        std::string actualTemplateName = !tpl.templateName.empty() ? tpl.templateName : tpl.code;
        if (actualTemplateName.empty())
        {
            omnisphere::utils::Logger::LogError("WhatsAppService", "[SendMetaTemplate] Nombre y código de plantilla vacíos en el modelo.");
            return false;
        }

        std::string langCode = !tpl.language.empty() ? tpl.language : "es_MX";

        json::object body;
        body["messaging_product"] = "whatsapp";
        body["to"] = cleanPhone;
        body["type"] = "template";

        json::object tmpl;
        json::array components;

        // 1. Header (Variables o Media)
        if (tpl.header.has_value())
        {
            const auto& h = *tpl.header;
            json::object headerComp;
            headerComp["type"] = "header";
            json::array hParams;

            if (h.headerType == "TEXT" && !h.variables.empty())
            {
                for (const auto& var : h.variables)
                {
                    std::string val = availableValues.count(var.varName) ? availableValues.at(var.varName) : "";
                    if (val.empty() && var.exampleValue.has_value()) val = *var.exampleValue;
                    if (val.empty()) val = "N/A";

                    json::object p;
                    p["type"] = "text";
                    p["parameter_name"] = var.varName;
                    p["text"] = val;
                    hParams.push_back(p);
                }
            }
            else if (mediaHeader.has_value())
            {
                std::string hType = mediaHeader->type;
                if (hType == "IMAGE")
                {
                    json::object p;
                    p["type"] = "image";
                    json::object img;
                    img["link"] = mediaHeader->mediaUrl;
                    p["image"] = img;
                    hParams.push_back(p);
                }
                else if (hType == "VIDEO")
                {
                    json::object p;
                    p["type"] = "video";
                    json::object vid;
                    vid["link"] = mediaHeader->mediaUrl;
                    p["video"] = vid;
                    hParams.push_back(p);
                }
                else if (hType == "DOCUMENT")
                {
                    json::object p;
                    p["type"] = "document";
                    json::object doc;
                    doc["link"] = mediaHeader->mediaUrl;
                    if (!mediaHeader->filename.empty()) doc["filename"] = mediaHeader->filename;
                    p["document"] = doc;
                    hParams.push_back(p);
                }
            }

            if (!hParams.empty())
            {
                headerComp["parameters"] = hParams;
                components.push_back(headerComp);
            }
        }
        else if (mediaHeader.has_value())
        {
            json::object headerComp;
            headerComp["type"] = "header";
            json::array hParams;
            json::object p;
            p["type"] = "image";
            json::object img;
            img["link"] = mediaHeader->mediaUrl;
            p["image"] = img;
            hParams.push_back(p);
            headerComp["parameters"] = hParams;
            components.push_back(headerComp);
        }

        // 2. Body (Variables exactas registradas en el objeto de la plantilla)
        if (tpl.body.has_value() && !tpl.body->variables.empty())
        {
            json::object bodyComp;
            bodyComp["type"] = "body";
            json::array bParams;

            for (const auto& var : tpl.body->variables)
            {
                std::string val = availableValues.count(var.varName) ? availableValues.at(var.varName) : "";
                if (val.empty() && var.exampleValue.has_value()) val = *var.exampleValue;
                if (val.empty()) val = "Por confirmar";

                json::object p;
                p["type"] = "text";
                p["parameter_name"] = var.varName;
                p["text"] = val;
                bParams.push_back(p);
            }

            if (!bParams.empty())
            {
                bodyComp["parameters"] = bParams;
                components.push_back(bodyComp);
            }
        }
        else if (!availableValues.empty())
        {
            json::object bodyComp;
            bodyComp["type"] = "body";
            json::array bParams;
            for (const auto& [k, v] : availableValues)
            {
                if (!v.empty())
                {
                    json::object p;
                    p["type"] = "text";
                    p["parameter_name"] = k;
                    p["text"] = v;
                    bParams.push_back(p);
                }
            }
            if (!bParams.empty())
            {
                bodyComp["parameters"] = bParams;
                components.push_back(bodyComp);
            }
        }

        tmpl["name"] = actualTemplateName;
        json::object langObj;
        langObj["code"] = langCode;
        tmpl["language"] = langObj;

        if (!components.empty())
        {
            tmpl["components"] = components;
        }
        body["template"] = tmpl;

        std::string jsonStr = json::serialize(body);
        std::string contentStr = "Template: " + actualTemplateName;
        omnisphere::utils::Logger::LogInfo("WhatsAppService",
            "[SendMetaTemplate] Despachando plantilla [" + actualTemplateName + "] (" + langCode + ") a " + cleanPhone);

        return SendRequest(cleanPhone, "TEMPLATE", actualTemplateName, contentStr, jsonStr);
    }

    bool WhatsAppService::SendMetaTemplate(
        const std::string& phoneNumber,
        const std::string& templateCodeOrName,
        const std::map<std::string, std::string>& availableValues,
        const std::optional<WhatsAppMediaHeader>& mediaHeader
    )
    {
        if (templateCodeOrName.empty())
        {
            omnisphere::utils::Logger::LogError("WhatsAppService", "[SendMetaTemplate] Nombre o código de plantilla vacío.");
            return false;
        }

        auto waRepo = std::make_shared<omnisphere::repositories::WhatsAppRepository>(m_dbPool);
        auto tplOpt = waRepo->GetWhatsAppTemplateByCode(templateCodeOrName);

        if (tplOpt.has_value())
        {
            return SendMetaTemplate(phoneNumber, *tplOpt, availableValues, mediaHeader);
        }

        omnisphere::models::WhatsAppTemplate fallbackTpl;
        fallbackTpl.code = templateCodeOrName;
        fallbackTpl.templateName = templateCodeOrName;
        fallbackTpl.language = "es_MX";
        return SendMetaTemplate(phoneNumber, fallbackTpl, availableValues, mediaHeader);
    }

    static std::string EncodeToUnicodeEscapes(const std::string& input)
    {
        std::string result;
        result.reserve(input.size() * 2);

        for (size_t i = 0; i < input.size();)
        {
            unsigned char c = static_cast<unsigned char>(input[i]);
            if (c < 0x80)
            {
                result += static_cast<char>(c);
                i += 1;
            }
            else if ((c & 0xE0) == 0xC0 && i + 1 < input.size())
            {
                // 2-byte UTF-8
                uint32_t cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(input[i + 1]) & 0x3F);
                char hexBuf[10];
                std::snprintf(hexBuf, sizeof(hexBuf), "\\u%04X", cp);
                result += hexBuf;
                i += 2;
            }
            else if ((c & 0xF0) == 0xE0 && i + 2 < input.size())
            {
                // 3-byte UTF-8
                uint32_t cp = ((c & 0x0F) << 12) |
                              ((static_cast<unsigned char>(input[i + 1]) & 0x3F) << 6) |
                              (static_cast<unsigned char>(input[i + 2]) & 0x3F);
                char hexBuf[10];
                std::snprintf(hexBuf, sizeof(hexBuf), "\\u%04X", cp);
                result += hexBuf;
                i += 3;
            }
            else if ((c & 0xF8) == 0xF0 && i + 3 < input.size())
            {
                // 4-byte UTF-8 (Emojis y caracteres suplementarios >= 0x10000)
                uint32_t cp = ((c & 0x07) << 18) |
                              ((static_cast<unsigned char>(input[i + 1]) & 0x3F) << 12) |
                              ((static_cast<unsigned char>(input[i + 2]) & 0x3F) << 6) |
                              (static_cast<unsigned char>(input[i + 3]) & 0x3F);
                char hexBuf[12];
                std::snprintf(hexBuf, sizeof(hexBuf), "\\u%05X", cp);
                result += hexBuf;
                i += 4;
            }
            else
            {
                result += static_cast<char>(c);
                i += 1;
            }
        }
        return result;
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
        CheckLicense();
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
        CheckLicense();
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

    bool WhatsAppService::SetConversationContext(const std::string& phoneNumber, const std::string& reservationCode)
    {
        if (!m_repository) return false;
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        return m_repository->SetConversationContext(cleanPhone, reservationCode);
    }

    std::optional<std::string> WhatsAppService::GetConversationContext(const std::string& phoneNumber) const
    {
        if (!m_repository) return std::nullopt;
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        return m_repository->GetConversationContext(cleanPhone);
    }

    bool WhatsAppService::ClearConversationContext(const std::string& phoneNumber)
    {
        if (!m_repository) return false;
        std::string cleanPhone = SanitizePhoneNumber(phoneNumber);
        return m_repository->ClearConversationContext(cleanPhone);
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
        CheckLicense();
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
        CheckLicense();
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
            std::string tmplNameToUse = !msg.metaTemplateId.value_or("").empty() ? msg.code : (!msg.bodyTemplate.empty() ? msg.bodyTemplate : messageCode);
            success = SendNamedNotification(phoneNumber, tmplNameToUse, finalPlaceholders, bodyText);
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

        try
        {
            CheckLicense();
        }
        catch (const std::exception& ex)
        {
            result.success = false;
            result.errorMessage = "Módulo Meta WhatsApp no licenciado o desactivado: " + std::string(ex.what());
            omnisphere::utils::Logger::LogWarning("WhatsAppService", result.errorMessage);
            return result;
        }

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
        payloadObj["parameter_format"] = "NAMED";

        json::array componentsArr;

        // 1. HEADER Component (TEXT, IMAGE, VIDEO, DOCUMENT, LOCATION)
        if (input.headerType == "TEXT" && input.headerText.has_value() && !input.headerText->empty())
        {
            json::object headerObj;
            headerObj["type"] = "HEADER";
            headerObj["format"] = "TEXT";

            if (!input.headerVariables.empty())
            {
                json::object exampleObj;
                json::array namedParamsArr;
                for (const auto& hVar : input.headerVariables)
                {
                    json::object paramObj;
                    paramObj["param_name"] = hVar.key;
                    paramObj["example"] = boost::json::string_view(!hVar.example.empty() ? hVar.example : "Ejemplo");
                    namedParamsArr.push_back(paramObj);
                }
                exampleObj["header_text_named_params"] = namedParamsArr;
                headerObj["example"] = exampleObj;
            }
            headerObj["text"] = DecodeUnicodeEscapes(*input.headerText);
            componentsArr.push_back(headerObj);
        }
        else if (input.headerType == "IMAGE" || input.headerType == "VIDEO" || input.headerType == "DOCUMENT" || input.headerType == "LOCATION")
        {
            json::object headerObj;
            headerObj["type"] = "HEADER";
            headerObj["format"] = input.headerType;
            componentsArr.push_back(headerObj);
        }

        // 2. BODY Component
        json::object bodyObj;
        bodyObj["type"] = "BODY";

        if (!input.bodyVariables.empty())
        {
            json::object exampleObj;
            json::array namedParamsArr;
            for (const auto& bVar : input.bodyVariables)
            {
                json::object paramObj;
                paramObj["param_name"] = bVar.key;
                paramObj["example"] = boost::json::string_view(!bVar.example.empty() ? bVar.example : "Ejemplo");
                namedParamsArr.push_back(paramObj);
            }
            exampleObj["body_text_named_params"] = namedParamsArr;
            bodyObj["example"] = exampleObj;
        }
        bodyObj["text"] = DecodeUnicodeEscapes(input.bodyText);
        componentsArr.push_back(bodyObj);

        // 3. FOOTER Component
        if (input.footerText.has_value() && !input.footerText->empty())
        {
            json::object footerObj;
            footerObj["type"] = "FOOTER";
            footerObj["text"] = DecodeUnicodeEscapes(*input.footerText);
            componentsArr.push_back(footerObj);
        }


        // 4. BUTTONS Component
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
                    btnObj["text"] = DecodeUnicodeEscapes(btn.text);
                }
                else if (btn.type == "URL")
                {
                    btnObj["type"] = "URL";
                    btnObj["text"] = DecodeUnicodeEscapes(btn.text);
                    btnObj["url"] = btn.url.value_or("");
                }
                else if (btn.type == "PHONE_NUMBER")
                {
                    btnObj["type"] = "PHONE_NUMBER";
                    btnObj["text"] = DecodeUnicodeEscapes(btn.text);
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
                    // Modular Architecture Persistence in WhatsAppTemplates (codificado con escapes Unicode)
                    omnisphere::models::WhatsAppTemplate wTmpl;

                    wTmpl.code = input.name;
                    wTmpl.templateName = input.title.empty() ? input.name : EncodeToUnicodeEscapes(input.title);
                    wTmpl.language = input.language.empty() ? "es_MX" : input.language;
                    wTmpl.category = result.metaCategory;
                    wTmpl.metaTemplateId = result.metaTemplateId;
                    wTmpl.status = result.metaStatus;
                    wTmpl.isActive = true;
                    wTmpl.createdBy = "system";

                    if (input.headerType != "NONE")
                    {
                        omnisphere::models::WhatsAppTemplateHeader h;
                        h.headerType = input.headerType;
                        h.textContent = input.headerText.has_value() ? std::make_optional(EncodeToUnicodeEscapes(*input.headerText)) : std::nullopt;
                        int hIdx = 1;
                        for (const auto& hv : input.headerVariables)
                        {
                            omnisphere::models::WhatsAppTemplateHeaderVariable hVar;
                            hVar.varName = hv.key;
                            hVar.varIndex = hIdx++;
                            hVar.exampleValue = !hv.example.empty() ? std::make_optional(EncodeToUnicodeEscapes(hv.example)) : std::nullopt;
                            h.variables.push_back(hVar);
                        }
                        wTmpl.header = h;
                    }

                    omnisphere::models::WhatsAppTemplateBody b;
                    b.bodyText = EncodeToUnicodeEscapes(input.bodyText);
                    int bIdx = 1;
                    for (const auto& bv : input.bodyVariables)
                    {
                        omnisphere::models::WhatsAppTemplateBodyVariable bVar;
                        bVar.varName = bv.key;
                        bVar.varIndex = bIdx++;
                        bVar.exampleValue = !bv.example.empty() ? std::make_optional(EncodeToUnicodeEscapes(bv.example)) : std::nullopt;
                        b.variables.push_back(bVar);
                    }
                    wTmpl.body = b;

                    if (input.footerText.has_value() && !input.footerText->empty())
                    {
                        omnisphere::models::WhatsAppTemplateFooter f;
                        f.footerText = EncodeToUnicodeEscapes(*input.footerText);
                        int fIdx = 1;
                        for (const auto& fv : input.footerVariables)
                        {
                            omnisphere::models::WhatsAppTemplateFooterVariable fVar;
                            fVar.varName = fv.key;
                            fVar.varIndex = fIdx++;
                            fVar.exampleValue = !fv.example.empty() ? std::make_optional(EncodeToUnicodeEscapes(fv.example)) : std::nullopt;
                            f.variables.push_back(fVar);
                        }
                        wTmpl.footer = f;
                    }

                    int btnSort = 1;
                    for (const auto& btn : input.buttons)
                    {
                        omnisphere::models::WhatsAppTemplateButton tBtn;
                        tBtn.buttonType = btn.type;
                        tBtn.title = EncodeToUnicodeEscapes(btn.text);
                        if (btn.type == "URL") tBtn.payload = btn.url;
                        else if (btn.type == "PHONE_NUMBER") tBtn.payload = btn.phoneNumber;
                        tBtn.sortOrder = btnSort++;
                        wTmpl.buttons.push_back(tBtn);
                    }

                    m_repository->SaveWhatsAppTemplate(wTmpl);
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

    bool WhatsAppService::UpdateWhatsAppTemplate(
        const omnisphere::dtos::UpdateWhatsAppTemplateInput& input,
        const std::vector<std::string>& mutationFields
    )
    {
        if (!m_repository) return false;
        std::string cleanCode = CleanString(input.Code);
        if (cleanCode.empty()) return false;

        omnisphere::dtos::UpdateWhatsAppTemplateInput cleanInput = input;
        cleanInput.Code = cleanCode;

        if (input.TemplateName.has_value() && !input.TemplateName->empty())
        {
            cleanInput.TemplateName = EncodeToUnicodeEscapes(*input.TemplateName);
        }
        if (input.BodyText.has_value() && !input.BodyText->empty())
        {
            cleanInput.BodyText = EncodeToUnicodeEscapes(*input.BodyText);
        }
        if (input.HeaderText.has_value() && !input.HeaderText->empty())
        {
            cleanInput.HeaderText = EncodeToUnicodeEscapes(*input.HeaderText);
        }
        if (input.FooterText.has_value() && !input.FooterText->empty())
        {
            cleanInput.FooterText = EncodeToUnicodeEscapes(*input.FooterText);
        }

        return m_repository->UpdateWhatsAppTemplate(cleanInput, mutationFields);
    }

    bool WhatsAppService::DeleteMetaTemplate(const std::string& templateCode)
    {
        std::string cleanCode = CleanString(templateCode);
        if (cleanCode.empty() || cleanCode == "hello_world")
        {
            omnisphere::utils::Logger::LogWarning("WhatsAppService", "DeleteMetaTemplate: No se puede eliminar plantilla vacía o plantilla base del sistema (hello_world)");
            return false;
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

        bool metaDeleted = false;

        if (!cleanWabaId.empty() && !cleanToken.empty())
        {
            try
            {
                std::string host = "graph.facebook.com";
                std::string port = "443";
                std::string target = "/" + cleanApiVersion + "/" + cleanWabaId + "/message_templates?name=" + cleanCode;

                boost::asio::io_context ioc;
                ssl::context ctx(ssl::context::tlsv12_client);
                ctx.set_default_verify_paths();
                ctx.set_verify_mode(ssl::verify_peer);

                tcp::resolver resolver(ioc);
                beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

                if (SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
                {
                    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
                    auto const results = resolver.resolve(tcp::v4(), host, port);
                    beast::get_lowest_layer(stream).connect(results);
                    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
                    stream.handshake(ssl::stream_base::client);

                    http::request<http::empty_body> req{http::verb::delete_, target, 11};
                    req.set(http::field::host, host);
                    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
                    req.set(http::field::authorization, "Bearer " + cleanToken);

                    http::write(stream, req);

                    beast::flat_buffer buffer;
                    http::response<http::dynamic_body> res;
                    http::read(stream, buffer, res);

                    std::string responseBodyStr = beast::buffers_to_string(res.body().data());
                    beast::error_code ec;
                    stream.shutdown(ec);

                    if (res.result() == http::status::ok)
                    {
                        metaDeleted = true;
                        omnisphere::utils::Logger::LogInfo("WhatsAppService", "Meta Template DELETE SUCCESS: " + cleanCode + " -> " + responseBodyStr);
                    }
                    else
                    {
                        omnisphere::utils::Logger::LogWarning("WhatsAppService", "Meta Template DELETE returned HTTP " + std::to_string(res.result_int()) + ": " + responseBodyStr);
                        // Si Meta ya la había eliminado o no existe en Meta (404/error), procedemos a eliminarla de la BD local
                        metaDeleted = true;
                    }
                }
            }
            catch (const std::exception& ex)
            {
                omnisphere::utils::Logger::LogError("WhatsAppService", "DeleteMetaTemplate HTTP Exception: " + std::string(ex.what()));
            }
        }
        else
        {
            // Sin credenciales de Meta configuradas, se procede al menos en la base de datos local
            metaDeleted = true;
        }

        bool dbDeleted = false;
        if (m_repository)
        {
            dbDeleted = m_repository->DeleteWhatsAppTemplate(cleanCode);
            if (!dbDeleted)
            {
                omnisphere::utils::Logger::LogError("WhatsAppService", "DeleteMetaTemplate: Falló la eliminación de la plantilla en la base de datos local para el código [" + cleanCode + "]");
            }
        }

        return metaDeleted && dbDeleted;
    }

    bool WhatsAppService::SyncTemplatesFromMeta()
    {
        if (!m_repository) return false;

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
            omnisphere::utils::Logger::LogError("WhatsAppService", "SyncTemplatesFromMeta: Falta BusinessAccountId o ApiToken");
            return false;
        }

        try
        {
            std::string host = "graph.facebook.com";
            std::string port = "443";
            std::string target = "/" + cleanApiVersion + "/" + cleanWabaId + "/message_templates?limit=100";

            boost::asio::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_peer);

            tcp::resolver resolver(ioc);
            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                return false;
            }

            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            auto const results = resolver.resolve(tcp::v4(), host, port);
            beast::get_lowest_layer(stream).connect(results);
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            stream.handshake(ssl::stream_base::client);

            http::request<http::string_body> req{http::verb::get, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::authorization, "Bearer " + cleanToken);
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(stream, buffer, res);

            std::string responseBodyStr = beast::buffers_to_string(res.body().data());
            beast::error_code ec;
            stream.shutdown(ec);

            if (res.result() == http::status::ok)
            {
                auto parsed = json::parse(responseBodyStr);
                if (parsed.is_object() && parsed.as_object().contains("data") && parsed.as_object().at("data").is_array())
                {
                    auto templatesArr = parsed.as_object().at("data").as_array();
                    for (const auto& item : templatesArr)
                    {
                        if (!item.is_object()) continue;
                        auto obj = item.as_object();
                        std::string tName = obj.contains("name") ? std::string(obj.at("name").as_string()) : "";
                        if (tName.empty()) continue;

                        omnisphere::models::WhatsAppTemplate wTmpl;
                        wTmpl.code = tName;
                        wTmpl.templateName = tName;
                        if (obj.contains("language")) wTmpl.language = std::string(obj.at("language").as_string());
                        if (obj.contains("category")) wTmpl.category = std::string(obj.at("category").as_string());
                        if (obj.contains("status")) wTmpl.status = std::string(obj.at("status").as_string());
                        if (obj.contains("id")) wTmpl.metaTemplateId = std::string(obj.at("id").as_string());
                        wTmpl.isActive = true;
                        wTmpl.createdBy = "meta_sync";

                        if (obj.contains("components") && obj.at("components").is_array())
                        {
                            for (const auto& c : obj.at("components").as_array())
                            {
                                if (!c.is_object()) continue;
                                auto cObj = c.as_object();
                                std::string type = cObj.contains("type") ? std::string(cObj.at("type").as_string()) : "";

                                if (type == "HEADER")
                                {
                                    omnisphere::models::WhatsAppTemplateHeader h;
                                    std::string format = cObj.contains("format") ? std::string(cObj.at("format").as_string()) : "TEXT";
                                    h.headerType = format;
                                    if (cObj.contains("text")) h.textContent = std::string(cObj.at("text").as_string());
                                    wTmpl.header = h;
                                }
                                else if (type == "BODY")
                                {
                                    omnisphere::models::WhatsAppTemplateBody b;
                                    if (cObj.contains("text")) b.bodyText = std::string(cObj.at("text").as_string());
                                    wTmpl.body = b;
                                }
                                else if (type == "FOOTER")
                                {
                                    omnisphere::models::WhatsAppTemplateFooter f;
                                    if (cObj.contains("text")) f.footerText = std::string(cObj.at("text").as_string());
                                    wTmpl.footer = f;
                                }
                                else if (type == "BUTTONS" && cObj.contains("buttons") && cObj.at("buttons").is_array())
                                {
                                    int bOrder = 1;
                                    for (const auto& btnVal : cObj.at("buttons").as_array())
                                    {
                                        if (!btnVal.is_object()) continue;
                                        auto btnObj = btnVal.as_object();
                                        omnisphere::models::WhatsAppTemplateButton btn;
                                        if (btnObj.contains("type")) btn.buttonType = std::string(btnObj.at("type").as_string());
                                        if (btnObj.contains("text")) btn.title = std::string(btnObj.at("text").as_string());
                                        if (btnObj.contains("url")) btn.payload = std::string(btnObj.at("url").as_string());
                                        else if (btnObj.contains("phone_number")) btn.payload = std::string(btnObj.at("phone_number").as_string());
                                        btn.sortOrder = bOrder++;
                                        wTmpl.buttons.push_back(btn);
                                    }
                                }
                            }
                        }

                        m_repository->SaveWhatsAppTemplate(wTmpl);
                    }
                    return true;
                }
            }
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("WhatsAppService", "SyncTemplatesFromMeta Exception: " + std::string(ex.what()));
        }

        return false;
    }

    std::string WhatsAppService::DownloadMedia(
        const std::string& mediaId,
        const std::string& mimeType,
        const std::string& suggestedFilename
    )
    {
        if (mediaId.empty()) return "";

        std::string cleanToken = CleanString(m_config.token);
        std::string cleanApiVersion = CleanString(m_config.apiVersion.empty() ? "v24.0" : m_config.apiVersion);

        if (cleanToken.empty())
        {
            auto settings = GetSettings({"ApiToken", "ApiVersion"});
            cleanToken = CleanString(settings.apiToken);
            if (m_config.apiVersion.empty()) cleanApiVersion = CleanString(settings.apiVersion.empty() ? "v24.0" : settings.apiVersion);
        }

        if (cleanToken.empty())
        {
            omnisphere::utils::Logger::LogError("WhatsAppService", "DownloadMedia: Falta ApiToken configurado");
            return "";
        }

        try
        {
            // 1. Obtener la URL temporal de descarga consultando GET /{mediaId}
            std::string host = "graph.facebook.com";
            std::string port = "443";
            std::string target = "/" + cleanApiVersion + "/" + mediaId;

            boost::asio::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();
            ctx.set_verify_mode(ssl::verify_peer);

            tcp::resolver resolver(ioc);
            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                return "";
            }

            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            auto const results = resolver.resolve(tcp::v4(), host, port);
            beast::get_lowest_layer(stream).connect(results);
            beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));
            stream.handshake(ssl::stream_base::client);

            http::request<http::string_body> req{http::verb::get, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::authorization, "Bearer " + cleanToken);
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(stream, buffer, res);

            std::string metaResp = beast::buffers_to_string(res.body().data());
            beast::error_code ec;
            stream.shutdown(ec);

            std::string downloadUrl = "";
            auto parsed = json::parse(metaResp);
            if (parsed.is_object() && parsed.as_object().contains("url"))
            {
                downloadUrl = std::string(parsed.as_object().at("url").as_string());
            }

            if (downloadUrl.empty())
            {
                omnisphere::utils::Logger::LogError("WhatsAppService", "DownloadMedia: Meta no devolvió downloadUrl. Respuesta: " + metaResp);
                return "";
            }

            // 2. Parsear host y target de downloadUrl (ej: https://lookaside.fbsbx.com/whatsapp_business/attachments/...)
            std::string cdnHost = "";
            std::string cdnTarget = "";
            std::string prefix = "https://";
            size_t protoPos = downloadUrl.find(prefix);
            if (protoPos != std::string::npos)
            {
                size_t hostStart = protoPos + prefix.length();
                size_t slashPos = downloadUrl.find('/', hostStart);
                if (slashPos != std::string::npos)
                {
                    cdnHost = downloadUrl.substr(hostStart, slashPos - hostStart);
                    cdnTarget = downloadUrl.substr(slashPos);
                }
            }

            if (cdnHost.empty() || cdnTarget.empty())
            {
                omnisphere::utils::Logger::LogError("WhatsAppService", "DownloadMedia: URL de descarga inválida: " + downloadUrl);
                return "";
            }

            // 3. Descargar el binario del archivo
            boost::asio::io_context cdnIoc;
            ssl::context cdnCtx(ssl::context::tlsv12_client);
            cdnCtx.set_default_verify_paths();
            cdnCtx.set_verify_mode(ssl::verify_peer);

            tcp::resolver cdnResolver(cdnIoc);
            beast::ssl_stream<beast::tcp_stream> cdnStream(cdnIoc, cdnCtx);

            if (!SSL_set_tlsext_host_name(cdnStream.native_handle(), cdnHost.c_str()))
            {
                return "";
            }

            beast::get_lowest_layer(cdnStream).expires_after(std::chrono::seconds(30));
            auto const cdnResults = cdnResolver.resolve(tcp::v4(), cdnHost, "443");
            beast::get_lowest_layer(cdnStream).connect(cdnResults);
            beast::get_lowest_layer(cdnStream).expires_after(std::chrono::seconds(30));
            cdnStream.handshake(ssl::stream_base::client);

            http::request<http::empty_body> cdnReq{http::verb::get, cdnTarget, 11};
            cdnReq.set(http::field::host, cdnHost);
            cdnReq.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            cdnReq.set(http::field::authorization, "Bearer " + cleanToken);

            http::write(cdnStream, cdnReq);

            beast::flat_buffer cdnBuffer;
            http::response<http::string_body> cdnRes;
            http::read(cdnStream, cdnBuffer, cdnRes);

            beast::error_code cdnEc;
            cdnStream.shutdown(cdnEc);

            if (cdnRes.result() != http::status::ok)
            {
                omnisphere::utils::Logger::LogError("WhatsAppService", "DownloadMedia: Falló la descarga binaria (HTTP " + std::to_string(cdnRes.result_int()) + ")");
                return "";
            }

            // 4. Determinar extensión adecuada
            std::string ext = ".bin";
            if (!suggestedFilename.empty() && suggestedFilename.find('.') != std::string::npos)
            {
                ext = suggestedFilename.substr(suggestedFilename.rfind('.'));
            }
            else if (mimeType.find("jpeg") != std::string::npos || mimeType.find("jpg") != std::string::npos) ext = ".jpg";
            else if (mimeType.find("png") != std::string::npos) ext = ".png";
            else if (mimeType.find("pdf") != std::string::npos) ext = ".pdf";
            else if (mimeType.find("audio") != std::string::npos || mimeType.find("ogg") != std::string::npos) ext = ".ogg";
            else if (mimeType.find("mp4") != std::string::npos || mimeType.find("video") != std::string::npos) ext = ".mp4";
            else if (mimeType.find("webp") != std::string::npos) ext = ".webp";

            // 5. Guardar en directorio ./uploads/whatsapp/
            std::string uploadDir = "./uploads/whatsapp";
            std::error_code fsEc;
            fs::create_directories(uploadDir, fsEc);

            std::string outFilename = "wa_" + mediaId + "_" + std::to_string(std::time(nullptr)) + ext;
            std::string diskPath = uploadDir + "/" + outFilename;

            std::ofstream outFile(diskPath, std::ios::binary);
            if (!outFile.is_open())
            {
                omnisphere::utils::Logger::LogError("WhatsAppService", "DownloadMedia: No se pudo abrir archivo para escritura: " + diskPath);
                return "";
            }

            outFile.write(cdnRes.body().data(), cdnRes.body().size());
            outFile.close();

            std::string relativeUrl = "/uploads/whatsapp/" + outFilename;
            omnisphere::utils::Logger::LogInfo("WhatsAppService", "DownloadMedia: Archivo guardado con éxito -> " + relativeUrl);
            return relativeUrl;
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("WhatsAppService", "DownloadMedia Exception: " + std::string(ex.what()));
            return "";
        }
    }
} // namespace omnisphere::services

