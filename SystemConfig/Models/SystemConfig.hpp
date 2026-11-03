#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct SystemConfig
    {
        int entry = 1;
        std::string code = "DEFAULT";
        std::string feeHandlingStrategy = "SURCHARGE";
        double taxRatePercent = 16.0;
        std::string defaultCurrency = "MXN";
        std::string companyName = "OmniRoute Express";
        bool enableEmailNotifications = true;
        bool enableWhatsappNotifications = true;
        bool allowPartialPayments = false;
        std::string whatsappTransferConfirmMessage = "Por favor, *adjunta aquí tu comprobante de transferencia (únicamente imagen o archivo PDF)* para tu reservación *{folio}* para proceder con la validación de tus lugares. 📄✨";
        std::string whatsappMeetingPointMessage = "📍 *Punto de Abordaje para tu reservación ({folio}):*\n\n• *Lugar:* {pickup_name}\n• *Destino:* {destination_name}\n• *Hora de salida:* {departure_time}\n\nTe sugerimos llegar con al menos 15 minutos de anticipación. 🚌✨";
        std::string whatsappCancelTransferMessage = "Tu reservación *{folio}* ha sido cancelada con éxito y los lugares han sido liberados. ❌\n\nSi deseas viajar con nosotros en otra ocasión, con gusto te atenderemos.";
        std::string whatsappReceiptReceivedMessage = "¡Comprobante recibido con éxito para tu reservación *{folio}*! 📄✅\n\nNuestro equipo lo validará a la brevedad y te enviaremos la confirmación de tus accesos de abordaje.";
        std::string whatsappReceiptAcceptedMessage = "¡Hola {customer_name}! Tu comprobante de pago para la reservación *{folio}* ha sido validado y aceptado con éxito. ✅🚌\n\n• *Evento:* {event_name}\n• *Punto de encuentro:* {pickup_name}\n• *Hora de salida:* {departure_time}\n• *Lugares confirmados:* {seats}\n\n¡Gracias por viajar con nosotros!";
        std::string whatsappReceiptRejectedMessage = "Hola {customer_name}, lamentamos informarte que tu comprobante para la reservación *{folio}* no se ha podido validar y tu reservación ha sido cancelada. ❌\n\n*Motivo:* {reject_reason}\n\nSi consideras que es un error, por favor comunícate con nosotros.";
        std::string whatsappInvalidFileMessage = "⚠️ *Formato no válido*\n\nEl comprobante de pago únicamente debe ser una *imagen* (JPG, PNG) o un archivo *PDF*.\n\n¿Qué deseas hacer a continuación?";
        std::string whatsappConfirmationTemplateCode = "ticket_confirmation";
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(SystemConfig, (), (
        entry, code, feeHandlingStrategy, taxRatePercent,
        defaultCurrency, companyName, enableEmailNotifications,
        enableWhatsappNotifications, allowPartialPayments,
        whatsappTransferConfirmMessage, whatsappMeetingPointMessage,
        whatsappCancelTransferMessage, whatsappReceiptReceivedMessage,
        whatsappReceiptAcceptedMessage, whatsappReceiptRejectedMessage,
        whatsappInvalidFileMessage, whatsappConfirmationTemplateCode,
        isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
