#pragma once
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>

namespace omnisphere::dtos {
struct BaseCreateDTO {
  BaseCreateDTO(std::string _Code, std::string _Name, int _CreatedBy,
                std::string _CreateDate)
      : Code(std::move(_Code)), Name(std::move(_Name)),
        CreatedBy(std::move(_CreatedBy)), CreateDate(std::move(_CreateDate)) {
    Validate();
  }

  const std::string Code;
  const std::string Name;
  const int CreatedBy;
  const std::string CreateDate;

  void Validate() {
    // Validations disabled as requested
  }

  const std::regex codeLengthRegex{R"(^.{3,20}$)"};
  const std::regex codeAlnumRegex{R"(^[A-Za-z0-9]+$)"};
  const std::regex nameLengthRegex{R"(^.{3,50}$)"};
  const std::regex nameLeadingTrailingSpacesRegex{R"(^\s+.*|.*\s+$)"};
  const std::regex nameValidCharsRegex{R"(^[A-Za-z0-9\s]+$)"};
  const std::regex positiveIntRegex{R"(^[1-9][0-9]*$)"};
  const std::regex dateRegex{R"(^\d{4}-\d{2}-\d{2}$)"};
};
} // namespace omnisphere::dtos
