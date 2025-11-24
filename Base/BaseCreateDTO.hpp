#pragma once
#include <string>
#include <optional>
#include <regex>
#include <stdexcept>

namespace omnicore::dto
{
    struct BaseCreateDTO
    {
        BaseCreateDTO(
            std::string _Code,
            std::string _Name,
            int _CreatedBy,
            std::string _CreateDate)
            : Code(std::move(_Code)),
              Name(std::move(_Name)),
              CreatedBy(std::move(_CreatedBy)),
              CreateDate(std::move(_CreateDate))
        {
            Validate();
        }

        const std::string Code;
        const std::string Name;
        const int CreatedBy;
        const std::string CreateDate;

        void Validate()
        {
            if (Code.empty())
                throw std::runtime_error("Validation error: 'Code' cannot be empty.");

            if (Name.empty())
                throw std::runtime_error("Validation error: 'Name' cannot be empty.");
                
            if (CreateDate.empty())
                throw std::runtime_error("Validation error: 'CreateDate' cannot be empty.");

            if (!std::regex_match(Code, codeLengthRegex))
                throw std::runtime_error("Validation error: 'Code' must be 3–20 characters long.");

            if (!std::regex_match(Code, codeAlnumRegex))
                throw std::runtime_error("Validation error: 'Code' must contain only alphanumeric characters.");

            if (!std::regex_match(Name, nameLengthRegex))
                throw std::runtime_error("Validation error: 'Name' must be 3–50 characters long.");

            if (std::regex_match(Name, nameLeadingTrailingSpacesRegex))
                throw std::runtime_error("Validation error: 'Name' cannot start or end with whitespace.");

            if (!std::regex_match(Name, nameValidCharsRegex))
                throw std::runtime_error("Validation error: 'Name' has invalid characters.");

            if (!std::regex_match(std::to_string(CreatedBy), positiveIntRegex))
                throw std::runtime_error("Validation error: 'CreatedBy' must be a positive non-zero integer.");
        }

        const std::regex codeLengthRegex{R"(^.{3,20}$)"};
        const std::regex codeAlnumRegex{R"(^[A-Za-z0-9]+$)"};
        const std::regex nameLengthRegex{R"(^.{3,50}$)"};
        const std::regex nameLeadingTrailingSpacesRegex{R"(^\s+.*|.*\s+$)"};
        const std::regex nameValidCharsRegex{R"(^[A-Za-z0-9\s]+$)"};
        const std::regex positiveIntRegex{R"(^[1-9][0-9]*$)"};
        const std::regex dateRegex{R"(^\d{4}-\d{2}-\d{2}$)"};
    };
}
