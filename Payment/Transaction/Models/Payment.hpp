#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct Payment
    {
        int entry = 0;
        std::string code;
        std::string paymentCode;
        std::string type = "CASH"; // CASH, TRANSFER, CARD
        std::string entityType = "ROUTE_RESERVATION";
        std::string entityCode;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string status = "PENDING"; // PENDING, PAID, REFUNDED, CANCELLED

        // Transfer fields
        std::optional<std::string> bankName;
        std::optional<std::string> clabe;
        std::optional<std::string> accountHolder;
        std::optional<std::string> paymentReference;

        // Cash fields
        std::optional<double> cashReceived;
        std::optional<double> cashChange;
        std::optional<std::string> receivedBy;
        std::optional<std::string> location;
        std::optional<std::string> receiptNumber;
        std::optional<std::string> notes;

        // Card fields
        std::optional<std::string> provider;
        std::optional<std::string> paymentIntentId;
        std::optional<std::string> sessionId;
        std::optional<std::string> cardLast4;
        std::optional<std::string> cardBrand;
        std::optional<std::string> cardHolderName;
        std::optional<std::string> authorizationCode;
        std::optional<std::string> receiptUrl;
        std::optional<std::string> hostedUrl;
        std::optional<std::string> rawPayload;

        // Common audit fields
        std::optional<std::string> expiresAt;
        bool isActive = true;
        std::string createdBy = "system";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(Payment, (), (
        entry, code, paymentCode, type, entityType, entityCode,
        amount, currency, status,
        bankName, clabe, accountHolder, paymentReference,
        cashReceived, cashChange, receivedBy, location, receiptNumber, notes,
        provider, paymentIntentId, sessionId, cardLast4, cardBrand, cardHolderName, authorizationCode, receiptUrl, hostedUrl, rawPayload,
        expiresAt, isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))

    struct TransferTransaction
    {
        int entry = 0;
        std::string code;
        std::string paymentCode;
        std::string entityType = "ROUTE_RESERVATION";
        std::string entityCode;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string status = "PENDING";
        std::optional<std::string> bankName;
        std::optional<std::string> clabe;
        std::optional<std::string> accountHolder;
        std::optional<std::string> paymentReference;
        std::optional<std::string> receiptUrl;
        std::optional<std::string> expiresAt;
        bool isActive = true;
        std::string createdBy = "system";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(TransferTransaction, (), (
        entry, code, paymentCode, entityType, entityCode,
        amount, currency, status,
        bankName, clabe, accountHolder, paymentReference, receiptUrl,
        expiresAt, isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))

    struct CashTransaction
    {
        int entry = 0;
        std::string code;
        std::string paymentCode;
        std::string entityType = "ROUTE_RESERVATION";
        std::string entityCode;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string status = "PENDING";
        std::optional<double> cashReceived;
        std::optional<double> cashChange;
        std::optional<std::string> receivedBy;
        std::optional<std::string> location;
        std::optional<std::string> receiptNumber;
        std::optional<std::string> notes;
        std::optional<std::string> expiresAt;
        bool isActive = true;
        std::string createdBy = "system";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(CashTransaction, (), (
        entry, code, paymentCode, entityType, entityCode,
        amount, currency, status,
        cashReceived, cashChange, receivedBy, location, receiptNumber, notes,
        expiresAt, isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))

    struct CardTransaction
    {
        int entry = 0;
        std::string code;
        std::string paymentCode;
        std::string entityType = "ROUTE_RESERVATION";
        std::string entityCode;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string status = "PENDING";
        std::string provider = "STRIPE";
        std::optional<std::string> paymentIntentId;
        std::optional<std::string> sessionId;
        std::optional<std::string> cardLast4;
        std::optional<std::string> cardBrand;
        std::optional<std::string> cardHolderName;
        std::optional<std::string> authorizationCode;
        std::optional<std::string> receiptUrl;
        std::optional<std::string> hostedUrl;
        std::optional<std::string> rawPayload;
        std::optional<std::string> expiresAt;
        bool isActive = true;
        std::string createdBy = "system";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(CardTransaction, (), (
        entry, code, paymentCode, entityType, entityCode,
        amount, currency, status,
        provider, paymentIntentId, sessionId, cardLast4, cardBrand, cardHolderName, authorizationCode, receiptUrl, hostedUrl, rawPayload,
        expiresAt, isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
