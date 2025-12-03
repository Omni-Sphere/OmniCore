#include "Session.hpp"
#include "JWT.hpp"

namespace omnicore::service {
Session::Session(std::shared_ptr<service::Database> db) {
  session = std::make_shared<repository::Session>(db);
  user = std::make_shared<service::User>(db);
}

Session::~Session() = default;

model::AuthPayload Session::Login(const dto::Login &login) const {
  try {
    if (login.Code.has_value() &&
        !user->Exists(enums::UserFilter::Code, login.Code.value()))
      throw std::runtime_error("User Code doesn't exists");

    if (login.Email.has_value() &&
        !user->Exists(enums::UserFilter::Email, login.Email.value()))
      throw std::runtime_error("User Email doesn't exists");

    if (login.Phone.has_value() &&
        !user->Exists(enums::UserFilter::Phone, login.Phone.value()))
      throw std::runtime_error("User Phone doesn't exists");

    if (login.Code.has_value() &&
        !user->CheckPassword(enums::UserFilter::Code, login.Code.value(),
                             login.Password))
      throw std::runtime_error("Wrong password");

    if (login.Email.has_value() &&
        !user->CheckPassword(enums::UserFilter::Email, login.Email.value(),
                             login.Password))
      throw std::runtime_error("Wrong password");

    if (login.Phone.has_value() &&
        !user->CheckPassword(enums::UserFilter::Phone, login.Phone.value(),
                             login.Password))
      throw std::runtime_error("Wrong password");

    session->Create(login);

    type::Datatable data;
    model::AuthPayload authPayload;

    data = session->Read(login);

    authPayload.SessionUUID = std::string(data[0]["SessionUUID"]);
    authPayload.AccessToken = JWT::GenerateToken(data[0]["SessionUUID"], 86400);

    if (login.Code.has_value())
      authPayload.User = std::make_shared<model::User>(
          user->Get(enums::UserFilter::Code, data[0]["UserCode"]));

    if (login.Email.has_value())
      authPayload.User = std::make_shared<model::User>(
          user->Get(enums::UserFilter::Email, data[0]["UserEmail"]));

    if (login.Phone.has_value())
      authPayload.User = std::make_shared<model::User>(
          user->Get(enums::UserFilter::Phone, data[0]["UserPhone"]));

    return authPayload;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[Login Exception] ") + e.what());
  }
};

bool Session::Active(const std::string &sessionUUID) const {
  try {
    type::Datatable data = session->IsActive(sessionUUID);

    const bool sessionActive = data[0]["IsActive"];

    if (data.RowsCount() == 0)
      throw std::runtime_error("Session doesn't exists");

    if (!sessionActive)
      return false;

    return true;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[ActiveSession Exception] ") +
                             e.what());
  }
}

bool Session::Exists(const std::string &sessionUUID) const {
  try {
    type::Datatable data = session->ExistsUUID(sessionUUID);

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

    if (!session->Close(logout))
      throw std::runtime_error("Session could not be closed.");

    type::Datatable data = session->Read(logout.SessionUUID);

    model::LogoutPayload logoutModel{
        data[0]["SessionUUID"],
        data[0]["StartDate"],
        data[0]["EndDate"],
        data[0]["DurationSeconds"],
        data[0]["Reason"],
        data[0]["LogoutMessage"].AsDBNull<std::string>()};

    return logoutModel;
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("[LogoutDTO Exception] ") + e.what());
  }
}

} // namespace omnicore::service