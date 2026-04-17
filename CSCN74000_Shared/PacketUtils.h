#pragma once

#include <string>
#include <vector>

#include "Packet.h"
#include "SensorData.h"
#include "Serialization.h"

namespace Shared
{
    // Helper class for creating commonly used packet types.
    class PacketUtils
    {
    public:
        // Creates a packet with header fields only and no payload.
        static Packet CreateSimplePacket(PacketType type, StatusCode status = StatusCode::SUCCESS)
        {
            Packet packet;
            packet.header.type = type;
            packet.header.timestamp = Serialization::GetCurrentTimestamp();
            packet.header.payloadSize = 0;
            packet.header.sensorCount = 0;
            packet.header.status = status;
            return packet;
        }

        // Creates a packet containing a text message payload.
        static Packet CreateTextPacket(PacketType type, const std::string& text, StatusCode status = StatusCode::SUCCESS)
        {
            Packet packet = CreateSimplePacket(type, status);

            if (!Serialization::BuildTextPayload(text, packet.payload))
            {
                packet.header.status = StatusCode::PAYLOAD_TOO_LARGE;
                packet.payload.clear();
            }

            packet.UpdatePayloadSize();
            return packet;
        }

        // Creates a sensor response packet from a list of sensor readings.
        static Packet CreateSensorResponsePacket(const std::vector<SensorData>& sensors)
        {
            Packet packet = CreateSimplePacket(PacketType::SENSOR_RESPONSE, StatusCode::SUCCESS);

            if (!Serialization::SerializeSensorDataList(sensors, packet.payload))
            {
                packet.header.type = PacketType::ERROR_PACKET;
                packet.header.status = StatusCode::SERIALIZATION_ERROR;
                packet.payload.clear();
                packet.header.sensorCount = 0;
                packet.UpdatePayloadSize();
                return packet;
            }

            packet.header.sensorCount = static_cast<std::uint32_t>(sensors.size());
            packet.UpdatePayloadSize();
            return packet;
        }
    };
}
