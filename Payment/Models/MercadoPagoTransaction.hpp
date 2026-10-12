#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct MercadoPagoTransaction
    {
        int entry = 0;
        std::string code;
        std::string externalReference; // Reservation / Order / Invoice code
        std::string paymentId;         // Mercado Pago payment ID (e.g. 123456789)
        std::optional<std::string> preferenceId;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string status = "pending"; // pending, approved, in_process, rejected, cancelled, refunded
        std::optional<std::string> statusDetail;
        std::optional<std::string> paymentMethodId; // visa, master, oxxo, spei, etc.
        std::optional<std::string> paymentTypeId;   // credit_card, debit_card, bank_transfer, ticket
        int installments = 1;
        std::optional<std::string> ticketUrl;       // External resource URL for OXXO/SPEI instructions
        std::optional<std::string> clabe;
        std::optional<std::string> cardBrand;
        std::optional<std::string> cardLast4;
        std::optional<std::string> clientIp;
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(MercadoPagoTransaction, (), (
        entry,
        code,
        externalReference,
        paymentId,
        preferenceId,
        amount,
        currency,
        status,
        statusDetail,
        paymentMethodId,
        paymentTypeId,
        installments,
        ticketUrl,
        clabe,
        cardBrand,
        cardLast4,
        clientIp,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
} // namespace omnisphere::models
