#include "Serialization.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "Constants.h"

namespace Shared
{
    namespace
    {
        constexpr std::size_t PACKET_HEADER_SIZE =
            sizeof(std::uint32_t) +  // PacketType
            sizeof(std::uint64_t) +  // timestamp
            sizeof(std::uint32_t) +  // payloadSize
            sizeof(std::uint32_t) +  // sensorCount
            sizeof(std::uint32_t);   // StatusCode
    }

    std::uint64_t Serialization::GetCurrentTimestamp()
    {
        const auto now = std::chrono::system_clock::now();
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return static_cast<std::uint64_t>(milliseconds.count());
    }

    std::size_t Serialization::GetPacketHeaderSize()
    {
        return PACKET_HEADER_SIZE;
    }

    bool Serialization::ValidatePayloadSize(std::size_t payloadSize)
    {
        return payloadSize <= MAX_PAYLOAD_SIZE;
    }

    bool Serialization::SerializePacketHeader(const PacketHeader& header, std::vector<std::uint8_t>& outBuffer)
    {
        outBuffer.clear();
        outBuffer.reserve(PACKET_HEADER_SIZE);

        if (!WriteUInt32(outBuffer, static_cast<std::uint32_t>(header.type)))
        {
            return false;
        }

        if (!WriteUInt64(outBuffer, header.timestamp))
        {
            return false;
        }

        if (!WriteUInt32(outBuffer, header.payloadSize))
        {
            return false;
        }

        if (!WriteUInt32(outBuffer, header.sensorCount))
        {
            return false;
        }

        if (!WriteUInt32(outBuffer, static_cast<std::uint32_t>(header.status)))
        {
            return false;
        }

        return true;
    }

    bool Serialization::DeserializePacketHeader(const std::vector<std::uint8_t>& buffer, PacketHeader& outHeader)
    {
        if (buffer.size() < PACKET_HEADER_SIZE)
        {
            return false;
        }

        std::size_t offset = 0;
        std::uint32_t rawType = 0;
        std::uint32_t rawStatus = 0;

        if (!ReadUInt32(buffer, offset, rawType))
        {
            return false;
        }

        if (!ReadUInt64(buffer, offset, outHeader.timestamp))
        {
            return false;
        }

        if (!ReadUInt32(buffer, offset, outHeader.payloadSize))
        {
            return false;
        }

        if (!ReadUInt32(buffer, offset, outHeader.sensorCount))
        {
            return false;
        }

        if (!ReadUInt32(buffer, offset, rawStatus))
        {
            return false;
        }

        outHeader.type = static_cast<PacketType>(rawType);
        outHeader.status = static_cast<StatusCode>(rawStatus);

        return true;
    }

    bool Serialization::SerializePacket(const Packet& packet, std::vector<std::uint8_t>& outBuffer)
    {
        if (!packet.IsValid()) // header/payload mismatch
        {
            return false;
        }

        if (!ValidatePayloadSize(packet.payload.size())) // size guard
        {
            return false;
        }

        std::vector<std::uint8_t> headerBytes;
        if (!SerializePacketHeader(packet.header, headerBytes))
        {
            return false;
        }

        outBuffer.clear();
        outBuffer.reserve(headerBytes.size() + packet.payload.size());

        outBuffer.insert(outBuffer.end(), headerBytes.begin(), headerBytes.end());
        outBuffer.insert(outBuffer.end(), packet.payload.begin(), packet.payload.end());

        return true;
    }

    bool Serialization::DeserializePacket(const std::vector<std::uint8_t>& buffer, Packet& outPacket)
    {
        if (buffer.size() < PACKET_HEADER_SIZE)
        {
            return false;
        }

        std::vector<std::uint8_t> headerBytes(buffer.begin(), buffer.begin() + PACKET_HEADER_SIZE);
        if (!DeserializePacketHeader(headerBytes, outPacket.header))
        {
            return false;
        }

        if (!ValidatePayloadSize(outPacket.header.payloadSize))
        {
            return false;
        }

        const std::size_t expectedTotalSize = PACKET_HEADER_SIZE + outPacket.header.payloadSize;
        if (buffer.size() != expectedTotalSize) // reject malformed packet
        {
            return false;
        }

        outPacket.payload.assign(buffer.begin() + PACKET_HEADER_SIZE, buffer.end());

        return outPacket.IsValid();
    }

