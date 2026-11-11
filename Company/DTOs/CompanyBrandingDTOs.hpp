#pragma once

#include <OmniCore/Base/BaseModel.hpp>
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::dtos {

struct GetCompanyBranding {
  int CompanyEntry{1};
};

BOOST_DESCRIBE_STRUCT(GetCompanyBranding, (), (CompanyEntry))

struct UpdateCompanyBranding {
  int CompanyEntry{1};
  std::optional<std::string> LogoFile;
  std::optional<std::string> Slogan;
  std::optional<std::string> SloganDescription;

  // Primary brand palette
  std::optional<std::string> PrimaryColor;
  std::optional<std::string> PrimaryContainerColor;
  std::optional<std::string> PrimaryDarkColor;
  std::optional<std::string> PrimaryLightColor;

  // Secondary brand palette
  std::optional<std::string> SecondaryAmberColor;
  std::optional<std::string> SecondaryOrangeColor;
  std::optional<std::string> SecondaryNavyColor;
  std::optional<std::string> SecondaryCoralColor;

  std::optional<bool> IsActive;
  std::optional<std::string> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
};

BOOST_DESCRIBE_STRUCT(UpdateCompanyBranding, (),
                      (CompanyEntry,
                       LogoFile,
                       Slogan,
                       SloganDescription,
                       PrimaryColor,
                       PrimaryContainerColor,
                       PrimaryDarkColor,
                       PrimaryLightColor,
                       SecondaryAmberColor,
                       SecondaryOrangeColor,
                       SecondaryNavyColor,
                       SecondaryCoralColor,
                       IsActive,
                       LastUpdatedBy,
                       UpdateDate))

} // namespace omnisphere::dtos

