#pragma once

#include <cstdint>
#include "PacketTypes.h"
#include "StatusCodes.h"

namespace Shared
{
    struct PacketHeader
    {
        PacketType type;
        std::uint64_t timestamp;
        std::uint32_t payloadSize;
        std::uint32_t sensorCount;
        StatusCode status;

        PacketHeader()
            : type(PacketType::UNKNOWN),
            timestamp(0),
            payloadSize(0),
            sensorCount(0),
            status(StatusCode::SUCCESS)
        {
        }
    };
}