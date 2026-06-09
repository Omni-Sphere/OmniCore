#include <Database.hpp>
#include <DataTable.hpp>
#pragma once
#include <DataTable.hpp>
#include <Database.hpp>
#include <User/DTOs/CreateUser.hpp>
#include <User/DTOs/SearchUsers.hpp>
#include <User/DTOs/UpdateUser.hpp>
#include <User/Enums/UserFilter.hpp>
#include <User/Models/User.hpp>
#include <memory>

namespace omnisphere::repositories
{
    class User
    {
        private:
        std::shared_ptr<omnisphere::services::Database> database;
        int _UserEntry = -1;

        bool UpdateUserSequence() const;

        int GetCurrentSequence() const;

        public:
        explicit User(std::shared_ptr<omnisphere::services::Database> database);

        ~User() {};

        bool Create(const omnisphere::dtos::CreateUser &user) const;

        bool Update(const omnisphere::dtos::UpdateUser &user) const;

        omnisphere::types::DataTable
        Read(const omnisphere::dtos::SearchUsers &user) const;

        omnisphere::types::DataTable Read(const omnisphere::enums::UserFilter &filter,
                                          const std::string &value) const;

        bool ExistsEntry(const int &entry) const;

        bool ExistsCode(const std::string &code) const;

        bool UpdatePassword(const omnisphere::enums::UserFilter &filter,
                            const std::string &value, const std::string &oldPassword,
                            const std::string &newPassword) const;

        bool ValidatePassword(const omnisphere::enums::UserFilter &filter,
                              const std::string &value,
                              const std::string &password) const;
    };
} // namespace omnisphere::repositories