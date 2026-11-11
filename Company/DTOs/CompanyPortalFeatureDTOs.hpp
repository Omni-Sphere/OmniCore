#pragma once

#include <OmniCore/Base/BaseModel.hpp>
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::dtos {

struct GetPortalFeatures {
  int CompanyEntry{1};
  std::optional<bool> OnlyActive;
};

BOOST_DESCRIBE_STRUCT(GetPortalFeatures, (), (CompanyEntry, OnlyActive))

struct CreatePortalFeature {
  std::optional<std::string> Code;
  int CompanyEntry{1};
  std::string Title;
  std::optional<std::string> Description;
  std::string Icon{"stars"};
  std::string Color{"primary"};
  int SortOrder{0};
  bool IsActive{true};
  std::string CreatedBy{"system"};
};

BOOST_DESCRIBE_STRUCT(CreatePortalFeature, (),
                      (Code, CompanyEntry, Title, Description, Icon, Color, SortOrder, IsActive, CreatedBy))

struct UpdatePortalFeature {
  int Entry{0};
  std::optional<std::string> Title;
  std::optional<std::string> Description;
  std::optional<std::string> Icon;
  std::optional<std::string> Color;
  std::optional<int> SortOrder;
  std::optional<bool> IsActive;
  std::optional<std::string> LastUpdatedBy;
  std::optional<std::string> UpdateDate;
};

BOOST_DESCRIBE_STRUCT(UpdatePortalFeature, (),
                      (Entry, Title, Description, Icon, Color, SortOrder, IsActive, LastUpdatedBy, UpdateDate))

struct DeletePortalFeature {
  int Entry{0};
};

BOOST_DESCRIBE_STRUCT(DeletePortalFeature, (), (Entry))

} // namespace omnisphere::dtos

