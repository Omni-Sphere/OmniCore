#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct CreateIdentityInput
    {
        std::string domain;
        std::string prefix1;
        std::optional<std::string> prefix2;
        std::optional<std::string> prefix3;
        int initialSequence = 0;
        int createdBy = 1;
    };

    BOOST_DESCRIBE_STRUCT(CreateIdentityInput, (), (
        domain, prefix1, prefix2, prefix3, initialSequence, createdBy
    ))
} // namespace omnisphere::dtos
