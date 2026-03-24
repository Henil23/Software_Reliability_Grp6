#pragma once

#include <cstdint>

namespace Shared
{
    enum class PacketType : std::uint32_t
    {
        UNKNOWN = 0,

        VERIFY_REQUEST = 1,
        VERIFY_RESPONSE = 2,

        SENSOR_REQUEST = 3,
        SENSOR_RESPONSE = 4,

        TELEMETRY_REQUEST = 5,
        TELEMETRY_CHUNK = 6,
        TELEMETRY_COMPLETE = 7,

        ACK = 8,
        ERROR_PACKET = 9,

        DISCONNECT_REQUEST = 10,
        DISCONNECT_RESPONSE = 11
    };
}