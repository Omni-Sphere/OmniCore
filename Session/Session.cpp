#include "Session.hpp"
#include "JWT.hpp"
#include <memory>
#include <stdexcept>

#include "DataTable.hpp"
#include "Enums/UserFilter.hpp"
#include "Models/User.hpp"
#include "User.hpp"

namespace omnicore::service {

struct Session::Impl {
  std::shared_ptr<repository::Session> session;
  std::shared_ptr<service::User> user;

  explicit Impl(std::shared_ptr<service::Database> db)
      : session(std::make_shared<repository::Session>(db)),
        user(std::make_shared<service::User>(db)) {}
};

Session::Session(std::shared_ptr<service::Database> db)
    : pimpl(std::make_unique<Impl>(db)) {}

Session::~Session() = default;

model::AuthPayload Session::Login(const dto::Login &login) const {
  try {
    if (login.Code.has_value() &&
        !pimpl->user->Exists(enums::UserFilter::Code, login.Code.value()))
      throw std::runtime_error("User Code doesn't exists");

    if (login.Email.has_value() &&
        !pimpl->user->Exists(enums::UserFilter::Email, login.Email.value()))
      throw std::runtime_error("User Email doesn't exists");

    if (login.Phone.has_value() &&
        !pimpl->user->Exists(enums::UserFilter::Phone, login.Phone.value()))
      throw std::runtime_error("User Phone doesn't exists");

    // Add lockout check
    model::User userModel;
    if (login.Code.has_value())
      userModel = pimpl->user->Get(enums::UserFilter::Code, login.Code.value());
    else if (login.Email.has_value())
      userModel =
          pimpl->user->Get(enums::UserFilter::Email, login.Email.value());
    else if (login.Phone.has_value())
      userModel =
          pimpl->user->Get(enums::UserFilter::Phone, login.Phone.value());

    if (userModel.IsLocked)
      throw std::runtime_error("Account is locked");

    if (login.Code.has_value() &&
        !pimpl->user->CheckPassword(enums::UserFilter::Code, login.Code.value(),
                                    login.Password))
      throw std::runtime_error("Wrong password");

    if (login.Email.has_value() &&
        !pimpl->user->CheckPassword(enums::UserFilter::Email,
                                    login.Email.value(), login.Password))
      throw std::runtime_error("Wrong password");

    if (login.Phone.has_value() &&
        !pimpl->user->CheckPassword(enums::UserFilter::Phone,
                                    login.Phone.value(), login.Password))
      throw std::runtime_error("Wrong password");

    pimpl->session->Create(login);

    type::DataTable data;
    model::AuthPayload authPayload;

    data = pimpl->session->Read(login);

    authPayload.SessionUUID = std::string(data[0]["SessionUUID"]);

    boost::json::object payload;
    payload["SessionUUID"] = authPayload.SessionUUID;

    authPayload.AccessToken = JWT::GenerateToken(payload, 86400);

    if (login.Code.has_value())
      authPayload.User = std::make_shared<model::User>(
          pimpl->user->Get(enums::UserFilter::Code, data[0]["UserCode"]));

    if (login.Email.has_value())
      authPayload.User = std::make_shared<model::User>(
          pimpl->user->Get(enums::UserFilter::Email, data[0]["UserEmail"]));

    if (login.Phone.has_value())
      authPayload.User = std::make_shared<model::User>(
          pimpl->user->Get(enums::UserFilter::Phone, data[0]["UserPhone"]));

    return authPayload;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[Login Exception] ") + e.what());
  }
};

bool Session::Active(const std::string &token) const {
  try {
    boost::json::object payload = JWT::ValidateToken(token);

    std::string sessionUUID = payload["SessionUUID"].as_string().c_str();

    type::DataTable data = pimpl->session->IsActive(sessionUUID);

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
    type::DataTable data = pimpl->session->ExistsUUID(sessionUUID);

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

model::LogoutPayload Session::Logout(const dto::Logout &logout) const {
  try {
    if (!Exists(logout.SessionUUID))
      throw std::runtime_error("Session UUID doesn't exists");

    if (!Active(logout.SessionUUID))
      throw std::runtime_error("Session is not active");

    if (!pimpl->session->Close(logout))
      throw std::runtime_error("Session could not be closed.");

    type::DataTable data = pimpl->session->Read(logout.SessionUUID);

    model::LogoutPayload logoutModel{
        data[0]["SessionUUID"],
        data[0]["StartDate"],
        data[0]["EndDate"],
        data[0]["DurationSeconds"],
        data[0]["Reason"],
        data[0]["LogoutMessage"].GetOptional<std::string>()};

    return logoutModel;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[LogoutDTO Exception] ") + e.what());
  }
}

} // namespace omnicore::service