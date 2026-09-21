#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct CreateEmployee
    {
        std::string code;
        std::string name;
        std::optional<std::string> firstName;
        std::optional<std::string> secondName;
        std::optional<std::string> lastName;
        std::optional<std::string> secondLastName;
        std::optional<std::string> email;
        std::optional<std::string> phone;
        std::optional<std::string> department;
        std::optional<std::string> position;
        std::optional<std::string> directManagerCode;
        std::optional<std::string> dateOfBirth;
        std::optional<std::string> comments;
        bool isActive = true;
        int createdBy = 1;
    };
    BOOST_DESCRIBE_STRUCT(CreateEmployee, (), (
        code, name, firstName, secondName, lastName, secondLastName,
        email, phone, department, position, directManagerCode,
        dateOfBirth, comments, isActive, createdBy
    ))
} // namespace omnisphere::dtos
