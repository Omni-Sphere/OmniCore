#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>
#include "Payment/DTOs/PaymentMethodDetailInput.hpp"

namespace omnisphere::dtos
{
    struct CreatePaymentMethodInput
    {
        std::string Code;
        std::string Name;
        std::string Type = "NOT_APPLICABLE";
        bool UsesCommission = false;
        double CommissionRate = 0.0;
        bool UsesIntegration = false;
        std::optional<std::string> IntegrationProvider;
        bool IsActive = true;
        int CreatedBy = 0;
        std::optional<std::string> CreateDate;

        std::optional<PaymentMethodDetailInput> Details;
    };

    BOOST_DESCRIBE_STRUCT(CreatePaymentMethodInput, (), (
        Code, Name, Type, UsesCommission, CommissionRate,
        UsesIntegration, IntegrationProvider, IsActive, CreatedBy, CreateDate
    ))
} // namespace omnisphere::dtos
