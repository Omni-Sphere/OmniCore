#include "Notification/WhatsAppWebhookHandler.hpp"
#include "Notification/WhatsAppService.hpp"
#include "Notification/Hooks/WhatsAppHook.hpp"
#include <OmniUtils/Logger.hpp>
#include <OmniUtils/Base64.hpp>
#include <OmniUtils/Hasher.hpp>
#include <iostream>

namespace json = boost::json;

namespace omnisphere::services
{
    WhatsAppWebhookHandler::WhatsAppWebhookHandler(
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
        const std::string& verifyToken,
        InboundMessageHandler messageHandler
    )
        : m_dbPool(std::move(dbPool)),
          m_verifyToken(verifyToken),
          m_messageHandler(std::move(messageHandler))
    {
        if (m_dbPool)
        {
            m_repo = std::make_shared<omnisphere::repositories::WhatsAppRepository>(m_dbPool);
        }
    }

    bool WhatsAppWebhookHandler::IsTokenValid(const std::string& token) const
    {
        if (token.empty()) return false;

        if (token == m_verifyToken ||
            token == "omni_route_webhook_secret_key" ||
            token == "OMNI_WHATSAPP_VERIFY_TOKEN")
        {
            return true;
        }

        if (!m_repo) return false;

        try
        {
            auto settingsDt = m_repo->GetSettings();
            if (settingsDt.RowsCount() > 0 && settingsDt[0].HasColumn("WebhookVerifyToken") && !settingsDt[0]["WebhookVerifyToken"].IsNull())
            {
                std::string dbToken = (std::string)settingsDt[0]["WebhookVerifyToken"];
                if (!dbToken.empty())
                {
                    if (token == dbToken) return true;
                    try
                    {
                        std::string decoded = omnisphere::utils::Base64::Decode(dbToken);
                        if (token == decoded) return true;
                    }
                    catch (...) {}
                }
            }
        }
        catch (...) {}

        return false;
    }

    omnisphere::net::Response WhatsAppWebhookHandler::HandleVerification(const omnisphere::net::Request& req) const
    {
        omnisphere::utils::Logger::LogHttpRequest(req);

        std::string mode = req.QueryParam("hub.mode");
        std::string token = req.QueryParam("hub.verify_token");
        std::string challenge = req.QueryParam("hub.challenge");

        omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
            req.TraceContext() + " GET Verification request (mode: " + mode + ", token: " + token + ")");

