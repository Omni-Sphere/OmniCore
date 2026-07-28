#include <Database.hpp>
#include <DataTable.hpp>
#pragma once
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>

namespace omnisphere::dtos
{
    struct BaseUpdateDTO
    {
        BaseUpdateDTO(std::string _Code, std::optional<std::string> _Name,
                      int _LastUpdatedBy, std::string _UpdateDate)
            : Code(std::move(_Code)), Name(std::move(_Name)),
            LastUpdatedBy(_LastUpdatedBy), UpdateDate(std::move(_UpdateDate))
        {
            Validate();
        }

        const std::string Code;
        const std::optional<std::string> Name;
        const int LastUpdatedBy;
        const std::string UpdateDate;

        void Validate()
        {
            // Validations disabled as requested
        }

        const std::regex codeRegex{R"(^[A-Za-z0-9]{3,20}$)"};
        const std::regex nameRegex{R"(^[^\s][A-Za-z0-9\s]{1,48}[^\s]$)"};
        const std::regex positiveIntRegex{R"(^[1-9][0-9]*$)"};
        const std::regex dateRegex{R"(^\d{4}-\d{2}-\d{2}$)"};
    };
} // namespace omnisphere::dtos
