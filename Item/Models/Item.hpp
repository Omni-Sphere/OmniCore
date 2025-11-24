#pragma once
#include <string>
#include <optional>
#include "BaseModel.hpp"

namespace omnicore::model
{
    class Item : public BaseModel
    {
    public:        
        Item(
            int _Entry,
            std::string _Code,
            std::string _Name,
            std::optional<std::string> _Description,
            std::optional<std::string> _Image,
            bool _IsActive,
            bool _PurchaseItem,
            bool _SellItem,
            bool _InventoryItem,
            double _Price,
            std::optional<int> _Brand,
            std::optional<int> _Group,
            double _OnHand,
            std::optional<double> _OnOrder,
            std::optional<double> _OnRequest,
            std::optional<double> _MinStock,
            std::optional<double> _MaxStock,
            std::optional<double> _MinOrder,
            std::optional<double> _MaxOrder,
            std::optional<double> _MinRequest,
            std::optional<double> _MaxRequest,
            int _CreatedBy,
            std::string _CreateDate,
            std::optional<int> _LastUpdatedBy,
            std::optional<std::string> _LastUpdateDate)
            :
            BaseModel(
                _Entry,
                std::move(_Code),
                std::move(_Name),
                _CreatedBy,
                std::move(_CreateDate),
                _LastUpdatedBy,
                std::move(_LastUpdateDate)
            ),
            Description(std::move(_Description)),
            Image(std::move(_Image)),
            IsActive(_IsActive),
            PurchaseItem(_PurchaseItem),
            SellItem(_SellItem),
            InventoryItem(_InventoryItem),
            Price(_Price),
            Brand(_Brand),
            Group(_Group),
            OnHand(_OnHand),
            OnOrder(_OnOrder),
            OnRequest(_OnRequest),
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
        const bool IsActive;
        const bool PurchaseItem;
        const bool SellItem;
        const bool InventoryItem;
        const double Price;
        const std::optional<int> Brand;
        const std::optional<int> Group;
        const double OnHand;
        const std::optional<double> OnOrder;
        const std::optional<double> OnRequest;
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

        const std::regex descriptionLengthRegex{R"(^.{3,200}$)"};
        const std::regex descriptionValidCharsRegex{R"(^[A-Za-z0-9\s\.,;:!¡¿\?\-_()]*$)"};
    };

}