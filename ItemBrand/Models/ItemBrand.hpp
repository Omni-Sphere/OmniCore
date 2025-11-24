#pragma once

#include <string>

namespace omnicore::model
{
    class ItemBrand
    {
    public:
        int Entry;
        std::string Code;
        std::string Name;
        int CreatedBy;
        std::string CreateDate;
        std::optional<int> LastUpdatedBy;
        std::optional<std::string> UpdateDate; 
    };
}