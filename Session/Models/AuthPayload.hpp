#pragma once

#include <iostream>
#include <memory>
#include "Models/User.hpp"

namespace omnicore::model
{
    class AuthPayload
    {
        public:
            std::string AccessToken;
            std::string SessionUUID;
            std::shared_ptr<omnicore::model::User> User;
    };
}