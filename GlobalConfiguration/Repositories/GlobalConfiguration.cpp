#include "Repositories/GlobalConfiguration.hpp"
#include <stdexcept>
#include <vector>

namespace omnisphere::omnicore::repositories {
GlobalConfiguration::GlobalConfiguration(
    std::shared_ptr<omnisphere::omnidata::services::Database> _database)
    : database(std::move(_database)) {}

bool GlobalConfiguration::Update(
    const omnisphere::omnicore::dtos::UpdateGlobalConfiguration &config) const {
  try {
    std::string sQuery = "UPDATE GlobalConfiguration SET ";
    std::vector<omnisphere::omnidata::types::SQLParam> updateParams;
    bool firstField = true;

    if (config.ImagePath.has_value()) {
      sQuery += "ImagePath = ?";
      updateParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(config.ImagePath.value()));
      firstField = false;
    }

    if (config.PDFPath.has_value()) {
      if (!firstField)
        sQuery += ", ";
      sQuery += "PDFPath = ?";
      updateParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(config.PDFPath.value()));
      firstField = false;
    }

    if (config.XMLPath.has_value()) {
      if (!firstField)
        sQuery += ", ";
      sQuery += "XMLPath = ?";
      updateParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(config.XMLPath.value()));
      firstField = false;
    }

    if (config.PasswordExpirationDays.has_value()) {
      if (!firstField)
        sQuery += ", ";
      sQuery += "PasswordExpirationDays = ?";
      updateParams.emplace_back(omnisphere::omnidata::types::MakeSQLParam(
          config.PasswordExpirationDays.value()));
      firstField = false;
    }

    // If no fields were updated, return true as nothing needs to be done
    if (firstField) {
      return true;
    }

    database->BeginTransaction();

    if (!database->RunPrepared(sQuery, updateParams)) {
      database->RollbackTransaction();
      return false;
    }

    database->CommitTransaction();
    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw std::runtime_error(
        std::string("[GlobalConfiguration Update Exception] ") + e.what());
  }
}

omnisphere::omnicore::models::GlobalConfiguration
GlobalConfiguration::Get(int confEntry) const {
  try {
    std::string sQuery =
        "SELECT ConfEntry, ImagePath, PDFPath, XMLPath, "
        "PasswordExpirationDays FROM GlobalConfiguration WHERE ConfEntry = ?";

    omnisphere::omnidata::types::DataTable data =
        database->FetchPrepared(sQuery, std::to_string(confEntry));

    if (data.RowsCount() == 0) {
      throw std::runtime_error("Configuration not found");
    }

    omnisphere::omnicore::models::GlobalConfiguration config;
    config.ConfEntry = data[0]["ConfEntry"];

    if (!data[0]["ImagePath"].IsNull())
      config.ImagePath = (std::string)data[0]["ImagePath"];

    if (!data[0]["PDFPath"].IsNull())
      config.PDFPath = (std::string)data[0]["PDFPath"];

    if (!data[0]["XMLPath"].IsNull())
      config.XMLPath = (std::string)data[0]["XMLPath"];

    config.PasswordExpirationDays = data[0]["PasswordExpirationDays"];

    return config;
  } catch (const std::exception &e) {
    throw std::runtime_error(
        std::string("[GlobalConfiguration Get Exception] ") + e.what());
  }
}
} // namespace omnisphere::omnicore::repositories
