#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct CreatePaymentInput
    {
        std::optional<std::string> Code;
        std::string PaymentCode;
        std::string Type = "CASH"; // CASH, TRANSFER, CARD
        std::string EntityType = "ROUTE_RESERVATION";
        std::string EntityCode;
        double Amount = 0.0;
        std::string Currency = "mxn";
        std::string Status = "PENDING"; // PENDING, PAID, REFUNDED, CANCELLED

        // Transfer fields
        std::optional<std::string> BankName;
        std::optional<std::string> Clabe;
        std::optional<std::string> AccountHolder;
        std::optional<std::string> PaymentReference;

        // Cash fields
        std::optional<double> CashReceived;
        std::optional<double> CashChange;
        std::optional<std::string> ReceivedBy;
        std::optional<std::string> Location;
        std::optional<std::string> ReceiptNumber;
        std::optional<std::string> Notes;

        // Card fields
        std::optional<std::string> Provider;
        std::optional<std::string> PaymentIntentId;
        std::optional<std::string> SessionId;
        std::optional<std::string> CardLast4;
        std::optional<std::string> CardBrand;
        std::optional<std::string> CardHolderName;
        std::optional<std::string> AuthorizationCode;
        std::optional<std::string> ReceiptUrl;
        std::optional<std::string> HostedUrl;
        std::optional<std::string> RawPayload;

        // Common audit fields
        std::optional<std::string> ExpiresAt;
        bool IsActive = true;
        std::string CreatedBy = "system";
        std::string CreateDate;
    };

    BOOST_DESCRIBE_STRUCT(CreatePaymentInput, (), (
        Code, PaymentCode, Type, EntityType, EntityCode,
        Amount, Currency, Status,
        BankName, Clabe, AccountHolder, PaymentReference,
        CashReceived, CashChange, ReceivedBy, Location, ReceiptNumber, Notes,
        Provider, PaymentIntentId, SessionId, CardLast4, CardBrand, CardHolderName, AuthorizationCode, ReceiptUrl, HostedUrl, RawPayload,
        ExpiresAt, IsActive, CreatedBy, CreateDate
    ))
} // namespace omnisphere::dtos
