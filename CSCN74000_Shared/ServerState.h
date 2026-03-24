#pragma once

#include <cstdint>

namespace Shared
{
    enum class ServerState : std::uint32_t
    {
        WAITING_FOR_CONNECTION = 0,
        CONNECTED = 1,
        VERIFIED = 2,
        SENSOR_DATA = 3,
        TELEMETRY_TRANSFER = 4,
        DISCONNECTING = 5,
        ERROR_STATE = 6
    };
}