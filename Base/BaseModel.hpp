#include <Database.hpp>
#include <DataTable.hpp>
#pragma once
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    class BaseModel
    {
        public:
        BaseModel() = default;
        BaseModel(
            int _Entry,
            std::string _Code,
            std::string _Name,
            int _CreatedBy,
            std::string _CreateDate,
            std::optional<int> _LastUpdatedBy,
            std::optional<std::string> _UpdateDate
        )
            : Entry(
                _Entry
            ),
            Code(std::move(_Code)),
            Name(std::move(_Name)),
            CreatedBy(_CreatedBy),
            CreateDate(std::move(_CreateDate)),
            LastUpdatedBy(_LastUpdatedBy),
            UpdateDate(std::move(_UpdateDate))
        {
            Validate();
        }

        int Entry;
        std::string Code;
        std::string Name;
        int CreatedBy;
        std::string CreateDate;
        std::optional<int> LastUpdatedBy;
        std::optional<std::string> UpdateDate;

        protected:
        void Validate()
        {
            if (Code.size() < 3)
                throw std::runtime_error("Code demasiado corto");

            if (Code.size() > 20)
                throw std::runtime_error("Code demasiado largo");

            if (Code.find(' ') != std::string::npos)
                throw std::runtime_error("Code no puede contener espacios");

            if (!std::regex_match(Code, alphaNumRegex))
                throw std::runtime_error(
                    "Code solo puede contener caracteres alfanuméricos");

            if (Name.size() < 3)
                throw std::runtime_error("Name demasiado corto");

            if (Name.size() > 50)
                throw std::runtime_error("Name demasiado largo");

            if (CreatedBy <= 0)
                throw std::runtime_error("CreatedBy inválido");
        }

        private:
        const std::regex alphaNumRegex{"^[A-Za-z0-9]+$"};
    };
    BOOST_DESCRIBE_STRUCT(BaseModel, (), (Entry, Code, Name, CreatedBy, CreateDate, LastUpdatedBy, UpdateDate))
} // namespace omnisphere::models
