#include <OmniUtils/JWT.hpp>
#include <OmniData/DataTable.hpp>
#include <OmniData/DatabasePool.hpp>
#include "Session/Session.hpp"
#include <memory>
#include <optional>
#include <stdexcept>

#include "Session/Repositories/Session.hpp"
#include "User/Enums/UserFilter.hpp"
#include "User/Models/User.hpp"
#include "User/User.hpp"

namespace omnisphere::services {
struct Session::Impl {
  std::shared_ptr<omnisphere::repositories::Session> session;
  std::shared_ptr<omnisphere::services::User> user;

  explicit Impl(std::shared_ptr<omnisphere::data::DatabasePool> db)
      : session(std::make_shared<omnisphere::repositories::Session>(db)),
        user(std::make_shared<omnisphere::services::User>(db)) {}
};

Session::Session(std::shared_ptr<omnisphere::data::DatabasePool> db)
    : pimpl(std::make_unique<Impl>(db)) {}

Session::~Session() = default;

omnisphere::models::AuthPayload
Session::Login(const omnisphere::dtos::Login &login) const {
  try {
    if (login.Code.has_value() &&
        !pimpl->user->Exists(omnisphere::enums::UserFilter::Code,
                             login.Code.value()))
      throw std::runtime_error("User Code doesn't exists");

    if (login.Email.has_value() &&
        !pimpl->user->Exists(omnisphere::enums::UserFilter::Email,
                             login.Email.value()))
      throw std::runtime_error("User Email doesn't exists");

    if (login.Phone.has_value() &&
        !pimpl->user->Exists(omnisphere::enums::UserFilter::Phone,
                             login.Phone.value()))
      throw std::runtime_error("User Phone doesn't exists");

    omnisphere::models::User userModel = [&]() -> omnisphere::models::User {
      if (login.Code.has_value())
        return pimpl->user->Get(omnisphere::enums::UserFilter::Code,
                                login.Code.value());
      else if (login.Email.has_value())
        return pimpl->user->Get(omnisphere::enums::UserFilter::Email,
                                login.Email.value());
      else if (login.Phone.has_value())
        return pimpl->user->Get(omnisphere::enums::UserFilter::Phone,
                                login.Phone.value());
      throw std::runtime_error("No login credential provided");
    }();

    if (userModel.IsLocked)
      throw std::runtime_error("Account is locked");

    if (login.Code.has_value() &&
        !pimpl->user->CheckPassword(omnisphere::enums::UserFilter::Code,
                                    login.Code.value(), login.Password))
      throw std::runtime_error("Wrong password");

    if (login.Email.has_value() &&
        !pimpl->user->CheckPassword(omnisphere::enums::UserFilter::Email,
                                    login.Email.value(), login.Password))
      throw std::runtime_error("Wrong password");

    if (login.Phone.has_value() &&
        !pimpl->user->CheckPassword(omnisphere::enums::UserFilter::Phone,
                                    login.Phone.value(), login.Password))
      throw std::runtime_error("Wrong password");

    pimpl->session->Create(login);

    omnisphere::types::DataTable data;
    omnisphere::models::AuthPayload authPayload;

    data = pimpl->session->Read(login);

    authPayload.SessionUUID = std::string(data[0]["SessionUUID"]);

    boost::json::object payload;
    payload["SessionUUID"] = authPayload.SessionUUID;
    if (data.RowsCount() > 0 && data[0].HasColumn("UserCode") && !data[0]["UserCode"].IsNull()) {
      std::string uCode = std::string(data[0]["UserCode"]);
      payload["UserCode"] = uCode;
      payload["userCode"] = uCode;
      payload["sub"] = uCode;
    }

    authPayload.AccessToken =
        omnisphere::utils::JWT::GenerateToken(payload, 86400);

    if (login.Code.has_value())
      authPayload.User =
          std::make_shared<omnisphere::models::User>(pimpl->user->Get(
              omnisphere::enums::UserFilter::Code, data[0]["UserCode"]));

    if (login.Email.has_value())
      authPayload.User =
          std::make_shared<omnisphere::models::User>(pimpl->user->Get(
              omnisphere::enums::UserFilter::Email, data[0]["UserEmail"]));

    if (login.Phone.has_value())
      authPayload.User =
          std::make_shared<omnisphere::models::User>(pimpl->user->Get(
              omnisphere::enums::UserFilter::Phone, data[0]["UserPhone"]));

    return authPayload;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[Login Exception] ") + e.what());
  }
}

bool Session::Active(const std::string &tokenOrUUID) const {
  try {
    std::string sessionUUID = tokenOrUUID;
    if (tokenOrUUID.find('.') != std::string::npos) {
      boost::json::object payload = omnisphere::utils::JWT::ValidateToken(tokenOrUUID);
      if (payload.contains("SessionUUID") && payload.at("SessionUUID").is_string()) {
        sessionUUID = std::string(payload.at("SessionUUID").as_string());
      }
    }

    omnisphere::types::DataTable data = pimpl->session->IsActive(sessionUUID);

    if (data.RowsCount() == 0)
      return false;

    std::string activeStr = std::string(data[0]["IsActive"]);
    return (activeStr == "Y" || activeStr == "true" || activeStr == "1");
  } catch (const std::exception &) {
    return false;
  }
}

bool Session::Exists(const std::string &sessionUUID) const {
  try {
    omnisphere::types::DataTable data = pimpl->session->ExistsUUID(sessionUUID);

    const int sessionCount = data[0]["Total"];

    if (data.RowsCount() == 0)
      return false;

    if (sessionCount == 0)
      return false;

    if (sessionCount > 1)
      throw std::runtime_error(std::string(
          "Inconsistencies in session UUID, found more than one session"));

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ExistsSession Exception] ") +
                             e.what());
  }
}

omnisphere::models::LogoutPayload
Session::Logout(const omnisphere::dtos::Logout &logout) const {
  try {
    if (!Exists(logout.SessionUUID))
      throw std::runtime_error("Session UUID doesn't exists");

    if (!Active(logout.SessionUUID))
      throw std::runtime_error("Session is not active");

    if (!pimpl->session->Close(logout))
      throw std::runtime_error("Session could not be closed.");

    omnisphere::types::DataTable data =
        pimpl->session->Read(logout.SessionUUID);

    omnisphere::models::LogoutPayload logoutModel;
    logoutModel.SessionUUID = std::string(data[0]["SessionUUID"]);
    logoutModel.StartDate = std::string(data[0]["StartDate"]);
    logoutModel.EndDate = std::string(data[0]["EndDate"]);
    logoutModel.Duration = data[0]["DurationSeconds"];
    logoutModel.Reason = static_cast<omnisphere::enums::LogoutReason>(
        static_cast<int>(data[0]["Reason"]));
    logoutModel.Message = data[0]["LogoutMessage"].GetOptional<std::string>();

    return logoutModel;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[LogoutDTO Exception] ") + e.what());
  }
}

} // namespace omnisphere::services