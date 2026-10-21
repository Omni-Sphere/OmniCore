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
        int headerEntry = 0;
        std::string varName;
        int varIndex = 1;
        std::optional<std::string> exampleValue;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateHeaderVariable, (), (
        entry,
        headerEntry,
        varName,
        varIndex,
        exampleValue,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateHeader
    {
        int entry = 0;
        int templateEntry = 0;
        std::string headerType = "NONE"; // NONE, TEXT, IMAGE, DOCUMENT, VIDEO
        std::optional<std::string> textContent;
        std::optional<std::string> mediaUrl;
        std::string createdBy = "system";
        std::optional<std::string> createDate;

        std::vector<WhatsAppTemplateHeaderVariable> variables;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateHeader, (), (
        entry,
        templateEntry,
        headerType,
        textContent,
        mediaUrl,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateBodyVariable
    {
        int entry = 0;
        int bodyEntry = 0;
        std::string varName;
        int varIndex = 1;
        std::optional<std::string> exampleValue;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateBodyVariable, (), (
        entry,
        bodyEntry,
        varName,
        varIndex,
        exampleValue,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateBody
    {
        int entry = 0;
        int templateEntry = 0;
        std::string bodyText;
        std::string createdBy = "system";
        std::optional<std::string> createDate;

        std::vector<WhatsAppTemplateBodyVariable> variables;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateBody, (), (
        entry,
        templateEntry,
        bodyText,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateFooterVariable
    {
        int entry = 0;
        int footerEntry = 0;
        std::string varName;
        int varIndex = 1;
        std::optional<std::string> exampleValue;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateFooterVariable, (), (
        entry,
        footerEntry,
        varName,
        varIndex,
        exampleValue,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateFooter
    {
        int entry = 0;
        int templateEntry = 0;
        std::optional<std::string> footerText;
        std::string createdBy = "system";
        std::optional<std::string> createDate;

        std::vector<WhatsAppTemplateFooterVariable> variables;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateFooter, (), (
        entry,
        templateEntry,
        footerText,
        createdBy,
        createDate
    ))

    struct WhatsAppTemplateButton
    {
        int entry = 0;
        int templateEntry = 0;
        std::string buttonType = "QUICK_REPLY"; // QUICK_REPLY, URL, PHONE_NUMBER
        std::string title;
        std::optional<std::string> payload;
        int sortOrder = 1;
        std::string createdBy = "system";
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateButton, (), (
        entry,
        templateEntry,
        buttonType,
        title,
        payload,
        sortOrder,
        createdBy,
        createDate
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
