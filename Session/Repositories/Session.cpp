#include "Session/Repositories/Session.hpp"

namespace omnisphere::repositories {
Session::Session(std::shared_ptr<omnisphere::data::DatabasePool> _database) {
  database = std::move(_database);
}

bool Session::Create(const omnisphere::dtos::Login &login) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery = "INSERT INTO \"Sessions\" ("
                         "\"SessionUUID\", ";

    if (login.Code.has_value())
      sQuery += "\"UserCode\", ";

    if (login.Email.has_value())
      sQuery += "\"UserEmail\", ";

    if (login.Phone.has_value())
      sQuery += "\"UserPhone\", ";

    sQuery += "\"StartDate\", "
              "\"DeviceIP\", "
              "\"HostName\" "
              ") VALUES ("
              "gen_random_uuid()::text, ";

    if (login.Code.has_value())
      sQuery += "?, ";

    if (login.Email.has_value())
      sQuery += "?, ";

    if (login.Phone.has_value())
      sQuery += "?, ";

    sQuery += "?, ?, ?)";

    std::vector<omnisphere::types::SQLParam> vParams;

    if (login.Code.has_value())
      vParams.emplace_back(omnisphere::types::MakeSQLParam(login.Code.value()));

    if (login.Email.has_value())
      vParams.emplace_back(
          omnisphere::types::MakeSQLParam(login.Email.value()));

    if (login.Phone.has_value())
      vParams.emplace_back(
          omnisphere::types::MakeSQLParam(login.Phone.value()));

    vParams.emplace_back(omnisphere::types::MakeSQLParam(login.StartDate));
    vParams.emplace_back(omnisphere::types::MakeSQLParam(login.DeviceIP));
    vParams.emplace_back(omnisphere::types::MakeSQLParam(login.HostName));

    conn->BeginTransaction();

    if (!conn->RunPrepared(sQuery, vParams))
      throw std::runtime_error("[RunPrepared exception]");

    conn->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    conn->RollbackTransaction();
    throw(std::runtime_error(std::string("[LoginException]") + e.what()));
  }
}

int Session::GetCurrentSequence() const {
  return 0;
}

bool Session::UpdateSessionSequence() const {
  return true;
}

omnisphere::types::DataTable
Session::Read(const omnisphere::dtos::Login &login) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery = "SELECT "
                         "T0.\"SessionEntry\", "
                         "T0.\"SessionUUID\", "
                         "T0.\"UserCode\", "
                         "T0.\"UserEmail\", "
                         "T0.\"UserPhone\", "
                         "T0.\"IsActive\", "
                         "T0.\"StartDate\", "
                         "T0.\"DeviceIP\", "
                         "T0.\"HostName\", "
                         "T0.\"EndDate\", "
                         "T0.\"DurationSeconds\" "
                         "FROM \"Sessions\" T0 "
                         "JOIN \"Users\" T1 ON ";

    std::vector<omnisphere::types::SQLParam> vParams;

    if (login.Code.has_value()) {
      sQuery += "T0.\"UserCode\" = T1.\"Code\" WHERE T1.\"Code\" = ? ";
      vParams.emplace_back(omnisphere::types::MakeSQLParam(login.Code.value()));
    }

    if (login.Email.has_value()) {
      sQuery += "T0.\"UserEmail\" = T1.\"Email\" WHERE T1.\"Email\" = ? ";
      vParams.emplace_back(
          omnisphere::types::MakeSQLParam(login.Email.value()));
    }

    if (login.Phone.has_value()) {
      sQuery += "T0.\"UserPhone\" = T1.\"Phone\" WHERE T1.\"Phone\" = ? ";
      vParams.emplace_back(
          omnisphere::types::MakeSQLParam(login.Phone.value()));
    }

    sQuery += "AND T0.\"DeviceIP\" = ? AND T0.\"HostName\" = ? AND T0.\"IsActive\" = 'Y'";
    vParams.emplace_back(omnisphere::types::MakeSQLParam(login.DeviceIP));
    vParams.emplace_back(omnisphere::types::MakeSQLParam(login.HostName));

    omnisphere::types::DataTable data =
        conn->FetchPrepared(sQuery, vParams);

    return data;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadSessionData Exception] ") + " " +
                             e.what());
  }
}

omnisphere::types::DataTable
Session::Read(const std::string &sessionUUID) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery =
        "SELECT \"SessionUUID\", \"StartDate\", \"EndDate\", \"DurationSeconds\", \"Reason\", "
        "\"LogoutMessage\" FROM \"Sessions\" WHERE \"SessionUUID\" = ?";

    return conn->FetchPrepared(sQuery, sessionUUID);
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ReadSessionData Exception] ") + " " +
                             e.what());
  }
}

omnisphere::types::DataTable
Session::ExistsUUID(const std::string &sessionUUID) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery =
        "SELECT COUNT(*) AS \"Total\" FROM \"Sessions\" WHERE \"SessionUUID\" = ?";

    types::DataTable data = conn->FetchPrepared(sQuery, sessionUUID);

    return data;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ValidateSessionUUID Exception] ") +
                             " " + e.what());
  }
}

omnisphere::types::DataTable
Session::IsActive(const std::string &sessionUUID) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery = "SELECT \"IsActive\" FROM \"Sessions\" WHERE \"SessionUUID\" = ?";

    types::DataTable data = conn->FetchPrepared(sQuery, sessionUUID);

    return data;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[IsSessionActive Exception] ") + " " +
                             e.what());
  }
}

bool Session::Close(const omnisphere::dtos::Logout &logout) const {
  auto conn = database->Acquire();
  try {
    std::string sQuery =
        "UPDATE \"Sessions\" SET \"IsActive\" = 'N', \"EndDate\" = ?, \"DurationSeconds\" = ? ";
    std::vector<omnisphere::types::SQLParam> vParams;

    if (logout.Message.has_value())
      sQuery += ", \"LogoutMessage\" = ? ";

    sQuery += ", \"Reason\" = ? WHERE \"SessionUUID\" = ? AND \"IsActive\" = 'Y'";

    vParams.emplace_back(omnisphere::types::MakeSQLParam(logout.EndDate));
    vParams.emplace_back(omnisphere::types::MakeSQLParam(3600));

    if (logout.Message.has_value())
      vParams.emplace_back(omnisphere::types::MakeSQLParam(logout.Message.value()));

    vParams.emplace_back(omnisphere::types::MakeSQLParam(static_cast<int>(logout.Reason)));
    vParams.emplace_back(omnisphere::types::MakeSQLParam(logout.SessionUUID));

    conn->BeginTransaction();
    if (!conn->RunPrepared(sQuery, vParams)) {
      conn->RollbackTransaction();
      return false;
    }

    conn->CommitTransaction();

    return true;
  } catch (const std::exception &e) {
    conn->RollbackTransaction();
    throw std::runtime_error(std::string("[CloseSession Exception] ") + " " +
                             e.what());
  }
}
} // namespace omnisphere::repositories