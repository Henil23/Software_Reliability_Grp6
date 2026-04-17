#pragma once

#include <cstdint>
#include "PacketTypes.h"
#include "StatusCodes.h"

namespace Shared
{
    // Stores metadata for each packet before the actual payload.
    struct PacketHeader
    {
        PacketType type;
        std::uint64_t timestamp;
        std::uint32_t payloadSize;
        std::uint32_t sensorCount;
        StatusCode status;

        // Default values keep the packet in a safe initial state.
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
