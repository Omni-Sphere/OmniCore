#pragma once

#include <string>

namespace omnicore::model
{
class Payload
{
public:
    uint64_t ExpiresAt;
    uint64_t IssuedAt;
    std::string SessionUUID;        
};
}