#pragma once

#include <OmniCore/Base/BaseModel.hpp>
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models {
class Company : public omnisphere::models::BaseModel {
public:
  Company() = default;
  Company(int _Entry, std::string _Code, std::string _Name,
          std::optional<std::string> _CommercialName,
          std::optional<std::string> _Address,
          std::optional<std::string> _Address2, std::optional<int> _City,
          std::optional<int> _State, std::optional<int> _ZipCode,
          std::optional<int> _Country, std::optional<std::string> _TaxID,
          int _Currency, std::optional<std::string> _Phone1,
          std::optional<std::string> _Phone2, std::optional<std::string> _Email,
          std::optional<std::string> _WebSite,
          std::optional<std::string> _FacebookProfile,
          std::optional<std::string> _InstagramProfile,
          std::optional<std::string> _XProfile,
          std::optional<std::string> _LogoFile,
          std::optional<std::string> _PrimaryColor,
          std::optional<std::string> _SecondaryColor,
          std::optional<std::string> _PaletteJson,
          bool _IsActive, int _CreatedBy,
          std::string _CreateDate, std::optional<int> _LastUpdatedBy,
          std::optional<std::string> _UpdateDate)
      : omnisphere::models::BaseModel(
            _Entry, std::move(_Code), std::move(_Name), _CreatedBy,
            std::move(_CreateDate), _LastUpdatedBy, std::move(_UpdateDate)),
        Address(std::move(_Address)), Address2(std::move(_Address2)),
        City(_City), State(_State), ZipCode(_ZipCode), Country(_Country),
        TaxID(std::move(_TaxID)), Currency(_Currency),
        Phone1(std::move(_Phone1)), Phone2(std::move(_Phone2)),
        Email(std::move(_Email)), WebSite(std::move(_WebSite)),
        FacebookProfile(std::move(_FacebookProfile)),
        InstagramProfile(std::move(_InstagramProfile)),
        XProfile(std::move(_XProfile)), LogoFile(std::move(_LogoFile)),
        PrimaryColor(std::move(_PrimaryColor)),
        SecondaryColor(std::move(_SecondaryColor)),
        PaletteJson(std::move(_PaletteJson)),
        CommercialName(std::move(_CommercialName)), IsActive(_IsActive) {}

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
  std::optional<std::string> CommercialName;
  bool IsActive{true};
};

BOOST_DESCRIBE_STRUCT(Company, (omnisphere::models::BaseModel),
                      (Address, Address2, City, State, ZipCode, Country, TaxID, Currency,
                       Phone1, Phone2, Email, WebSite, FacebookProfile, InstagramProfile,
                       XProfile, LogoFile, PrimaryColor, SecondaryColor, PaletteJson,
                       CommercialName, IsActive))
} // namespace omnisphere::models

