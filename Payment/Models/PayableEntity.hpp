#pragma once
#include <string>
#include <optional>
#include <boost/json.hpp>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct PayableEntity
    {
        std::string entityType;              // e.g. "ROUTE_RESERVATION", "CAFE_ORDER", "ERP_INVOICE"
        std::string entityCode;              // e.g. "RSV027", "ORD-101", "INV-500"
        double amount = 0.0;
        std::string currency = "mxn";
        std::string customerName;
        std::string customerEmail;
        std::string customerPhone;
        int expiresInMinutes = 60;           // TTL: 3 min for CARD, 60 min for TRANSFER
        std::string successUrl;
        std::string cancelUrl;
        int seats = 1;
        boost::json::object metadata;
    };

    BOOST_DESCRIBE_STRUCT(PayableEntity, (), (
        entityType,
        entityCode,
        amount,
        currency,
        customerName,
        customerEmail,
        customerPhone,
        expiresInMinutes,
        successUrl,
        cancelUrl,
        seats
    ))
} // namespace omnisphere::models
