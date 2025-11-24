#pragma once
#include <string>
#include <optional>
#include <regex>
#include <stdexcept>

namespace omnicore::dto
{
    struct BaseUpdateDTO
    {
        BaseUpdateDTO(
            std::string _Code, 
            std::optional<std::string> _Name,
            int _LastUpdatedBy, 
            std::string _UpdateDate)
            : Code(std::move(_Code)),
              Name(std::move(_Name)),
              LastUpdatedBy(_LastUpdatedBy),
              UpdateDate(std::move(_UpdateDate))
        {
            Validate();
        }

        const std::string Code;
        const std::optional<std::string> Name;
        const int LastUpdatedBy;
        const std::string UpdateDate;

        void Validate()
        {
            if(Code.empty())
                throw std::runtime_error("Validation error: 'Code' cannot be empty.");

            if(Name.has_value() && Name.value().empty())
                throw std::runtime_error("Validation error: 'Name' cannot be empty.");

            if (!std::regex_match(Code, codeRegex))
                throw std::runtime_error("Validation error: 'Code' must be 3-20 characters, alphanumeric only, and must not contain whitespace.");

            if (Name.has_value() && !std::regex_match(Name.value(), nameRegex))
                throw std::runtime_error("Validation error: 'Name' must be between 3 and 50 characters, may include internal spaces, and must not start or end with whitespace.");

            if (!std::regex_match(std::to_string(LastUpdatedBy), positiveIntRegex))
                throw std::runtime_error("Validation error: 'LastUpdatedBy' must be a positive, non-zero integer.");

            if (UpdateDate.empty())
                throw std::runtime_error("Validation error: 'UpdateDate' cannot be empty.");
        }

        const std::regex codeRegex{ R"(^[A-Za-z0-9]{3,20}$)" };
        const std::regex nameRegex{ R"(^[^\s][A-Za-z0-9\s]{1,48}[^\s]$)" };
        const std::regex positiveIntRegex{ R"(^[1-9][0-9]*$)" };
        const std::regex dateRegex{ R"(^\d{4}-\d{2}-\d{2}$)" };
    };
}
