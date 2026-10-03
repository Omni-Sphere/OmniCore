#include "GlobalConfiguration/Repositories/GlobalConfiguration.hpp"
#include <stdexcept>
#include <vector>

namespace omnisphere::repositories {
GlobalConfiguration::GlobalConfiguration(
    std::shared_ptr<omnisphere::data::DatabasePool> _database)
    : database(std::move(_database)) {}

bool GlobalConfiguration::Update(
    const omnisphere::dtos::UpdateGlobalConfiguration &config) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery = "UPDATE \"GlobalConfiguration\" SET ";
    std::vector<omnisphere::types::SQLParam> updateParams;
    bool firstField = true;

    if (config.ImagePath.has_value()) {
      sQuery += "\"ImagePath\" = ?";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(config.ImagePath.value()));
      firstField = false;
    }

    if (config.PDFPath.has_value()) {
      if (!firstField)
        sQuery += ", ";
      sQuery += "\"PDFPath\" = ?";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(config.PDFPath.value()));
      firstField = false;
    }

    if (config.XMLPath.has_value()) {
      if (!firstField)
        sQuery += ", ";
      sQuery += "\"XMLPath\" = ?";
      updateParams.emplace_back(
          omnisphere::types::MakeSQLParam(config.XMLPath.value()));
      firstField = false;
    }

    if (config.PasswordExpirationDays.has_value()) {
      if (!firstField)
        sQuery += ", ";
      sQuery += "\"PasswordExpirationDays\" = ?";
      updateParams.emplace_back(omnisphere::types::MakeSQLParam(
          config.PasswordExpirationDays.value()));
      firstField = false;
    }

    if (firstField) {
      return true;
    }

    conn->BeginTransaction();

    if (!conn->RunPrepared(sQuery, updateParams)) {
      conn->RollbackTransaction();
      return false;
    }

    conn->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    conn->RollbackTransaction();
    throw std::runtime_error(
        std::string("[GlobalConfiguration Update Exception] ") + e.what());
  }
}

omnisphere::models::GlobalConfiguration
GlobalConfiguration::Get(int confEntry) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery =
        "SELECT \"ConfEntry\", \"ImagePath\", \"PDFPath\", \"XMLPath\", "
        "\"PasswordExpirationDays\" FROM \"GlobalConfiguration\" WHERE \"ConfEntry\" = ?";

    omnisphere::types::DataTable data =
        conn->FetchPrepared(sQuery, std::to_string(confEntry));

    if (data.IsEmpty()) {
      throw std::runtime_error("Configuration not found");
    }

    const auto& row = data[0];
    omnisphere::models::GlobalConfiguration config;
    config.ConfEntry = row["ConfEntry"];
    config.ImagePath = row["ImagePath"].GetOptional<std::string>();
    config.PDFPath = row["PDFPath"].GetOptional<std::string>();
    config.XMLPath = row["XMLPath"].GetOptional<std::string>();
    config.PasswordExpirationDays = row["PasswordExpirationDays"];

    return config;
  } catch (const std::exception &e) {
    throw std::runtime_error(
        std::string("[GlobalConfiguration Get Exception] ") + e.what());
  }
}
} // namespace omnisphere::repositories
