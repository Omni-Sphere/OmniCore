#pragma once

#include <OmniCore/Base/BaseModel.hpp>
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::dtos {
struct UpdateCompany {
  int Entry{1};
  std::optional<std::string> Code;
  std::optional<std::string> Name;
  std::optional<std::string> CommercialName;
  std::optional<std::string> Address;
  std::optional<std::string> Address2;
  std::optional<int> City;
  std::optional<int> State;
  std::optional<int> ZipCode;
  std::optional<int> Country;
  std::optional<std::string> TaxID;
  std::optional<int> Currency;
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
  std::optional<bool> IsActive;
  std::optional<int> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
};

BOOST_DESCRIBE_STRUCT(UpdateCompany, (),
                      (Entry, Code, Name, CommercialName, Address, Address2, City, State,
                       ZipCode, Country, TaxID, Currency, Phone1, Phone2, Email,
                       WebSite, FacebookProfile, InstagramProfile, XProfile,
                       LogoFile, PrimaryColor, SecondaryColor, PaletteJson,
                       IsActive, LastUpdatedBy, UpdateDate))
} // namespace omnisphere::dtos

