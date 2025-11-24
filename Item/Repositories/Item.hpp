#pragma once

#include <string>
#include <optional>

#include "Database.hpp"
#include "DTOs/CreateItem.hpp"
#include "DTOs/UpdateItem.hpp"
#include "DTOs/SearchItems.hpp"
#include "DTOs/GetItem.hpp"

namespace omnicore::repository
{
    class Item
    {
    public:
        explicit Item(std::shared_ptr<service::Database> database);
        ~Item();

        bool Create(const dto::CreateItem &_item) const;
        bool Update(const dto::UpdateItem &_item) const;
        type::Datatable Read(const dto::SearchItems &_item) const;
        type::Datatable Read(const dto::GetItem &_item) const;
        type::Datatable Read() const;


    private:
        std::shared_ptr<service::Database> Database;

        int GetCurrentSequence() const;
        bool UpdateUserSequence() const;

    };
}