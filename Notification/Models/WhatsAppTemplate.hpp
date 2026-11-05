#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>
#include <vector>

namespace omnisphere::models
{
    struct WhatsAppTemplateHeaderVariable
    {
        int entry = 0;
        std::string templateCode;
        std::string varName;
        int varIndex = 1;
        std::optional<std::string> exampleValue;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateHeaderVariable, (), (
        entry,
        templateCode,
        varName,
        varIndex,
        exampleValue,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateHeader
    {
        int entry = 0;
        std::string templateCode;
        std::string headerType = "NONE"; // NONE, TEXT, IMAGE, DOCUMENT, VIDEO
        std::optional<std::string> textContent;
        std::optional<std::string> mediaUrl;
        std::string createdBy = "system";
        std::optional<std::string> createDate;

        std::vector<WhatsAppTemplateHeaderVariable> variables;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateHeader, (), (
        entry,
        templateCode,
        headerType,
        textContent,
        mediaUrl,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateBodyVariable
    {
        int entry = 0;
        std::string templateCode;
        std::string varName;
        int varIndex = 1;
        std::optional<std::string> exampleValue;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateBodyVariable, (), (
        entry,
        templateCode,
        varName,
        varIndex,
        exampleValue,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateBody
    {
        int entry = 0;
        std::string templateCode;
        std::string bodyText;
        std::string createdBy = "system";
        std::optional<std::string> createDate;

        std::vector<WhatsAppTemplateBodyVariable> variables;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateBody, (), (
        entry,
        templateCode,
        bodyText,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateFooterVariable
    {
        int entry = 0;
        std::string templateCode;
        std::string varName;
        int varIndex = 1;
        std::optional<std::string> exampleValue;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateFooterVariable, (), (
        entry,
        templateCode,
        varName,
        varIndex,
        exampleValue,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateFooter
    {
        int entry = 0;
        std::string templateCode;
        std::optional<std::string> footerText;
        std::string createdBy = "system";
        std::optional<std::string> createDate;

        std::vector<WhatsAppTemplateFooterVariable> variables;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateFooter, (), (
        entry,
        templateCode,
        footerText,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateButton
    {
        int entry = 0;
        std::string templateCode;
        std::string buttonType = "QUICK_REPLY"; // QUICK_REPLY, URL, PHONE_NUMBER
        std::string title;
        std::optional<std::string> payload;
        int sortOrder = 1;
        std::string createdBy = "system";
        std::optional<std::string> createDate;

        // Propiedades hidratadas en tiempo de ejecución desde WhatsAppInternalButtonActions (NO son columnas de WhatsAppTemplateButtons)
        std::string triggerAction = "NONE";
        std::string responseMessage;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateButton, (), (
        entry,
        templateCode,
        buttonType,
        title,
        payload,
        sortOrder,
        createdBy,
        createDate
    ))

    struct WhatsAppInternalTemplate
    {
        int entry = 0;
        std::string templateCode;
        std::string receiptReceivedType = "TEXT";
        std::string receiptReceivedPayload;
        std::string invalidFileType = "TEXT";
        std::string invalidFilePayload;
        std::string receiptAcceptedType = "TEXT";
        std::string receiptAcceptedPayload;
        std::string receiptRejectedType = "TEXT";
        std::string receiptRejectedPayload;
        std::string receiptReceivedMessage;
        std::string invalidFileMessage;
        std::string receiptAcceptedMessage;
        std::string receiptRejectedMessage;
        std::string confirmationTemplateCode;
        bool isActive = true;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppInternalTemplate, (), (
        entry,
        templateCode,
        receiptReceivedType,
        receiptReceivedPayload,
        invalidFileType,
        invalidFilePayload,
        receiptAcceptedType,
        receiptAcceptedPayload,
        receiptRejectedType,
        receiptRejectedPayload,
        receiptReceivedMessage,
        invalidFileMessage,
        receiptAcceptedMessage,
        receiptRejectedMessage,
        confirmationTemplateCode,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))

    struct WhatsAppInternalButtonAction
    {
        int entry = 0;
        std::string templateCode;
        std::string buttonTitle;
        int sortOrder = 1;
        std::string messageType = "NONE";
        std::string responsePayload;
        bool requestReceipt = false;
        bool cancelReservation = false;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppInternalButtonAction, (), (
        entry,
        templateCode,
        buttonTitle,
        sortOrder,
        messageType,
        responsePayload,
        requestReceipt,
        cancelReservation,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))

    struct WhatsAppTemplate
    {
        int entry = 0;
        std::string code;
        std::string templateName;
        std::string language = "es_MX";
        std::string category = "UTILITY";
        std::optional<std::string> metaTemplateId;
        std::string status = "PENDING";
        std::optional<std::string> rejectReason;
        bool isActive = true;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;

        // Propiedades hidratadas en tiempo de ejecución desde WhatsAppInternalTemplates (NO son columnas de WhatsAppTemplates)
        std::string receiptReceivedType = "TEXT";
        std::string receiptReceivedPayload;
        std::string invalidFileType = "TEXT";
        std::string invalidFilePayload;
        std::string receiptAcceptedType = "TEXT";
        std::string receiptAcceptedPayload;
        std::string receiptRejectedType = "TEXT";
        std::string receiptRejectedPayload;
        std::string receiptReceivedMessage;
        std::string invalidFileMessage;
        std::string receiptAcceptedMessage;
        std::string receiptRejectedMessage;
        std::string confirmationTemplateCode;

        // Structured Sub-Components
        std::optional<WhatsAppTemplateHeader> header;
        std::optional<WhatsAppTemplateBody> body;
        std::optional<WhatsAppTemplateFooter> footer;
        std::vector<WhatsAppTemplateButton> buttons;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplate, (), (
        entry,
        code,
        templateName,
        language,
        category,
        metaTemplateId,
        status,
        rejectReason,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
