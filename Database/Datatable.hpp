#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <variant>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

#ifdef __GNUG__
#include <cxxabi.h>
#include <memory>
#endif

namespace omnicore::type
{
    class Datatable
    {
    public:
        class Row
        {
        public:
            using VariantType = std::variant<int, double, bool, std::string, std::vector<uint8_t>>;
            using Value = std::optional<VariantType>;

            class ValueProxy
            {
            public:
                explicit ValueProxy(Value *valuePtr) : valuePtr_(valuePtr) {}

                // 🔹 Demangle para mostrar tipos legibles
                static std::string demangle(const char *name)
                {
#ifdef __GNUG__
                    int status = 0;
                    std::unique_ptr<char, void (*)(void *)> demangled(
                        abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free);
                    return (status == 0 && demangled) ? demangled.get() : name;
#else
                    return name;
#endif
                }

                static std::string getVariantTypeName(const VariantType &v)
                {
                    if (std::holds_alternative<int>(v))
                        return "int";
                    if (std::holds_alternative<double>(v))
                        return "double";
                    if (std::holds_alternative<bool>(v))
                        return "bool";
                    if (std::holds_alternative<std::string>(v))
                        return "std::string";
                    if (std::holds_alternative<std::vector<uint8_t>>(v))
                        return "std::vector<uint8_t>";
                    return "unknown";
                }

                template <typename T>
                std::optional<T> AsDBNull() const
                {
                    if (!valuePtr_ || !valuePtr_->has_value())
                        return std::nullopt;

                    if (auto val = std::get_if<T>(&valuePtr_->value()))
                        return *val;

                    const auto &v = valuePtr_->value();
                    throw std::runtime_error(
                        "Error parsing value to type '" + demangle(typeid(T).name()) +
                        "' (actual type: " + getVariantTypeName(v) + ")");
                }

                template <typename T>
                operator std::optional<T>() const
                {
                    if (!valuePtr_ || !valuePtr_->has_value())
                        return std::nullopt;

                    if (auto val = std::get_if<T>(&valuePtr_->value()))
                        return *val;

                    const auto &v = valuePtr_->value();
                    throw std::runtime_error(
                        "Error parsing optional value to type '" + demangle(typeid(T).name()) +
                        "' (actual type: " + getVariantTypeName(v) + ")");
                }

                template <typename T>
                operator T() const
                {
                    if (!valuePtr_ || !valuePtr_->has_value())
                        throw std::runtime_error("Value is null");

                    if (auto val = std::get_if<T>(&valuePtr_->value()))
                        return *val;

                    // Conversión automática int <-> double
                    if constexpr (std::is_same_v<T, double>)
                    {
                        if (auto ival = std::get_if<int>(&valuePtr_->value()))
                            return static_cast<double>(*ival);
                    }
                    else if constexpr (std::is_same_v<T, int>)
                    {
                        if (auto dval = std::get_if<double>(&valuePtr_->value()))
                            return static_cast<int>(*dval);
                    }

                    const auto &v = valuePtr_->value();
                    throw std::runtime_error(
                        "Error parsing value to type '" + demangle(typeid(T).name()) +
                        "' (actual type: " + getVariantTypeName(v) + ")");
                }

                operator bool() const
                {
                    if (!valuePtr_ || !valuePtr_->has_value())
                        throw std::runtime_error("Value is null");

                    if (auto val = std::get_if<std::string>(&valuePtr_->value()))
                    {
                        if (*val == "Y" || *val == "y" || *val == "1")
                            return true;
                        if (*val == "N" || *val == "n" || *val == "0")
                            return false;
                        throw std::runtime_error("Invalid char value for bool conversion: " + *val);
                    }

                    if (auto val = std::get_if<int>(&valuePtr_->value()))
                        return (*val != 0);

                    if (auto val = std::get_if<double>(&valuePtr_->value()))
                        return (*val != 0.0);

                    if (auto val = std::get_if<bool>(&valuePtr_->value()))
                        return *val;

                    const auto &v = valuePtr_->value();
                    throw std::runtime_error(
                        "Unsupported type for bool conversion (actual type: " + getVariantTypeName(v) + ")");
                }

                template <typename EnumType>
                operator EnumType() const
                    requires std::is_enum_v<EnumType>
                {
                    if (!valuePtr_ || !valuePtr_->has_value())
                        return static_cast<EnumType>(-1);

                    if (auto val = std::get_if<int>(&valuePtr_->value()))
                        return static_cast<EnumType>(*val);

                    if (auto dval = std::get_if<double>(&valuePtr_->value()))
                        return static_cast<EnumType>(static_cast<int>(*dval));

                    if (auto sval = std::get_if<std::string>(&valuePtr_->value()))
                    {
                        try
                        {
                            int intval = std::stoi(*sval);
                            return static_cast<EnumType>(intval);
                        }
                        catch (...)
                        {
                            return static_cast<EnumType>(-1);
                        }
                    }

                    return static_cast<EnumType>(-1);
                }

                bool IsNull() const
                {
                    return !valuePtr_ || !valuePtr_->has_value();
                }

            private:
                Value *valuePtr_;
            };

            void Set(const std::string &column, const Value &value);
            void SetColumns(const std::vector<std::string> &columns);

            ValueProxy operator[](const std::string &column);
            const Value &operator[](const std::string &column) const;

        private:
            std::unordered_map<std::string, Value> values;
        };

        void SetColumns(const std::vector<std::string> &cols);
        void Fill(const std::vector<Row> &newData);
        Row &operator[](int index);
        const Row &operator[](int index) const;
        int RowsCount() const;

    private:
        std::vector<std::string> columns;
        std::vector<Row> data;
    };
}
