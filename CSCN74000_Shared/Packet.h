#pragma once

#include <cstdint>
#include <vector>
#include "PacketHeader.h"

namespace Shared
{
    // Represents a complete packet sent between client and server.
    // It contains a fixed header and a variable-size payload.
    struct Packet
    {
        PacketHeader header;
        std::vector<std::uint8_t> payload; // Dynamic payload data

        Packet() = default;

        // Checks whether payload size stored in the header matches actual payload size.
        bool IsValid() const
        {
            return header.payloadSize == payload.size();
        }

        // Updates header payload size after modifying payload data.
        void UpdatePayloadSize()
        {
            header.payloadSize = static_cast<std::uint32_t>(payload.size());
        }
    };
}
