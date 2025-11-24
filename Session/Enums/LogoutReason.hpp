#pragma once

namespace omnicore::enums
{
    enum LogoutReason
    {        
        UNDEFINED = -1,
        USER_LOGOUT,
        SESSION_TIMEOUT,
        TOKEN_EXPIRED,
        FORCE_LOGOUT,
        DEVICE_DISCONNECTED,
        SECURITY_EVENT
    };
}