    bool Serialization::SerializeSensorDataList(const std::vector<SensorData>& sensors, std::vector<std::uint8_t>& outBuffer)
    {
        outBuffer.clear();

        if (sensors.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
        {
            return false;
        }

        if (!WriteUInt32(outBuffer, static_cast<std::uint32_t>(sensors.size())))
        {
            return false;
        }

        for (const SensorData& sensor : sensors)
        {
            if (sensor.name.size() > MAX_SENSOR_NAME_LENGTH)
            {
                return false;
            }

            if (sensor.unit.size() > MAX_SENSOR_UNIT_LENGTH)
            {
                return false;
            }

            if (!WriteString(outBuffer, sensor.name))
            {
                return false;
            }

            if (!WriteDouble(outBuffer, sensor.value))
            {
                return false;
            }

            if (!WriteString(outBuffer, sensor.unit))
            {
                return false;
            }

            if (!WriteUInt64(outBuffer, sensor.timestamp))
            {
                return false;
            }
        }

        return ValidatePayloadSize(outBuffer.size());
    }

    bool Serialization::DeserializeSensorDataList(const std::vector<std::uint8_t>& buffer, std::vector<SensorData>& outSensors)
    {
        outSensors.clear();

        std::size_t offset = 0;
        std::uint32_t sensorCount = 0;

        if (!ReadUInt32(buffer, offset, sensorCount))
        {
            return false;
        }

        for (std::uint32_t i = 0; i < sensorCount; ++i)
        {
            SensorData sensor;

            if (!ReadString(buffer, offset, sensor.name))
            {
                return false;
            }

            if (sensor.name.size() > MAX_SENSOR_NAME_LENGTH)
            {
                return false;
            }

            if (!ReadDouble(buffer, offset, sensor.value))
            {
                return false;
            }

            if (!ReadString(buffer, offset, sensor.unit))
            {
                return false;
            }

            if (sensor.unit.size() > MAX_SENSOR_UNIT_LENGTH)
            {
                return false;
            }

            if (!ReadUInt64(buffer, offset, sensor.timestamp))
            {
                return false;
            }

            outSensors.push_back(sensor);
        }

        return offset == buffer.size();
    }

    bool Serialization::BuildTextPayload(const std::string& text, std::vector<std::uint8_t>& outPayload)
    {
        if (!ValidatePayloadSize(text.size()))
        {
            return false;
        }

        outPayload.assign(text.begin(), text.end());
        return true;
    }

    std::string Serialization::ExtractTextPayload(const std::vector<std::uint8_t>& payload)
    {
        return std::string(payload.begin(), payload.end());
    }

    bool Serialization::WriteUInt32(std::vector<std::uint8_t>& buffer, std::uint32_t value)
    {
        const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
        return true;
    }

    bool Serialization::WriteUInt64(std::vector<std::uint8_t>& buffer, std::uint64_t value)
    {
        const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
        return true;
    }

    bool Serialization::WriteDouble(std::vector<std::uint8_t>& buffer, double value)
    {
        const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
        return true;
    }

    bool Serialization::WriteString(std::vector<std::uint8_t>& buffer, const std::string& value)
    {
        if (value.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
        {
            return false;
        }

        if (!WriteUInt32(buffer, static_cast<std::uint32_t>(value.size())))
        {
            return false;
        }

        buffer.insert(buffer.end(), value.begin(), value.end());
        return true;
    }

    bool Serialization::ReadUInt32(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::uint32_t& value)
    {
        if (offset + sizeof(value) > buffer.size())
        {
            return false;
        }

        std::memcpy(&value, buffer.data() + offset, sizeof(value));
        offset += sizeof(value);
        return true;
    }

    bool Serialization::ReadUInt64(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::uint64_t& value)
    {
        if (offset + sizeof(value) > buffer.size())
        {
            return false;
        }

        std::memcpy(&value, buffer.data() + offset, sizeof(value));
        offset += sizeof(value);
        return true;
    }

    bool Serialization::ReadDouble(const std::vector<std::uint8_t>& buffer, std::size_t& offset, double& value)
    {
        if (offset + sizeof(value) > buffer.size())
        {
            return false;
        }

        std::memcpy(&value, buffer.data() + offset, sizeof(value));
        offset += sizeof(value);
        return true;
    }

    bool Serialization::ReadString(const std::vector<std::uint8_t>& buffer, std::size_t& offset, std::string& value)
    {
        std::uint32_t length = 0;

        if (!ReadUInt32(buffer, offset, length))
        {
            return false;
        }

        if (offset + length > buffer.size())
        {
            return false;
        }

        value.assign(reinterpret_cast<const char*>(buffer.data() + offset), length);
        offset += length;
        return true;
    }
}