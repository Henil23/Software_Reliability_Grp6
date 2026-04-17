#pragma once

#include <string>

#include "Packet.h"

namespace Shared
{
    // Indicates whether a log entry is for a sent packet,
    // received packet, or a general event.
    enum class LogDirection
    {
        SENT = 0,
        RECEIVED = 1,
        EVENT = 2
    };

    // Utility class for writing packet activity and system events to log files.
    class Logger
    {
    public:
        // Logs packet metadata such as type, size, status, and direction.
        static void LogPacket(
            const std::string& filePath,
            const Packet& packet,
            LogDirection direction,
            const std::string& note = ""
        );

        // Logs a simple event message with timestamp.
        static void LogEvent(
            const std::string& filePath,
            const std::string& message
        );

    private:
        // Helper functions to convert enums into readable text for logs.
        static std::string DirectionToString(LogDirection direction);
        static std::string PacketTypeToString(PacketType type);
        static std::string StatusCodeToString(StatusCode status);
    };
}
