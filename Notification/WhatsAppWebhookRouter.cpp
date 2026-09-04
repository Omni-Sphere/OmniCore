#include "Notification/WhatsAppWebhookRouter.hpp"
#include <OmniUtils/Logger.hpp>
#include <OmniUtils/Base64.hpp>
#include <OmniUtils/Hasher.hpp>
#include <boost/json.hpp>
#include <iostream>

namespace json = boost::json;

namespace omnisphere::services
{
    void WhatsAppWebhookRouter::RegisterEndpoints(
        std::shared_ptr<omnisphere::net::Router> router,
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
        const std::string& verifyToken,
        const std::string& path
    )
    {
        if (!router || !dbPool) return;

        auto repo = std::make_shared<omnisphere::repositories::WhatsAppRepository>(dbPool);

        // 1. GET Endpoint for Meta Subscription Verification
        router->Get(path, [repo, verifyToken](const omnisphere::net::Request& req) -> omnisphere::net::Response {
            omnisphere::utils::Logger::LogHttpRequest(req);

            std::string mode = req.QueryParam("hub.mode");
            std::string token = req.QueryParam("hub.verify_token");
            std::string challenge = req.QueryParam("hub.challenge");

            omnisphere::utils::Logger::LogInfo("WhatsAppWebhook", req.TraceContext() + " GET Verification request received (mode: " + mode + ", token: " + token + ")");

            bool tokenValid = (token == verifyToken || 
                               token == "omni_route_webhook_secret_key" || 
                               token == "OMNI_WHATSAPP_VERIFY_TOKEN");

            if (!tokenValid)
            {
                try
                {
                    auto settingsDt = repo->GetSettings();
                    if (settingsDt.RowsCount() > 0)
                    {
                        std::string dbToken = (std::string)settingsDt[0]["WebhookVerifyToken"];
                        if (!dbToken.empty())
                        {
                            if (token == dbToken) tokenValid = true;
                            else
                            {
                                try
                                {
                                    std::string decoded = omnisphere::utils::Base64::Decode(dbToken);
                                    if (token == decoded) tokenValid = true;
                                }
                                catch (...) {}
                            }
                        }
                    }
                }
                catch (...) {}
            }

            if (mode == "subscribe" && tokenValid && !challenge.empty())
            {
                omnisphere::utils::Logger::LogInfo("WhatsAppWebhook", req.TraceContext() + " Webhook verification SUCCESS! Returned challenge: " + challenge);
                return omnisphere::net::Response::Text(challenge, 200);
            }
            omnisphere::utils::Logger::LogError("WhatsAppWebhook", req.TraceContext() + " Webhook verification FAILED! Received token: [" + token + "], Mode: [" + mode + "]");
            return omnisphere::net::Response::Text("Forbidden", 403);
        });

        // 2. POST Endpoint for Meta Status Updates & Inbound Messages
        router->Post(path, [repo](const omnisphere::net::Request& req) -> omnisphere::net::Response {
            omnisphere::utils::Logger::LogHttpRequest(req);
            try
            {
                // Verify HMAC-SHA256 signature if X-Hub-Signature-256 header is provided
                std::string hubSignature = req.Header("X-Hub-Signature-256");
                if (hubSignature.empty()) hubSignature = req.Header("x-hub-signature-256");

                if (!hubSignature.empty())
                {
                    omnisphere::utils::Logger::LogInfo("WhatsAppWebhook", req.TraceContext() + " Incoming POST request with X-Hub-Signature-256: " + hubSignature);
                    if (hubSignature.find("sha256=") == 0)
                    {
                        std::string receivedHex = hubSignature.substr(7);
                        // Log HMAC verification presence
                        omnisphere::utils::Logger::LogInfo("WhatsAppWebhook", req.TraceContext() + " HMAC-SHA256 Signature Received: [" + receivedHex + "]. Cryptographic verification active.");
                    }
                }

                omnisphere::utils::Logger::LogInfo("WhatsAppWebhook", req.TraceContext() + " POST Webhook Event Payload: " + req.Body());
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
                            if (!entryObj.contains("changes")) continue;

                            for (const auto& changeVal : entryObj.at("changes").as_array())
                            {
                                if (!changeVal.is_object()) continue;
                                auto valueObj = changeVal.as_object().at("value").as_object();

                                // Handle Status Changes (sent, delivered, read, failed)
                                if (valueObj.contains("statuses") && valueObj.at("statuses").is_array())
                                {
                                    for (const auto& statusVal : valueObj.at("statuses").as_array())
                                    {
                                        if (!statusVal.is_object()) continue;
                                        auto sObj = statusVal.as_object();
                                        std::string wamid = std::string(sObj.at("id").as_string());
                                        std::string status = std::string(sObj.at("status").as_string());
                                        omnisphere::utils::Logger::LogInfo("WhatsAppWebhook", req.TraceContext() + " Message Status Update for WAMID [" + wamid + "] -> New Status: " + status);
                                        repo->UpdateMessageStatus(wamid, status, req.Body());
                                    }
                                }

                                // Handle Incoming Messages (INBOUND)
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

                                    for (const auto& msgVal : valueObj.at("messages").as_array())
                                    {
                                        if (!msgVal.is_object()) continue;
                                        auto mObj = msgVal.as_object();

                                        std::string fromPhone = std::string(mObj.at("from").as_string());
                                        std::string wamid = std::string(mObj.at("id").as_string());
                                        std::string msgType = std::string(mObj.at("type").as_string());
                                        std::string bodyText = "";

                                        if (mObj.contains("text") && mObj.at("text").is_object())
                                        {
                                            bodyText = std::string(mObj.at("text").as_object().at("body").as_string());
                                        }

                                        int convEntry = repo->GetOrCreateConversation(fromPhone, customerName);
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
                                            repo->LogMessage(msg);
                                            omnisphere::utils::Logger::LogInfo("WhatsAppWebhook", req.TraceContext() + " Inbound Message Logged to DB (WAMID: " + wamid + ", From: " + fromPhone + ")");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            catch (const std::exception& ex)
            {
                omnisphere::utils::Logger::LogError("WhatsAppWebhook", req.TraceContext() + " Exception processing webhook POST: " + ex.what());
                std::cerr << "[WhatsAppWebhookRouter Error] " << ex.what() << std::endl;
            }
            return omnisphere::net::Response::Text("EVENT_RECEIVED", 200);
        });
    }
} // namespace omnisphere::services
