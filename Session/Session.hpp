#pragma once

#include "Database.hpp"
#include "User.hpp"
#include "Repositories/Session.hpp"

#include "Models/AuthPayload.hpp"
#include "Models/LogoutPayload.hpp"

#include "DTOs/Login.hpp"
#include "DTOs/Logout.hpp"

namespace omnicore::service
{
    /**
     * @brief Service class for managing user sessions
     * 
     * This service handles user authentication, session creation,
     * and session validation.
     */
    class Session
    {
    public:
        /**
         * @brief Constructs a Session service
         * @param database Shared pointer to the database service
         */
        explicit Session(std::shared_ptr<service::Database> database);
        
        /**
         * @brief Destructor
         */
        ~Session();

        /**
         * @brief Authenticates a user and creates a session
         * @param login DTO containing login credentials
         * @return AuthPayload with session token and user information
         * @throws std::runtime_error if authentication fails
         */
        model::AuthPayload Login(const dto::Login &login) const;
        
        /**
         * @brief Logs out a user and invalidates the session
         * @param logout DTO containing logout information
         * @return LogoutPayload with logout status
         */
        model::LogoutPayload Logout(const dto::Logout &logout) const;
        
        /**
         * @brief Checks if a session exists
         * @param token Session token to check
         * @return true if session exists, false otherwise
         */
        bool Exists(const std::string &token) const;
        
        /**
         * @brief Checks if a session is active
         * @param token Session token to check
         * @return true if session is active, false otherwise
         */
        bool Active(const std::string &token) const;

    private:
        std::shared_ptr<repository::Session> session; ///< Repository instance for session data access
        std::shared_ptr<service::User> user; ///< User service instance

    };
}