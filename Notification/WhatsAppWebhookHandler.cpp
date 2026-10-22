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

        // Si se configuró un verifyToken explícito no vacío y no es el placeholder por defecto, verificarlo
        if (!m_verifyToken.empty() && 
            m_verifyToken != "OMNI_WHATSAPP_VERIFY_TOKEN" && 
            token == m_verifyToken)
        {
            return true;
        }

        if (!m_repo) return false;

        try
        {
            auto settingsDt = m_repo->GetSettings();
            if (!settingsDt.IsEmpty())
            {
                const auto& row = settingsDt[0];
                std::string dbToken = row["WebhookVerifyToken"].GetOptional<std::string>().value_or("");
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
            std::string mediaId = "";
            std::string mimeType = "";
            std::string filename = "";
            std::string mediaUrl = "";
            std::string contextWamid = "";
            std::string messageTimestamp = "";

            if (mObj.contains("timestamp"))
            {
                messageTimestamp = json::value_to<std::string>(mObj["timestamp"]);
            }

            if (mObj.contains("context") && mObj.at("context").is_object())
            {
                auto ctxObj = mObj.at("context").as_object();
                if (ctxObj.contains("id")) contextWamid = std::string(ctxObj.at("id").as_string());
            }

            omnisphere::services::WhatsAppService waService(m_dbPool);

            if (mObj.contains("text") && mObj.at("text").is_object())
            {
                bodyText = std::string(mObj.at("text").as_object().at("body").as_string());
            }
            else if (mObj.contains("image") && mObj.at("image").is_object())
            {
                auto imgObj = mObj.at("image").as_object();
                if (imgObj.contains("id")) mediaId = std::string(imgObj.at("id").as_string());
                if (imgObj.contains("mime_type")) mimeType = std::string(imgObj.at("mime_type").as_string());
                if (imgObj.contains("caption")) bodyText = std::string(imgObj.at("caption").as_string());
                if (bodyText.empty()) bodyText = "[Imagen]";
                if (!mediaId.empty())
                {
                    mediaUrl = waService.DownloadMedia(mediaId, mimeType, "comprobante.jpg");
                }
            }
            else if (mObj.contains("document") && mObj.at("document").is_object())
            {
                auto docObj = mObj.at("document").as_object();
                if (docObj.contains("id")) mediaId = std::string(docObj.at("id").as_string());
                if (docObj.contains("filename")) filename = std::string(docObj.at("filename").as_string());
                if (docObj.contains("mime_type")) mimeType = std::string(docObj.at("mime_type").as_string());
                if (docObj.contains("caption")) bodyText = std::string(docObj.at("caption").as_string());
                if (bodyText.empty()) bodyText = "[Documento: " + (!filename.empty() ? filename : "archivo.pdf") + "]";
                if (!mediaId.empty())
                {
                    mediaUrl = waService.DownloadMedia(mediaId, mimeType, filename);
                }
            }
            else if (mObj.contains("audio") && mObj.at("audio").is_object())
            {
                auto audObj = mObj.at("audio").as_object();
                if (audObj.contains("id")) mediaId = std::string(audObj.at("id").as_string());
                if (audObj.contains("mime_type")) mimeType = std::string(audObj.at("mime_type").as_string());
                bodyText = "[Audio / Nota de voz]";
                if (!mediaId.empty())
                {
                    mediaUrl = waService.DownloadMedia(mediaId, mimeType, "audio.ogg");
                }
            }
            else if (mObj.contains("video") && mObj.at("video").is_object())
            {
                auto vidObj = mObj.at("video").as_object();
                if (vidObj.contains("id")) mediaId = std::string(vidObj.at("id").as_string());
                if (vidObj.contains("mime_type")) mimeType = std::string(vidObj.at("mime_type").as_string());
                if (vidObj.contains("caption")) bodyText = std::string(vidObj.at("caption").as_string());
                if (bodyText.empty()) bodyText = "[Video]";
                if (!mediaId.empty())
                {
                    mediaUrl = waService.DownloadMedia(mediaId, mimeType, "video.mp4");
                }
            }
            else if (mObj.contains("sticker") && mObj.at("sticker").is_object())
            {
                auto stkObj = mObj.at("sticker").as_object();
                if (stkObj.contains("id")) mediaId = std::string(stkObj.at("id").as_string());
                if (stkObj.contains("mime_type")) mimeType = std::string(stkObj.at("mime_type").as_string());
                bodyText = "[Sticker]";
                if (!mediaId.empty())
                {
                    mediaUrl = waService.DownloadMedia(mediaId, mimeType, "sticker.webp");
                }
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
                std::string rawBody = req.Body();
                omnisphere::models::WhatsAppMessage msg;
                msg.whatsAppId = wamid;
                msg.conversationEntry = convEntry;
                msg.senderType = "INBOUND";
                msg.messageType = msgType;
                msg.content = bodyText;
                if (!mediaUrl.empty()) msg.mediaUrl = mediaUrl;
                if (!mediaId.empty()) msg.mediaId = mediaId;
                if (!mimeType.empty()) msg.mediaMimeType = mimeType;
                if (!filename.empty()) msg.mediaFileName = filename;
                if (!contextWamid.empty()) msg.contextWhatsAppId = contextWamid;
                if (!messageTimestamp.empty())
                {
                    try
                    {
                        time_t tSec = std::stoll(messageTimestamp);
                        std::tm tmBuf{};
                        gmtime_r(&tSec, &tmBuf);
                        char buf[32];
                        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmBuf);
                        msg.messageTimestamp = std::string(buf);
                    }
                    catch (...)
                    {
                        msg.messageTimestamp = messageTimestamp;
                    }
                }
                msg.status = "RECEIVED";
                msg.responsePayload = rawBody;
                msg.sentBy = 0;
                m_repo->LogMessage(msg);
                omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
                    req.TraceContext() + " Inbound Message Logged to DB (WAMID: " + wamid + ", From: " + fromPhone + ", Type: " + msgType + ", MediaUrl: '" + mediaUrl + "', Body: '" + bodyText + "')");

                // Marcar el mensaje como LEÍDO en la Cloud API de Meta (doble palomita azul para el cliente)
                if (!wamid.empty())
                {
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
                hookEvent.mediaUrl = mediaUrl;
                hookEvent.mediaId = mediaId;
                hookEvent.mimeType = mimeType;
                hookEvent.filename = filename;
                hookEvent.contextWamid = contextWamid;
                hookEvent.timestamp = messageTimestamp;
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

        auto lic = omnisphere::services::LicenseService::GetSharedInstance();
        if (lic && !lic->IsModuleLicensed(omnisphere::license::MODULE_WHATSAPP))
        {
            omnisphere::utils::Logger::LogWarning("WhatsAppWebhookHandler",
                req.TraceContext() + " Inbound event ignored: WhatsApp integration is disabled by license.");
            return omnisphere::net::Response::Json(boost::json::object{
                {"status", "ignored"},
                {"reason", "WhatsApp integration is disabled by license"}
            }, 403);
        }

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
            req.TraceContext() + " Enqueueing Meta POST Webhook Event to [MetaWorkerThread]...");

        // Despacho asíncrono al hilo dedicado de Meta
        m_metaWorker.Enqueue([this, req]() {
            ProcessEventAsync(req);
        });

        // Respuesta HTTP ultra-rápida (< 5ms) para Meta Cloud API
        return omnisphere::net::Response::Text("EVENT_RECEIVED", 200);
    }

    void WhatsAppWebhookHandler::ProcessEventAsync(omnisphere::net::Request req) const
    {
        try
        {
            omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
                req.TraceContext() + " [MetaWorkerThread] Processing POST Webhook Event: " + req.Body());

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

                            // Procesar actualizaciones de estado de plantillas (message_template_status_update)
                            ProcessTemplateStatusUpdate(changeObj, valueObj, req.TraceContext());
                        }
                    }
                }
            }
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("WhatsAppWebhookHandler",
                req.TraceContext() + " [MetaWorkerThread] Exception processing POST event: " + ex.what());
            std::cerr << "[WhatsAppWebhookHandler Error - MetaWorkerThread] " << ex.what() << std::endl;
        }
    }

    void WhatsAppWebhookHandler::ProcessTemplateStatusUpdate(
        const boost::json::object& changeObj,
        const boost::json::object& valueObj,
        const std::string& traceCtx
    ) const
    {
        if (!m_repo) return;

        std::string field = changeObj.contains("field") && changeObj.at("field").is_string()
            ? std::string(changeObj.at("field").as_string()) : "";

        bool isTemplateUpdate = (field == "message_template_status_update") ||
            valueObj.contains("message_template_id") ||
            valueObj.contains("message_template_name") ||
            (valueObj.contains("event") && !valueObj.contains("statuses") && !valueObj.contains("messages"));

        if (!isTemplateUpdate) return;

        std::string templateId = valueObj.contains("message_template_id") && valueObj.at("message_template_id").is_string()
            ? std::string(valueObj.at("message_template_id").as_string()) : "";

        std::string templateName = valueObj.contains("message_template_name") && valueObj.at("message_template_name").is_string()
            ? std::string(valueObj.at("message_template_name").as_string()) : "";

        std::string eventStatus = "";
        if (valueObj.contains("event") && valueObj.at("event").is_string())
            eventStatus = std::string(valueObj.at("event").as_string());
        else if (valueObj.contains("status") && valueObj.at("status").is_string())
            eventStatus = std::string(valueObj.at("status").as_string());

        std::string reason = "";
        if (valueObj.contains("reason") && valueObj.at("reason").is_string())
            reason = std::string(valueObj.at("reason").as_string());
        else if (valueObj.contains("disable_reason") && valueObj.at("disable_reason").is_string())
            reason = std::string(valueObj.at("disable_reason").as_string());

        if (!templateId.empty() || !templateName.empty())
        {
            omnisphere::utils::Logger::LogInfo("WhatsAppWebhookHandler",
                traceCtx + " Template Status Update received from Meta -> Name: [" + templateName + "], ID: [" + templateId + "], Event: [" + eventStatus + "], Reason: [" + reason + "]");
            m_repo->UpdateMetaTemplateStatus(templateId, templateName, eventStatus, reason);
        }
    }
} // namespace omnisphere::services
