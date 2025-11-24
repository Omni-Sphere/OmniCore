#pragma once
#include <memory>
#include "Database.hpp"
#include "Models/User.hpp"
#include "Enums/UserFilter.hpp"
#include "DTOs/SearchUsers.hpp"
#include "DTOs/CreateUser.hpp"
#include "DTOs/UpdateUser.hpp"

namespace omnicore::repository
{
    /**
     * @brief Repository class for User data access
     * 
     * This repository handles all database operations related to the Users table.
     */
    class User
    {
    private:
        std::shared_ptr<service::Database> database; ///< Database service instance
        int _UserEntry = -1; ///< Current user entry ID

        /**
         * @brief Updates the user sequence counter in the database
         * @return true if successful, false otherwise
         */
        bool UpdateUserSequence() const;
        
        /**
         * @brief Gets the current user sequence value
         * @return Current sequence number
         */
        int GetCurrentSequence() const;

    public:
        /**
         * @brief Constructs a User repository
         * @param database Shared pointer to the database service
         */
        explicit User(std::shared_ptr<service::Database> database);
        
        /**
         * @brief Destructor
         */
        ~User() {};

        /**
         * @brief Creates a new user in the database
         * @param user DTO containing user creation data
         * @return true if successful, false otherwise
         * @throws std::runtime_error if creation fails
         */
        bool Create(const dto::CreateUser &user) const;
        
        /**
         * @brief Updates an existing user in the database
         * @param user DTO containing user update data
         * @return true if successful, false otherwise
         * @throws std::runtime_error if update fails
         */
        bool Update(const dto::UpdateUser &user) const;

        /**
         * @brief Searches for users based on criteria
         * @param user DTO containing search criteria
         * @return Datatable with matching users
         * @throws std::runtime_error if query fails
         */
        type::Datatable Read(const dto::SearchUsers &user) const;
        
        /**
         * @brief Reads a specific user from the database
         * @param filter Filter type to identify the user
         * @param value Filter value (e.g., user code, email)
         * @return Datatable with user data
         * @throws std::runtime_error if query fails
         */
        type::Datatable Read(const enums::UserFilter &filter, const std::string &value) const;

        /**
         * @brief Checks if a user exists by entry ID
         * @param entry User entry ID
         * @return true if user exists, false otherwise
         */
        bool ExistsEntry(const int &entry) const;
        
        /**
         * @brief Checks if a user exists by code
         * @param code User code
         * @return true if user exists, false otherwise
         */
        bool ExistsCode(const std::string &code) const;
        
        /**
         * @brief Updates a user's password
         * @param filter Filter type to identify the user
         * @param value Filter value (e.g., user code)
         * @param oldPassword Current password (for validation)
         * @param newPassword New password to set
         * @return true if successful, false otherwise
         * @throws std::runtime_error if update fails
         */
        bool UpdatePassword(const enums::UserFilter &filter, const std::string &value, const std::string &oldPassword, const std::string &newPassword) const;
        
        /**
         * @brief Validates a user's password
         * @param filter Filter type to identify the user
         * @param value Filter value (e.g., user code)
         * @param password Password to validate
         * @return true if password is valid, false otherwise
         * @throws std::runtime_error if validation fails
         */
        bool ValidatePassword(const enums::UserFilter &filter, const std::string &value, const std::string &password) const;
    };
}