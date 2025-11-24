#include "Datatable.hpp"
#include <stdexcept>
#include <iostream>

namespace omnicore::type
{

    void Datatable::SetColumns(const std::vector<std::string> &cols)
    {
        columns = cols;
    }

    void Datatable::Fill(const std::vector<Row> &newData)
    {
        data.insert(data.end(), newData.begin(), newData.end());
    }

    Datatable::Row &Datatable::operator[](int index)
    {
        if (index < 0 || index >= static_cast<int>(data.size()))
        {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

    const Datatable::Row &Datatable::operator[](int index) const
    {
        if (index < 0 || index >= static_cast<int>(data.size()))
        {
            throw std::out_of_range("Index out of range");
        }
        return data[index];
    }

    int Datatable::RowsCount() const
    {
        return static_cast<int>(data.size());
    }

    void Datatable::Row::Set(const std::string &column, const Value &value)
    {
        values[column] = value;
    }

    void Datatable::Row::SetColumns(const std::vector<std::string> &columns)
    {
        for (const auto &col : columns)
        {
            if (values.find(col) == values.end())
            {
                values[col] = std::nullopt;
            }
        }
    }

    Datatable::Row::ValueProxy Datatable::Row::operator[](const std::string &column)
    {
        auto it = values.find(column);
        if (it == values.end())
        {
            throw std::runtime_error("The " + column + " column does not belong to the table");
        }
        return ValueProxy(&it->second);
    }

    const Datatable::Row::Value &Datatable::Row::operator[](const std::string &column) const
    {
        auto it = values.find(column);
        if (it == values.end())
        {
            throw std::runtime_error("The" + column + " column does not belong to the table");
        }
        return it->second;
    }
}