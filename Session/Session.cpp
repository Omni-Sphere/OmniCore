#include "Session.hpp"
#include "../../OmniUtils/Include/JWT.hpp"
#include <memory>
#include <optional>
#include <stdexcept>

#include "../OmniData/Include/DataTable.hpp"
#include "../User/Enums/UserFilter.hpp"
#include "../User/Models/User.hpp"
#include "../User/User.hpp"
#include "Repositories/Session.hpp"

namespace omnisphere::omnicore::services {

struct Session::Impl {
  std::shared_ptr<omnisphere::omnicore::repositories::Session> session;
  std::shared_ptr<omnisphere::omnicore::services::User> user;

  explicit Impl(std::shared_ptr<omnidata::services::Database> db)
      : session(
            std::make_shared<omnisphere::omnicore::repositories::Session>(db)),
        user(std::make_shared<omnisphere::omnicore::services::User>(db)) {}
};

Session::Session(std::shared_ptr<omnidata::services::Database> db)
    : pimpl(std::make_unique<Impl>(db)) {}

Session::~Session() = default;

omnisphere::omnicore::models::AuthPayload
Session::Login(const omnisphere::omnicore::dtos::Login &login) const {
  try {
    if (login.Code.has_value() &&
        !pimpl->user->Exists(omnisphere::omnicore::enums::UserFilter::Code,
                             login.Code.value()))
      throw std::runtime_error("User Code doesn't exists");

    if (login.Email.has_value() &&
        !pimpl->user->Exists(omnisphere::omnicore::enums::UserFilter::Email,
                             login.Email.value()))
      throw std::runtime_error("User Email doesn't exists");

    if (login.Phone.has_value() &&
        !pimpl->user->Exists(omnisphere::omnicore::enums::UserFilter::Phone,
                             login.Phone.value()))
      throw std::runtime_error("User Phone doesn't exists");

    // Add lockout check

    omnisphere::omnicore::models::User userModel =
        [&]() -> omnisphere::omnicore::models::User {
      if (login.Code.has_value())
        return pimpl->user->Get(omnisphere::omnicore::enums::UserFilter::Code,
                                login.Code.value());
      else if (login.Email.has_value())
        return pimpl->user->Get(omnisphere::omnicore::enums::UserFilter::Email,
                                login.Email.value());
      else if (login.Phone.has_value())
        return pimpl->user->Get(omnisphere::omnicore::enums::UserFilter::Phone,
                                login.Phone.value());
      throw std::runtime_error("No login credential provided");
    }();

    if (userModel.IsLocked)
      throw std::runtime_error("Account is locked");

    if (login.Code.has_value() &&
        !pimpl->user->CheckPassword(
            omnisphere::omnicore::enums::UserFilter::Code, login.Code.value(),
            login.Password))
      throw std::runtime_error("Wrong password");

    if (login.Email.has_value() &&
        !pimpl->user->CheckPassword(
            omnisphere::omnicore::enums::UserFilter::Email, login.Email.value(),
            login.Password))
      throw std::runtime_error("Wrong password");

    if (login.Phone.has_value() &&
        !pimpl->user->CheckPassword(
            omnisphere::omnicore::enums::UserFilter::Phone, login.Phone.value(),
            login.Password))
      throw std::runtime_error("Wrong password");

    pimpl->session->Create(login);

    omnidata::types::DataTable data;
    omnisphere::omnicore::models::AuthPayload authPayload;

    data = pimpl->session->Read(login);

    authPayload.SessionUUID = std::string(data[0]["SessionUUID"]);

    boost::json::object payload;
    payload["SessionUUID"] = authPayload.SessionUUID;

    authPayload.AccessToken =
        omnisphere::utils::JWT::GenerateToken(payload, 86400);

    if (login.Code.has_value())
      authPayload.User = std::make_shared<omnisphere::omnicore::models::User>(
          pimpl->user->Get(omnisphere::omnicore::enums::UserFilter::Code,
                           data[0]["UserCode"]));

    if (login.Email.has_value())
      authPayload.User = std::make_shared<omnisphere::omnicore::models::User>(
          pimpl->user->Get(omnisphere::omnicore::enums::UserFilter::Email,
                           data[0]["UserEmail"]));

    if (login.Phone.has_value())
      authPayload.User = std::make_shared<omnisphere::omnicore::models::User>(
          pimpl->user->Get(omnisphere::omnicore::enums::UserFilter::Phone,
                           data[0]["UserPhone"]));

    return authPayload;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[Login Exception] ") + e.what());
  }
};

bool Session::Active(const std::string &token) const {
  try {
    boost::json::object payload = omnisphere::utils::JWT::ValidateToken(token);

    std::string sessionUUID = payload["SessionUUID"].as_string().c_str();

    omnidata::types::DataTable data = pimpl->session->IsActive(sessionUUID);

    if (data.RowsCount() == 0)
      return false;

    const bool sessionActive = data[0]["IsActive"];

    return sessionActive;
  } catch (const std::exception &) {
    return false;
  }
}

bool Session::Exists(const std::string &sessionUUID) const {
  try {
    omnidata::types::DataTable data = pimpl->session->ExistsUUID(sessionUUID);

    const int sessionCount = data[0]["Total"];

    if (data.RowsCount() == 0)
      return false;

    if (sessionCount == 0)
      return false;

    if (sessionCount > 1)
      throw std::runtime_error(std::string(
          "Inconsistences in sesion UUID, found more than one session"));

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ExistsSession Exception] ") +
                             e.what());
  }
}

omnisphere::omnicore::models::LogoutPayload
Session::Logout(const omnisphere::omnicore::dtos::Logout &logout) const {
  try {
    if (!Exists(logout.SessionUUID))
      throw std::runtime_error("Session UUID doesn't exists");

    if (!Active(logout.SessionUUID))
      throw std::runtime_error("Session is not active");

    if (!pimpl->session->Close(logout))
      throw std::runtime_error("Session could not be closed.");

    omnidata::types::DataTable data = pimpl->session->Read(logout.SessionUUID);

    omnisphere::omnicore::models::LogoutPayload logoutModel;
    logoutModel.SessionUUID = std::string(data[0]["SessionUUID"]);
    logoutModel.StartDate = std::string(data[0]["StartDate"]);
    logoutModel.EndDate = std::string(data[0]["EndDate"]);
    logoutModel.Duration = data[0]["DurationSeconds"];
    logoutModel.Reason = static_cast<omnisphere::omnicore::enums::LogoutReason>(
        static_cast<int>(data[0]["Reason"])); // Casting check might be needed
    logoutModel.Message = data[0]["LogoutMessage"].GetOptional<std::string>();

    return logoutModel;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[LogoutDTO Exception] ") + e.what());
  }
}

} // namespace omnisphere::omnicore::services