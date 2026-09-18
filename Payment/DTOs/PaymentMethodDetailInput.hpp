#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct PaymentMethodDetailInput
    {
        std::string BankName;
        std::string Clabe;
        std::string AccountHolder;
        std::optional<std::string> PaymentReference;
    };

    BOOST_DESCRIBE_STRUCT(PaymentMethodDetailInput, (), (
        BankName, Clabe, AccountHolder, PaymentReference
    ))
} // namespace omnisphere::dtos
