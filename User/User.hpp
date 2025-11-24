#pragma once

#include "Database.hpp"
#include "Repositories/User.hpp"

#include "DTOs/CreateUser.hpp"
#include "DTOs/UpdateUser.hpp"
#include "DTOs/ChangePassword.hpp"
#include "DTOs/SearchUsers.hpp"
#include "Models/User.hpp"
#include "Enums/UserFilter.hpp"

namespace omnicore::service
{
    /**
     * @brief Service class for managing user accounts
     * 
     * This service provides methods for user CRUD operations, authentication,
     * and account management.
     */
    class User {
    public: 
        /**
         * @brief Constructs a User service
         * @param database Shared pointer to the database service
         */
        explicit User(std::shared_ptr<service::Database> database);
        
        /**
         * @brief Default destructor
         */
        ~User()  = default; 
        
        /**
         * @brief Creates a new user account
         * @param user DTO containing user creation data
         * @return true if user was created successfully, false otherwise
         */
        bool Add(const dto::CreateUser &user) const ;
        
        /**
         * @brief Updates an existing user account
         * @param user DTO containing user update data
         * @return Updated user model
         */
        model::User Modify(const dto::UpdateUser &user) const ;
        
        /**
         * @brief Changes a user's password
         * @param changePassword DTO containing password change data
         * @return true if password was changed successfully, false otherwise
         */
        bool ModifyPassword(const dto::ChangePassword &) const ;
        
        /**
         * @brief Validates a user's password
         * @param filter Filter type to identify the user
         * @param oldPassword Current password to validate
         * @param newPassword New password (for validation context)
         * @return true if password is valid, false otherwise
         */
        bool CheckPassword(const enums::UserFilter &filter, const std::string &oldPassword, const std::string &newPassword) const ;
        
        /**
         * @brief Locks or unlocks a user account
         * @param filter Filter type to identify the user
         * @param value Filter value (e.g., user code, email)
         * @param lock true to lock the account, false to unlock
         * @return true if operation was successful, false otherwise
         */
        bool LockUnlockUser(const enums::UserFilter &, const std::string &value, const bool &lock) const ;
        
        /**
         * @brief Searches for users based on criteria
         * @param user DTO containing search criteria
         * @return Vector of user models matching the criteria
         */
        std::vector<model::User> Search(const dto::SearchUsers &user) const ;
        
        /**
         * @brief Retrieves a specific user
         * @param filter Filter type to identify the user
         * @param value Filter value (e.g., user code, email)
         * @return User model
         */
        model::User Get(const enums::UserFilter &filter, const std::string&value) const ;
        
        /**
         * @brief Checks if a user exists
         * @param filter Filter type to identify the user
         * @param value Filter value (e.g., user code, email)
         * @return true if user exists, false otherwise
         */
        bool Exists(const enums::UserFilter &filter, const std::string &value) const ;

    private:
        std::shared_ptr<repository::User> user; ///< Repository instance for data access
};
}