#include "Session.hpp"

namespace omnisphere::omnicore::repositories {
Session::Session(
    std::shared_ptr<omnisphere::omnidata::services::Database> _database) {
  database = std::move(_database);
}

bool Session::Create(const omnisphere::omnicore::dtos::Login &login) const {
  try {
    std::string sQuery = "INSERT INTO Sessions ("
                         "SessionEntry, "
                         "SessionUUID, ";

    if (login.Code.has_value())
      sQuery += "UserCode, ";

    if (login.Email.has_value())
      sQuery += "UserEmail, ";

    if (login.Phone.has_value())
      sQuery += "UserPhone, ";

    sQuery += "StartDate, "
              "DeviceIP, "
              "HostName "
              ") VALUES ("
              "?, "
              "NEWID(), "
              "?, "
              "?, "
              "?, "
              "?)";

    std::vector<omnisphere::omnidata::types::SQLParam> vParams;

    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(GetCurrentSequence()));

    if (login.Code.has_value())
      vParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(login.Code.value()));

    if (login.Email.has_value())
      vParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(login.Email.value()));

    if (login.Phone.has_value())
      vParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(login.Phone.value()));

    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(login.StartDate));
    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(login.DeviceIP));
    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(login.HostName));

    database->BeginTransaction();

    if (!database->RunPrepared(sQuery, vParams))
      throw std::runtime_error("[RunPrepared exception]");

    if (!UpdateSessionSequence())
      throw std::runtime_error("Error updating sesion sequence");

    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    database->RollbackTransaction();
    throw(std::runtime_error(std::string("[LoginException]") + e.what()));
  }
};

int Session::GetCurrentSequence() const {
  try {
    const std::string sQuery =
        "SELECT ISNULL(SessionSequence, 0) + 1 SessionSequence FROM Sequences "
        "WHERE SeqEntry = 1";

    omnisphere::omnidata::types::DataTable data =
        database->FetchResults(sQuery);

    if (data.RowsCount() == 1)
      return data[0]["SessionSequence"];
    else
      return 0;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[GetCurrentSequence Exception] ") +
                             " " + e.what());
  }
};

bool Session::UpdateSessionSequence() const {
  try {
    const std::string sQuery =
        "UPDATE Sequences SET SessionSequence = ISNULL(SessionSequence,0) + 1";

    if (!database->RunStatement(sQuery))
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[SessionSequence Exception] ") + " " +
                             e.what());
  }
};

omnisphere::omnidata::types::DataTable
Session::Read(const omnisphere::omnicore::dtos::Login &login) const {
  try {
    std::string sQuery = "SELECT "
                         "T0.SessionEntry, "
                         "T0.SessionUUID, "
                         "T0.UserCode, "
                         "T0.UserEmail, "
                         "T0.UserPhone, "
                         "T0.IsActive, "
                         "T0.StartDate, "
                         "T0.DeviceIP, "
                         "T0.HostName, "
                         "T0.EndDate, "
                         "T0.DurationSeconds "
                         "FROM Sessions T0 "
                         "JOIN Users T1 ON ";

    std::vector<omnisphere::omnidata::types::SQLParam> vParams;

    if (login.Code.has_value()) {
      sQuery += "T0.UserCode = T1.[Code] WHERE T1.[Code] = ? ";
      vParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(login.Code.value()));
    }

    if (login.Email.has_value()) {
      sQuery += "T0.UserEmail = T1.Email WHERE T1.Email = ? ";
      vParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(login.Email.value()));
    }

    if (login.Phone.has_value()) {
      sQuery += "T0.UserPhone = T1.Phone WHERE T1.Phone = ? ";
      vParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(login.Phone.value()));
    }

    sQuery += "AND T0.DeviceIP = ? AND T0.HostName = ? AND T0.IsActive = 'Y'";
    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(login.DeviceIP));
    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(login.HostName));

    omnisphere::omnidata::types::DataTable data =
        database->FetchPrepared(sQuery, vParams);

    return data;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadSessionData Exception] ") + " " +
                             e.what());
  }
};

omnisphere::omnidata::types::DataTable
Session::Read(const std::string &sessionUUID) const {
  try {
    std::string sQuery =
        "SELECT SessionUUID, StartDate, EndDate, DurationSeconds, Reason, "
        "LogoutMessage FROM Sessions WHERE SessionUUID = ?";
    return database->FetchPrepared(sQuery, sessionUUID);
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadSessionData Exception] ") + " " +
                             e.what());
  }
};

omnisphere::omnidata::types::DataTable
Session::ExistsUUID(const std::string &sessionUUID) const {
  try {
    std::string sQuery =
        "SELECT COUNT(*) Total FROM Sessions WHERE SessionUUID  = ?";

    omnidata::types::DataTable data =
        database->FetchPrepared(sQuery, sessionUUID);

    return data;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ValidateSessionUUID Exception] ") +
                             " " + e.what());
  }
};

omnisphere::omnidata::types::DataTable
Session::IsActive(const std::string &sessionUUID) const {
  try {
    std::string sQuery = "SELECT IsActive FROM Sessions WHERE SessionUUID  = ?";

    omnidata::types::DataTable data =
        database->FetchPrepared(sQuery, sessionUUID);

    return data;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[IsSessionActive Exception] ") + " " +
                             e.what());
  }
};

bool Session::Close(const omnisphere::omnicore::dtos::Logout &logout) const {
  try {
    std::string sQuery =
        "UPDATE Sessions SET IsActive = 'N', EndDate = ?, DurationSeconds = ? ";
    std::vector<omnisphere::omnidata::types::SQLParam> vParams;

    if (logout.Message.has_value())
      sQuery += ", LogoutMessage = ? ";

    sQuery += ", Reason = ? WHERE SessionUUID  = ? AND IsActive = 'Y'";

    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(logout.EndDate));
    vParams.emplace_back(omnisphere::omnidata::types::MakeSQLParam(3600));

    if (logout.Message.has_value())
      vParams.emplace_back(
          omnisphere::omnidata::types::MakeSQLParam(logout.Message));

    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(logout.Reason));
    vParams.emplace_back(
        omnisphere::omnidata::types::MakeSQLParam(logout.SessionUUID));

    database->BeginTransaction();
    database->RunPrepared(sQuery, vParams);
    database->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[CloseSession Exception] ") + " " +
                             e.what());
  }
};
} // namespace omnisphere::omnicore::repositories