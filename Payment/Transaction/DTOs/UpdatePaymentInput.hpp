#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct UpdatePaymentInput
    {
        std::string Code;
        std::optional<std::string> PaymentCode;
        std::optional<std::string> Type;
        std::optional<std::string> Status;

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
        std::optional<bool> IsActive;
        std::optional<std::string> LastUpdatedBy;
        std::optional<std::string> UpdateDate;
    };

    BOOST_DESCRIBE_STRUCT(UpdatePaymentInput, (), (
        Code, PaymentCode, Type, Status,
        BankName, Clabe, AccountHolder, PaymentReference,
        CashReceived, CashChange, ReceivedBy, Location, ReceiptNumber, Notes,
        Provider, PaymentIntentId, SessionId, CardLast4, CardBrand, CardHolderName, AuthorizationCode, ReceiptUrl, HostedUrl, RawPayload,
        ExpiresAt, IsActive, LastUpdatedBy, UpdateDate
    ))
} // namespace omnisphere::dtos
