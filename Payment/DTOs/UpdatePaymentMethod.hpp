#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>
#include "Payment/DTOs/PaymentMethodDetailInput.hpp"

namespace omnisphere::dtos
{
    struct UpdatePaymentMethodInput
    {
        std::string Code;
        int Entry = 0; // Campo legado, preferir siempre Code
        std::optional<std::string> Name;
        std::optional<std::string> Type;
        std::optional<bool> UsesCommission;
        std::optional<double> CommissionRate;
        std::optional<bool> UsesIntegration;
        std::optional<std::string> IntegrationProvider;
        std::optional<bool> IsActive;
        std::string LastUpdatedBy = "SYSTEM";
        std::optional<std::string> UpdateDate;

        std::optional<PaymentMethodDetailInput> Details;
    };

    BOOST_DESCRIBE_STRUCT(UpdatePaymentMethodInput, (), (
        Code, Name, Type, UsesCommission, CommissionRate,
        UsesIntegration, IntegrationProvider, IsActive, UpdateDate
    ))
} // namespace omnisphere::dtos
