#pragma once

#include "Database.hpp"
#include "DTOs/Login.hpp"
#include "DTOs/Logout.hpp"

namespace omnicore::repository
{
    class Session
    {
    private:
        std::shared_ptr<service::Database> database;
        int GetCurrentSequence() const;
        bool UpdateSessionSequence() const;

    public:
        explicit Session(std::shared_ptr<service::Database> Database);
        ~Session() {};

        bool Create(const dto::Login &login) const;
        bool Close(const dto::Logout &logout) const;
        type::Datatable ExistsUUID(const std::string &sessionUUID) const;
        type::Datatable Read(const std::string &) const;
        type::Datatable Read(const dto::Login &) const;
        type::Datatable IsActive(const std::string &) const;
    };
}