#pragma once

#include <cstdint>
#include <vector>
#include "PacketHeader.h"

namespace Shared
{
    struct Packet
    {
        PacketHeader header;
        std::vector<std::uint8_t> payload; // dynamic payload

        Packet() = default;

        bool IsValid() const
        {
            return header.payloadSize == payload.size();
        }

        void UpdatePayloadSize()
        {
            header.payloadSize = static_cast<std::uint32_t>(payload.size());
        }
    };
}