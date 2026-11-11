#pragma once

#include <OmniCore/Base/BaseModel.hpp>
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models {

class CompanyBranding {
public:
  CompanyBranding() = default;
  CompanyBranding(int _Entry, int _CompanyEntry,
                  std::optional<std::string> _LogoFile,
                  std::optional<std::string> _Slogan,
                  std::optional<std::string> _SloganDescription,
                  std::string _PrimaryColor,
                  std::string _PrimaryContainerColor,
                  std::string _PrimaryDarkColor,
                  std::string _PrimaryLightColor,
                  std::string _SecondaryAmberColor,
                  std::string _SecondaryOrangeColor,
                  std::string _SecondaryNavyColor,
                  std::string _SecondaryCoralColor,
                  bool _IsActive,
                  std::string _CreatedBy,
                  std::string _CreateDate,
                  std::optional<std::string> _LastUpdatedBy,
                  std::optional<std::string> _UpdateDate)
      : Entry(_Entry), CompanyEntry(_CompanyEntry),
        LogoFile(std::move(_LogoFile)),
        Slogan(std::move(_Slogan)),
        SloganDescription(std::move(_SloganDescription)),
        PrimaryColor(std::move(_PrimaryColor)),
        PrimaryContainerColor(std::move(_PrimaryContainerColor)),
        PrimaryDarkColor(std::move(_PrimaryDarkColor)),
        PrimaryLightColor(std::move(_PrimaryLightColor)),
        SecondaryAmberColor(std::move(_SecondaryAmberColor)),
        SecondaryOrangeColor(std::move(_SecondaryOrangeColor)),
        SecondaryNavyColor(std::move(_SecondaryNavyColor)),
        SecondaryCoralColor(std::move(_SecondaryCoralColor)),
        IsActive(_IsActive),
        CreatedBy(std::move(_CreatedBy)),
        CreateDate(std::move(_CreateDate)),
        LastUpdatedBy(std::move(_LastUpdatedBy)),
        UpdateDate(std::move(_UpdateDate)) {}

  int Entry{0};
  int CompanyEntry{1};
  std::optional<std::string> LogoFile;
  std::optional<std::string> Slogan;
  std::optional<std::string> SloganDescription;

  // Paleta de Colores Principales
  std::string PrimaryColor{"#45a6df"};
  std::string PrimaryContainerColor{"#2d77b4"};
  std::string PrimaryDarkColor{"#20436a"};
  std::string PrimaryLightColor{"#95c0d8"};

  // Paleta de Colores Secundarios
  std::string SecondaryAmberColor{"#F2A65A"};
  std::string SecondaryOrangeColor{"#E97845"};
  std::string SecondaryNavyColor{"#20436a"};
  std::string SecondaryCoralColor{"#D96C5F"};

  bool IsActive{true};
  std::string CreatedBy{"system"};
  std::string CreateDate;
  std::optional<std::string> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
};

BOOST_DESCRIBE_STRUCT(CompanyBranding, (),
                      (Entry, CompanyEntry, LogoFile, Slogan, SloganDescription,
                       PrimaryColor, PrimaryContainerColor, PrimaryDarkColor, PrimaryLightColor,
                       SecondaryAmberColor, SecondaryOrangeColor, SecondaryNavyColor, SecondaryCoralColor,
                       IsActive, CreatedBy, CreateDate, LastUpdatedBy, UpdateDate))

} // namespace omnisphere::models

