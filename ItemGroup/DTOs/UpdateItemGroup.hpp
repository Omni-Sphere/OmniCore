#pragma once

#include <string>
#include "BaseUpdateDTO.hpp"

namespace omnicore::dto
{
    struct UpdateItemGroup : public BaseUpdateDTO
    {
        using BaseUpdateDTO::BaseUpdateDTO;
    };
}