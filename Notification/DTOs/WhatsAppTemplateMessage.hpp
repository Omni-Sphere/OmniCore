#pragma once
#include <string>
#include <vector>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct WhatsAppTemplateParameter
    {
        std::string type = "text"; // "text", "currency", "date_time", etc.
        std::string parameterName; // Nombre del parámetro de Meta (ej. "customer_name")
        std::string text;          // Valor interpolado
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateParameter, (), (
        type, parameterName, text
    ))

    struct WhatsAppTemplateHeaderPayload
    {
        std::string type; // "TEXT", "IMAGE", "VIDEO", "DOCUMENT", "LOCATION"
        std::vector<WhatsAppTemplateParameter> parameters; // Para TEXT
        std::optional<std::string> mediaUrl;               // Para IMAGE, VIDEO, DOCUMENT
        std::optional<std::string> filename;               // Para DOCUMENT
        double latitude = 0.0;                             // Para LOCATION
        double longitude = 0.0;
        std::string locationName;
        std::string locationAddress;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateHeaderPayload, (), (
        type, parameters, mediaUrl, filename, latitude, longitude, locationName, locationAddress
    ))

    struct WhatsAppTemplateButtonPayload
    {
        std::string subType; // "quick_reply", "url"
        int index = 0;
        std::string parameterValue; // Payload para quick_reply o suffix para url
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateButtonPayload, (), (
        subType, index, parameterValue
    ))

    struct WhatsAppTemplateMessage
    {
        std::string phoneNumber;
        std::string templateName;        // Nombre oficial de la plantilla en Meta (ej. "ticket_confirmation")
        std::string language = "es_MX";   // Idioma registrado en Meta (ej. "es_MX", "en", "en_US")
        
        std::optional<WhatsAppTemplateHeaderPayload> header;
        std::vector<WhatsAppTemplateParameter> bodyParameters; // Parámetros exactos del cuerpo
        std::vector<WhatsAppTemplateButtonPayload> buttons;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppTemplateMessage, (), (
        phoneNumber, templateName, language, header, bodyParameters, buttons
    ))
} // namespace omnisphere::dtos
