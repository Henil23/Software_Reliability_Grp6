#pragma once

#include <string>

#include "Packet.h"

namespace Shared
{
    enum class LogDirection
    {
        SENT = 0,
        RECEIVED = 1,
        EVENT = 2
    };

    class Logger
    {
    public:
        static void LogPacket(
            const std::string& filePath,
            const Packet& packet,
            LogDirection direction,
            const std::string& note = ""
        );

        static void LogEvent(
            const std::string& filePath,
            const std::string& message
        );

    private:
        static std::string DirectionToString(LogDirection direction);
        static std::string PacketTypeToString(PacketType type);
        static std::string StatusCodeToString(StatusCode status);
    };
}