#include "Logger.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace
{
    // Protects log file writing so multiple log calls do not overlap.
    std::mutex g_logMutex;

    // Returns current local date and time as a formatted string.
    std::string GetLocalTimeString()
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t timeNow = std::chrono::system_clock::to_time_t(now);

        std::tm localTime{};
        localtime_s(&localTime, &timeNow);

        std::ostringstream stream;
        stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        return stream.str();
    }
}

namespace Shared
{
    // Writes packet-related information to the given log file.
    void Logger::LogPacket(
        const std::string& filePath,
        const Packet& packet,
        LogDirection direction,
        const std::string& note
    )
    {
        std::lock_guard<std::mutex> lock(g_logMutex);

        std::ofstream file(filePath, std::ios::app);
        if (!file.is_open())
        {
            return;
        }

        file << "[" << GetLocalTimeString() << "] "
            << "packet_ts=" << packet.header.timestamp
            << ", direction=" << DirectionToString(direction)
            << ", type=" << PacketTypeToString(packet.header.type)
            << ", payload_size=" << packet.header.payloadSize
            << ", sensor_count=" << packet.header.sensorCount
            << ", status=" << StatusCodeToString(packet.header.status);

        if (!note.empty())
        {
            file << ", note=" << note;
        }

        file << "\n";
    }

    // Writes a general system event message to the log file.
    void Logger::LogEvent(
        const std::string& filePath,
        const std::string& message
    )
    {
        std::lock_guard<std::mutex> lock(g_logMutex);

        std::ofstream file(filePath, std::ios::app);
        if (!file.is_open())
        {
            return;
        }

        file << "[" << GetLocalTimeString() << "] "
            << "event=" << message << "\n";
    }

    // Converts log direction enum to readable text.
    std::string Logger::DirectionToString(LogDirection direction)
    {
        switch (direction)
        {
        case LogDirection::SENT:
            return "SENT";
        case LogDirection::RECEIVED:
            return "RECEIVED";
        case LogDirection::EVENT:
            return "EVENT";
        default:
            return "UNKNOWN";
        }
    }

    // Converts packet type enum to readable text.
    std::string Logger::PacketTypeToString(PacketType type)
    {
        switch (type)
        {
        case PacketType::VERIFY_REQUEST:
            return "VERIFY_REQUEST";
        case PacketType::VERIFY_RESPONSE:
            return "VERIFY_RESPONSE";
        case PacketType::SENSOR_REQUEST:
            return "SENSOR_REQUEST";
        case PacketType::SENSOR_RESPONSE:
            return "SENSOR_RESPONSE";
        case PacketType::TELEMETRY_REQUEST:
            return "TELEMETRY_REQUEST";
        case PacketType::TELEMETRY_CHUNK:
            return "TELEMETRY_CHUNK";
        case PacketType::TELEMETRY_COMPLETE:
            return "TELEMETRY_COMPLETE";
        case PacketType::ACK:
            return "ACK";
        case PacketType::ERROR_PACKET:
            return "ERROR_PACKET";
        case PacketType::DISCONNECT_REQUEST:
            return "DISCONNECT_REQUEST";
        case PacketType::DISCONNECT_RESPONSE:
            return "DISCONNECT_RESPONSE";
        default:
            return "UNKNOWN";
        }
    }

    // Converts status code enum to readable text.
    std::string Logger::StatusCodeToString(StatusCode status)
    {
        switch (status)
        {
        case StatusCode::SUCCESS:
            return "SUCCESS";
        case StatusCode::INVALID_COMMAND:
            return "INVALID_COMMAND";
        case StatusCode::NOT_VERIFIED:
            return "NOT_VERIFIED";
        case StatusCode::INVALID_STATE:
            return "INVALID_STATE";
        case StatusCode::INVALID_PACKET:
            return "INVALID_PACKET";
        case StatusCode::PAYLOAD_TOO_LARGE:
            return "PAYLOAD_TOO_LARGE";
        case StatusCode::SERIALIZATION_ERROR:
            return "SERIALIZATION_ERROR";
        case StatusCode::INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        case StatusCode::AUTH_FAILED:
            return "AUTH_FAILED";
        default:
            return "UNKNOWN";
        }
    }
}
