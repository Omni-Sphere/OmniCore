#pragma once
#include <string>
#include <optional>
#include <stdexcept>
#include <regex>
#include "BaseUpdateDTO.hpp"

namespace omnicore::dto {
    struct UpdateItem : public BaseUpdateDTO
    {
        UpdateItem(
            std::string _Code,
            std::optional<std::string> _Name,
            std::optional<std::string> _Description,
            std::optional<std::string> _Image,
            std::optional<bool> _IsActive,
            std::optional<bool> _PurchaseItem,
            std::optional<bool> _SellItem,
            std::optional<bool> _InventoryItem,
            std::optional<double> _Price,
            std::optional<int> _Brand,
            std::optional<int> _Group,
            std::optional<double> _MinStock,
            std::optional<double> _MaxStock,
            std::optional<double> _MinOrder,
            std::optional<double> _MaxOrder,
            std::optional<double> _MinRequest,
            std::optional<double> _MaxRequest,
            int _LastUpdatedBy,
            std::string _UpdateDate)
            :
            BaseUpdateDTO(std::move(_Code), std::move(_Name), _LastUpdatedBy, std::move(_UpdateDate)),        
            Description(std::move(_Description)),
            Image(std::move(_Image)),
            IsActive(_IsActive),
            PurchaseItem(_PurchaseItem),
            SellItem(_SellItem),
            InventoryItem(_InventoryItem),
            Price(_Price),
            Brand(std::move(_Brand)),
            Group(std::move(_Group)),
            MinStock(_MinStock),
            MaxStock(_MaxStock),
            MinOrder(_MinOrder),
            MaxOrder(_MaxOrder),
            MinRequest(_MinRequest),
            MaxRequest(_MaxRequest)
        {
            ValidateUpdateItem();
        }

        const std::optional<std::string> Description;
        const std::optional<std::string> Image;
        const std::optional<bool> IsActive;
        const std::optional<bool> PurchaseItem;
        const std::optional<bool> SellItem;
        const std::optional<bool> InventoryItem;
        const std::optional<double> Price;
        const std::optional<int> Brand;
        const std::optional<int> Group;
        const std::optional<double> MinStock;
        const std::optional<double> MaxStock;
        const std::optional<double> MinOrder;
        const std::optional<double> MaxOrder;
        const std::optional<double> MinRequest;
        const std::optional<double> MaxRequest;

        void ValidateUpdateItem() const
        {
            if (Price.has_value() && *Price < 0)
                throw std::runtime_error("Validation error: Price cannot be negative.");

            ValidateRange(MinStock, MaxStock, "MinStock", "MaxStock");
            ValidateRange(MinOrder, MaxOrder, "MinOrder", "MaxOrder");
            ValidateRange(MinRequest, MaxRequest, "MinRequest", "MaxRequest");
            ValidateBrandAndGroup(Brand, "Brand");
            ValidateBrandAndGroup(Group, "Group");
        }

        void ValidateRange(const std::optional<double> &minV, const std::optional<double> &maxV, const char *minName, const char *maxName) const
        {
            if (minV.has_value() && *minV < 0)
                throw std::runtime_error(std::string("Validation error: ") + minName + " cannot be negative.");

            if (maxV.has_value() && *maxV < 0)
                throw std::runtime_error(std::string("Validation error: ") + maxName + " cannot be negative.");

            if (minV.has_value() && maxV.has_value() && *minV > *maxV)
                throw std::runtime_error(std::string("Validation error: ") + minName + " cannot be greater than " + maxName + ".");
        }

        void ValidateBrandAndGroup(const std::optional<int> &_entry, const std::string &field) const
        {
            if(Brand.has_value() && Brand.value() <= 0 )
                throw std::runtime_error("[Validation error]: " + field + " cannot be negative or zero");

            if(Group.has_value() && Group.value() <= 0 )
                throw std::runtime_error("[Validation error]: " + field + " cannot be negative or zero");
        }
    };
}