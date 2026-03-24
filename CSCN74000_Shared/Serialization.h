#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Packet.h"
#include "SensorData.h"

namespace Shared
{
    class Serialization
    {
    public:
        static std::uint64_t GetCurrentTimestamp();

        static bool SerializePacketHeader(const PacketHeader& header, std::vector<std::uint8_t>& outBuffer);
        static bool DeserializePacketHeader(const std::vector<std::uint8_t>& buffer, PacketHeader& outHeader);

        static bool SerializePacket(const Packet& packet, std::vector<std::uint8_t>& outBuffer);
        static bool DeserializePacket(const std::vector<std::uint8_t>& buffer, Packet& outPacket);

        static bool SerializeSensorDataList(const std::vector<SensorData>& sensors, std::vector<std::uint8_t>& outBuffer);
        static bool DeserializeSensorDataList(const std::vector<std::uint8_t>& buffer, std::vector<SensorData>& outSensors);

        static bool BuildTextPayload(const std::string& text, std::vector<std::uint8_t>& outPayload);
        static std::string ExtractTextPayload(const std::vector<std::uint8_t>& payload);

        static bool ValidatePayloadSize(std::size_t payloadSize);
        static std::size_t GetPacketHeaderSize();

    private:
        static bool WriteUInt32(std::vector<std::uint8_t>& buffer, std::uint32_t value);
        static bool WriteUInt64(std::vector<std::uint8_t>& buffer, std::uint64_t value);
        static bool WriteDouble(std::vector<std::uint8_t>& buffer, double value);
        static bool WriteString(std::vector<std::uint8_t>& buffer, const std::string& value);

        static bool ReadUInt32(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::uint32_t& value);
        static bool ReadUInt64(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::uint64_t& value);
        static bool ReadDouble(const std::vector<std::uint8_t>& buffer, std::size_t& offset, double& value);
        static bool ReadString(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::string& value);
    };
}