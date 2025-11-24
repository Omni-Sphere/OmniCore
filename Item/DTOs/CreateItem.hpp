#pragma once

#include <string>
#include <optional>
#include <regex>
#include <stdexcept>
#include "BaseCreateDTO.hpp"

namespace omnicore::dto {

    struct CreateItem : public BaseCreateDTO
    {
        using BaseCreateDTO::BaseCreateDTO;

        CreateItem(
            std::string _Code,
            std::string _Name,
            std::optional<std::string> _Description,
            std::optional<std::string> _Image,
            bool _PurchaseItem,
            bool _SellItem,
            bool _InventoryItem,
            double _Price,
            std::optional<int> _Brand,
            std::optional<int> _Group,
            std::optional<double> _MinStock,
            std::optional<double> _MaxStock,
            std::optional<double> _MinOrder,
            std::optional<double> _MaxOrder,
            std::optional<double> _MinRequest,
            std::optional<double> _MaxRequest,
            int _CreatedBy,
            std::string _CreateDate)
            :
            BaseCreateDTO(
                std::move(_Code),
                std::move(_Name),
                _CreatedBy,
                std::move(_CreateDate)
            ),
            Description(std::move(_Description)),
            Image(std::move(_Image)),
            PurchaseItem(_PurchaseItem),
            SellItem(_SellItem),
            InventoryItem(_InventoryItem),
            Price(_Price),
            Brand(_Brand),
            Group(_Group),
            MinStock(_MinStock),
            MaxStock(_MaxStock),
            MinOrder(_MinOrder),
            MaxOrder(_MaxOrder),
            MinRequest(_MinRequest),
            MaxRequest(_MaxRequest)
        {
            ValidateItem();
        }

        const std::optional<std::string> Description;
        const std::optional<std::string> Image;
        const bool PurchaseItem;
        const bool SellItem;
        const bool InventoryItem;
        const double Price;
        const std::optional<int> Brand;
        const std::optional<int> Group;
        const std::optional<double> MinStock;
        const std::optional<double> MaxStock;
        const std::optional<double> MinOrder;
        const std::optional<double> MaxOrder;
        const std::optional<double> MinRequest;
        const std::optional<double> MaxRequest;

        void ValidateItem() const
        {
            if (Price < 0.0)
                throw std::runtime_error("'Price' must be >= 0.");

            ValidateOptionalRange(MinStock, "MinStock");
            ValidateOptionalRange(MaxStock, "MaxStock");
            ValidateOptionalRange(MinOrder, "MinOrder");
            ValidateOptionalRange(MaxOrder, "MaxOrder");
            ValidateOptionalRange(MinRequest, "MinRequest");
            ValidateOptionalRange(MaxRequest, "MaxRequest");

            ValidateMinMaxPair(MinStock, MaxStock, "MinStock", "MaxStock");
            ValidateMinMaxPair(MinOrder, MaxOrder, "MinOrder", "MaxOrder");
            ValidateMinMaxPair(MinRequest, MaxRequest, "MinRequest", "MaxRequest");

            if (Description.has_value())
            {
                const std::string &d = Description.value();

                if (!std::regex_match(d, descriptionLengthRegex))
                    throw std::runtime_error("'Description' length must be 3-200 characters.");

                if (!std::regex_match(d, descriptionValidCharsRegex))
                    throw std::runtime_error("'Description' contains invalid characters.");
            }

            if (Image.has_value())
            {
                const std::string &img = Image.value();

                if (img.empty())
                    throw std::runtime_error("Empty or invalid image file");
            }

            ValidateBrandAndGroup(Brand, "Brand");
            ValidateBrandAndGroup(Group, "Group");
        }

    private:

        void ValidateOptionalRange(const std::optional<double> &val, const char *field) const
        {
            if (val.has_value() && val.value() < 0)
                throw std::runtime_error(std::string(field) + "' must be >= 0.");
        }

        void ValidateMinMaxPair(const std::optional<double> &minV, const std::optional<double> &maxV, const char *minName, const char *maxName) const
        {
            if (minV.has_value() && maxV.has_value())
            {
                if (minV.value() > maxV.value())
                {
                    throw std::runtime_error( std::string("") + minName + " cannot be greater than " + maxName);
                }
            }
        }

        void ValidateBrandAndGroup(const std::optional<int> &_entry, const std::string &field) const
        {
            if(Brand.has_value() && Brand.value() <= 0 )
                throw std::runtime_error("[Validation error]: " + field + " cannot be negative or zero");

            if(Group.has_value() && Group.value() <= 0 )
                throw std::runtime_error("[Validation error]: " + field + " cannot be negative or zero");
        }

        const std::regex descriptionLengthRegex{R"(^.{3,200}$)"};
        const std::regex descriptionValidCharsRegex{R"(^[A-Za-z0-9\s\.,;:!¡¿\?\-_()]*$)"};
    };

}
