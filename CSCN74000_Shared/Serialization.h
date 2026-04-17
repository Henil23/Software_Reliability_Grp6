#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Packet.h"
#include "SensorData.h"

namespace Shared
{
    // Handles conversion of packets and sensor data
    // between in-memory objects and raw byte buffers.
    class Serialization
    {
    public:
        // Returns current timestamp used in packet headers.
        static std::uint64_t GetCurrentTimestamp();

        // Packet header serialization helpers.
        static bool SerializePacketHeader(const PacketHeader& header, std::vector<std::uint8_t>& outBuffer);
        static bool DeserializePacketHeader(const std::vector<std::uint8_t>& buffer, PacketHeader& outHeader);

        // Complete packet serialization helpers.
        static bool SerializePacket(const Packet& packet, std::vector<std::uint8_t>& outBuffer);
        static bool DeserializePacket(const std::vector<std::uint8_t>& buffer, Packet& outPacket);

        // Sensor list serialization helpers.
        static bool SerializeSensorDataList(const std::vector<SensorData>& sensors, std::vector<std::uint8_t>& outBuffer);
        static bool DeserializeSensorDataList(const std::vector<std::uint8_t>& buffer, std::vector<SensorData>& outSensors);

        // Converts plain text to payload bytes and back.
        static bool BuildTextPayload(const std::string& text, std::vector<std::uint8_t>& outPayload);
        static std::string ExtractTextPayload(const std::vector<std::uint8_t>& payload);

        // Basic validation and utility helpers.
        static bool ValidatePayloadSize(std::size_t payloadSize);
        static std::size_t GetPacketHeaderSize();

    private:
        // Low-level write helpers used during serialization.
        static bool WriteUInt32(std::vector<std::uint8_t>& buffer, std::uint32_t value);
        static bool WriteUInt64(std::vector<std::uint8_t>& buffer, std::uint64_t value);
        static bool WriteDouble(std::vector<std::uint8_t>& buffer, double value);
        static bool WriteString(std::vector<std::uint8_t>& buffer, const std::string& value);

        // Low-level read helpers used during deserialization.
        static bool ReadUInt32(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::uint32_t& value);
        static bool ReadUInt64(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::uint64_t& value);
        static bool ReadDouble(const std::vector<std::uint8_t>& buffer, std::size_t& offset, double& value);
        static bool ReadString(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::string& value);
    };
}
