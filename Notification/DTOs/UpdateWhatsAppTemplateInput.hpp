#pragma once
#include <string>
#include <optional>
#include <vector>
#include <boost/describe.hpp>
#include "CreateMetaTemplateInput.hpp"

namespace omnisphere::dtos
{
    struct UpdateWhatsAppTemplateInput
    {
        std::string Code;
        std::optional<std::string> TemplateName;
        std::optional<std::string> Category;
        std::optional<std::string> Language;
        std::optional<std::string> BodyText;
        std::optional<std::string> HeaderType;
        std::optional<std::string> HeaderText;
        std::optional<std::string> FooterText;
        std::optional<std::string> ReceiptReceivedMessage;
        std::optional<std::string> InvalidFileMessage;
        std::optional<std::string> ReceiptAcceptedMessage;
        std::optional<std::string> ReceiptRejectedMessage;
        std::optional<std::string> ConfirmationTemplateCode;
        std::optional<bool> IsActive;
        std::string LastUpdatedBy = "system";
        std::optional<std::string> UpdateDate;

        // Sub-componentes
        std::optional<std::vector<CreateMetaTemplateVariableInput>> HeaderVariables;
        std::optional<std::vector<CreateMetaTemplateVariableInput>> BodyVariables;
        std::optional<std::vector<CreateMetaTemplateVariableInput>> FooterVariables;
        std::optional<std::vector<CreateMetaTemplateButtonInput>> Buttons;
    };

    BOOST_DESCRIBE_STRUCT(UpdateWhatsAppTemplateInput, (), (
        Code,
        TemplateName,
        Category,
        Language,
        ReceiptReceivedMessage,
        InvalidFileMessage,
        ReceiptAcceptedMessage,
        ReceiptRejectedMessage,
        ConfirmationTemplateCode,
        IsActive,
        LastUpdatedBy,
        UpdateDate
    ))
}
