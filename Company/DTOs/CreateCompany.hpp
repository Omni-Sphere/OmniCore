#pragma once

#include <OmniCore/Base/BaseModel.hpp>
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::dtos {
struct CreateCompany {
  std::string Code;
  std::string Name;
  std::optional<std::string> CommercialName;
  std::optional<std::string> Address;
  std::optional<std::string> Address2;
  std::optional<int> City;
  std::optional<int> State;
  std::optional<int> ZipCode;
  std::optional<int> Country;
  std::optional<std::string> TaxID;
  int Currency{0};
  std::optional<std::string> Phone1;
  std::optional<std::string> Phone2;
  std::optional<std::string> Email;
  std::optional<std::string> WebSite;
  std::optional<std::string> FacebookProfile;
  std::optional<std::string> InstagramProfile;
  std::optional<std::string> XProfile;
  std::optional<std::string> LogoFile;
  std::optional<std::string> PrimaryColor;
  std::optional<std::string> SecondaryColor;
  std::optional<std::string> PaletteJson;
  bool IsActive{true};
  int CreatedBy{0};
};

BOOST_DESCRIBE_STRUCT(CreateCompany, (),
                      (Code, Name, CommercialName, Address, Address2, City, State,
                       ZipCode, Country, TaxID, Currency, Phone1, Phone2, Email,
                       WebSite, FacebookProfile, InstagramProfile, XProfile,
                       LogoFile, PrimaryColor, SecondaryColor, PaletteJson,
                       IsActive, CreatedBy))
} // namespace omnisphere::dtos