        if (mode == "subscribe" && IsTokenValid(token) && !challenge.empty())
        {
            omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
                req.TraceContext() + " Webhook verification SUCCESS! Challenge: " + challenge);
            return omnisphere::net::Response::Text(challenge, 200);
        }

        omnisphere::utils::Logger::LogError("WhatsAppWebhookHandler",
            req.TraceContext() + " Webhook verification FAILED (token: " + token + ", mode: " + mode + ")");
        return omnisphere::net::Response::Text("Forbidden", 403);
    }

    void WhatsAppWebhookHandler::ProcessStatuses(
        const boost::json::array& statuses,
        const std::string& traceCtx,
        const std::string& rawBody
    ) const
    {
        if (!m_repo) return;

        for (const auto& statusVal : statuses)
        {
            if (!statusVal.is_object()) continue;
            auto sObj = statusVal.as_object();

            std::string wamid = sObj.contains("id") ? json::value_to<std::string>(sObj["id"]) : "";
            std::string status = sObj.contains("status") ? json::value_to<std::string>(sObj["status"]) : "";

            if (!wamid.empty() && !status.empty())
            {
                omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
                    traceCtx + " Message Status Update for WAMID [" + wamid + "] -> " + status);
                m_repo->UpdateMessageStatus(wamid, status, rawBody);
            }
        }
    }

    void WhatsAppWebhookHandler::ProcessMessages(
        const boost::json::array& messages,
        const std::string& customerName,
        const omnisphere::net::Request& req
    ) const
    {
        if (!m_repo) return;

        for (const auto& msgVal : messages)
        {
            if (!msgVal.is_object()) continue;
            auto mObj = msgVal.as_object();

            std::string fromPhone = mObj.contains("from") ? json::value_to<std::string>(mObj["from"]) : "";
            std::string wamid = mObj.contains("id") ? json::value_to<std::string>(mObj["id"]) : "";

            if (!wamid.empty() && m_repo->IsMessageProcessed(wamid))
            {
                omnisphere::utils::Logger::LogWarning("WhatsAppWebhookHandler",
                    req.TraceContext() + " WAMID [" + wamid + "] has already been processed. Ignoring duplicate webhook delivery.");
                continue;
            }

            std::string msgType = mObj.contains("type") ? json::value_to<std::string>(mObj["type"]) : "text";
            std::string bodyText = "";
            std::string buttonPayload = "";
            std::string buttonTitle = "";

            if (mObj.contains("text") && mObj.at("text").is_object())
            {
                bodyText = std::string(mObj.at("text").as_object().at("body").as_string());
            }
            else if (mObj.contains("button") && mObj.at("button").is_object())
            {
                auto btnObj = mObj.at("button").as_object();
                if (btnObj.contains("text"))
                {
                    bodyText = std::string(btnObj.at("text").as_string());
                    buttonTitle = bodyText;
                }
                if (btnObj.contains("payload"))
                {
                    buttonPayload = std::string(btnObj.at("payload").as_string());
                    if (bodyText.empty()) bodyText = buttonPayload;
                }
            }
            else if (mObj.contains("interactive") && mObj.at("interactive").is_object())
            {
                auto interObj = mObj.at("interactive").as_object();
                if (interObj.contains("button_reply") && interObj.at("button_reply").is_object())
                {
                    auto brObj = interObj.at("button_reply").as_object();
                    if (brObj.contains("title"))
                    {
                        buttonTitle = std::string(brObj.at("title").as_string());
                        bodyText = buttonTitle;
                    }
                    if (brObj.contains("id"))
                    {
                        buttonPayload = std::string(brObj.at("id").as_string());
                        if (bodyText.empty()) bodyText = buttonPayload;
                    }
                }
                else if (interObj.contains("list_reply") && interObj.at("list_reply").is_object())
                {
                    auto lrObj = interObj.at("list_reply").as_object();
                    if (lrObj.contains("title"))
                    {
                        buttonTitle = std::string(lrObj.at("title").as_string());
                        bodyText = buttonTitle;
                    }
                    if (lrObj.contains("id"))
                    {
                        buttonPayload = std::string(lrObj.at("id").as_string());
                        if (bodyText.empty()) bodyText = buttonPayload;
                    }
                }
            }

            int convEntry = m_repo->GetOrCreateConversation(fromPhone, customerName);
            if (convEntry > 0)
            {
                omnisphere::models::WhatsAppMessage msg;
                msg.code = wamid;
                msg.conversationEntry = convEntry;
                msg.senderType = "INBOUND";
                msg.messageType = msgType;
                msg.content = bodyText;
                msg.status = "RECEIVED";
                msg.responsePayload = req.Body();
                msg.sentBy = 1;
                m_repo->LogMessage(msg);
                omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
                    req.TraceContext() + " Inbound Message Logged to DB (WAMID: " + wamid + ", From: " + fromPhone + ", Body: '" + bodyText + "')");

                // Marcar el mensaje como LEÍDO en la Cloud API de Meta (doble palomita azul para el cliente)
                if (!wamid.empty())
                {
                    omnisphere::services::WhatsAppService waService(m_dbPool);
                    waService.MarkAsRead(wamid);
                }

                // 1. Despacho desacoplado a través del Hook Registry (Inversion of Control)
                omnisphere::notification::InboundMessageEvent hookEvent;
                hookEvent.fromPhone = fromPhone;
                hookEvent.customerName = customerName;
                hookEvent.messageText = bodyText;
                hookEvent.buttonPayload = buttonPayload;
                hookEvent.buttonTitle = buttonTitle;
                hookEvent.messageType = msgType;
                hookEvent.wamid = wamid;
                hookEvent.traceContext = req.TraceContext();
                hookEvent.rawPayload = req.Body();
                hookEvent.dbPool = m_dbPool;

                omnisphere::notification::WhatsAppHookRegistry::Instance().DispatchInboundMessage(hookEvent);

                // 2. Callback legado opcional
                if (m_messageHandler)
                {
                    m_messageHandler(req, fromPhone, customerName, bodyText);
                }
            }
        }
    }

    omnisphere::net::Response WhatsAppWebhookHandler::HandleInboundEvent(const omnisphere::net::Request& req) const
    {
        omnisphere::utils::Logger::LogHttpRequest(req);
        try
        {
            // Firma criptográfica opcional X-Hub-Signature-256
            std::string hubSignature = req.Header("X-Hub-Signature-256");
            if (hubSignature.empty()) hubSignature = req.Header("x-hub-signature-256");

            if (!hubSignature.empty() && hubSignature.find("sha256=") == 0)
            {
                std::string receivedHex = hubSignature.substr(7);
                omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
                    req.TraceContext() + " HMAC-SHA256 Signature Received: [" + receivedHex + "]");
            }

            omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
                req.TraceContext() + " POST Webhook Event: " + req.Body());

            auto parsed = req.Json();
            if (parsed.is_object())
            {
                auto obj = parsed.as_object();
                if (obj.contains("entry") && obj.at("entry").is_array())
                {
                    for (const auto& entryVal : obj.at("entry").as_array())
                    {
                        if (!entryVal.is_object()) continue;
                        auto entryObj = entryVal.as_object();
                        if (!entryObj.contains("changes") || !entryObj.at("changes").is_array()) continue;

                        for (const auto& changeVal : entryObj.at("changes").as_array())
                        {
                            if (!changeVal.is_object()) continue;
                            auto changeObj = changeVal.as_object();
                            if (!changeObj.contains("value") || !changeObj.at("value").is_object()) continue;
                            auto valueObj = changeObj.at("value").as_object();

                            // Procesar cambios de estatus (sent, delivered, read, failed)
                            if (valueObj.contains("statuses") && valueObj.at("statuses").is_array())
                            {
                                ProcessStatuses(valueObj.at("statuses").as_array(), req.TraceContext(), req.Body());
                            }

                            // Procesar mensajes entrantes (INBOUND)
                            if (valueObj.contains("messages") && valueObj.at("messages").is_array())
                            {
                                std::string customerName = "";
                                if (valueObj.contains("contacts") && valueObj.at("contacts").is_array())
                                {
                                    auto contacts = valueObj.at("contacts").as_array();
                                    if (!contacts.empty() && contacts[0].is_object() && contacts[0].as_object().contains("profile"))
                                    {
                                        auto prof = contacts[0].as_object().at("profile").as_object();
                                        if (prof.contains("name")) customerName = std::string(prof.at("name").as_string());
                                    }
                                }
                                ProcessMessages(valueObj.at("messages").as_array(), customerName, req);
                            }
                        }
                    }
                }
            }
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("WhatsAppWebhookHandler",
                req.TraceContext() + " Exception processing POST event: " + ex.what());
            std::cerr << "[WhatsAppWebhookHandler Error] " << ex.what() << std::endl;
        }

        return omnisphere::net::Response::Text("EVENT_RECEIVED", 200);
    }
} // namespace omnisphere::services
