#pragma once

#include <OmniCore/Base/BaseModel.hpp>
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models {

class CompanyPortalFeature {
public:
  CompanyPortalFeature() = default;
  CompanyPortalFeature(int _Entry,
                       std::string _Code,
                       int _CompanyEntry,
                       std::string _Title,
                       std::optional<std::string> _Description,
                       std::string _Icon,
                       std::string _Color,
                       int _SortOrder,
                       bool _IsActive,
                       std::string _CreatedBy,
                       std::string _CreateDate,
                       std::optional<std::string> _LastUpdatedBy,
                       std::optional<std::string> _UpdateDate)
      : Entry(_Entry),
        Code(std::move(_Code)),
        CompanyEntry(_CompanyEntry),
        Title(std::move(_Title)),
        Description(std::move(_Description)),
        Icon(std::move(_Icon)),
        Color(std::move(_Color)),
        SortOrder(_SortOrder),
        IsActive(_IsActive),
        CreatedBy(std::move(_CreatedBy)),
        CreateDate(std::move(_CreateDate)),
        LastUpdatedBy(std::move(_LastUpdatedBy)),
        UpdateDate(std::move(_UpdateDate)) {}

  int Entry{0};
  std::string Code;
  int CompanyEntry{1};
  std::string Title;
  std::optional<std::string> Description;
  std::string Icon{"stars"};
  std::string Color{"primary"};
  int SortOrder{0};
  bool IsActive{true};
  std::string CreatedBy{"system"};
  std::string CreateDate;
  std::optional<std::string> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
};

BOOST_DESCRIBE_STRUCT(CompanyPortalFeature, (),
                      (Entry,
                       Code,
                       CompanyEntry,
                       Title,
                       Description,
                       Icon,
                       Color,
                       SortOrder,
                       IsActive,
                       CreatedBy,
                       CreateDate,
                       LastUpdatedBy,
                       UpdateDate))

} // namespace omnisphere::models

