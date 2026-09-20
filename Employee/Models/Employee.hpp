#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct Employee
    {
        int entry = 0;
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
        std::optional<std::string> userCode;
        std::optional<std::string> dateOfBirth;
        std::optional<std::string> comments;
        bool isActive = true;
        int createdBy = 1;
        std::string createDate;
        std::optional<int> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };
    BOOST_DESCRIBE_STRUCT(Employee, (), (
        entry, code, name, firstName, secondName, lastName, secondLastName,
        email, phone, department, position, directManagerCode, userCode,
        dateOfBirth, comments, isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
