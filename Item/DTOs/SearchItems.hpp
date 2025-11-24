#pragma once

#include <string>
#include <optional>

namespace omnicore::dto {
    struct SearchItems {
        std::optional<std::string> CodeEqualsTo;
        std::optional<std::string> CodeContains;
        std::optional<std::string> NameEqualsTo;
        std::optional<std::string> NameContains;
        std::optional<std::string> DescriptionEqualsTo;
        std::optional<std::string> DescriptionContains;
        std::optional<bool> IsActive;
        std::optional<bool> PurchaseItem;
        std::optional<bool> SellItem;
        std::optional<bool> InventoryItem;
        std::optional<double> Price;
        std::optional<double> OnHandEqualsTo;
        std::optional<double> OnHandGreaterThan;
        std::optional<double> OnHandLessThan;
        std::optional<double> OnRequestEqualsTo;
        std::optional<double> OnRequestGreaterThan;
        std::optional<double> OnRequestLessThan;
        std::optional<double> OnOrderEqualsTo;
        std::optional<double> OnOrderGreaterThan;
        std::optional<double> OnOrderLessThan;
        std::optional<double> MinStockEqualsTo;
        std::optional<double> MinStockGreaterThan;
        std::optional<double> MinStockLessThan;
        std::optional<double> MaxStockEqualsTo;
        std::optional<double> MaxStockGreaterThan;
        std::optional<double> MaxStockLessThan;
        std::optional<double> MinOrderEqualsTo;
        std::optional<double> MinOrderGreaterThan;
        std::optional<double> MinOrderLessThan;
        std::optional<double> MaxOrderEqualsTo;
        std::optional<double> MaxOrderGreaterThan;
        std::optional<double> MaxOrderLessThan;
        std::optional<double> MinRequestEqualsTo;
        std::optional<double> MinRequestGreaterThan;
        std::optional<double> MinRequestLessThan;
        std::optional<double> MaxReqeuestEqualsTo;
        std::optional<double> MaxRequestGreaterThan;
        std::optional<double> MaxRequestLessThan;
    };
}