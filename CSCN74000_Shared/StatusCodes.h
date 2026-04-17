#pragma once

#include <cstdint>

namespace Shared
{
    // Status codes used to indicate request/result outcome in packets.
    enum class StatusCode : std::uint32_t
    {
        SUCCESS = 0,
        INVALID_COMMAND = 1,
        NOT_VERIFIED = 2,
        INVALID_STATE = 3,
        INVALID_PACKET = 4,
        PAYLOAD_TOO_LARGE = 5,
        SERIALIZATION_ERROR = 6,
        INTERNAL_ERROR = 7,
        AUTH_FAILED = 8
    };
}
