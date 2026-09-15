#pragma once
#include <string>
#include <optional>
#include <vector>

namespace omnisphere::dtos
{
    struct CreateMetaTemplateButtonInput
    {
        std::string type = "QUICK_REPLY"; // "QUICK_REPLY", "URL", "PHONE_NUMBER"
        std::string text;
        std::optional<std::string> url;
        std::optional<std::string> phoneNumber;
    };

    struct CreateMetaTemplateInput
    {
        std::string name;                 // e.g., "TPL_TRANSFER_REJECTED" or "tpl_transfer_rejected"
        std::string title;                // e.g., "Transferencia Rechazada"
        std::string category = "UTILITY"; // "UTILITY", "MARKETING", "AUTHENTICATION"
        std::string language = "es_MX";
        std::string bodyText;             // e.g., "Hola {{1}}, tu reservación {{2}} ha sido cancelada."
        std::string headerType = "NONE";   // "NONE", "TEXT"
        std::optional<std::string> headerText;
        std::optional<std::string> footerText;
        std::vector<CreateMetaTemplateButtonInput> buttons;
    };

    struct CreateMetaTemplateResult
    {
        bool success = false;
        std::string messageCode;
        std::string metaTemplateId;
        std::string metaStatus;
        std::string metaCategory;
        std::string errorMessage;
    };
}
