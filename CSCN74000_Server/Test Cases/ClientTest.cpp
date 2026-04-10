#include "pch.h"
#include "CppUnitTest.h"

#include "Packet.h"
#include "Serialization.h"
#include "PacketUtils.h"
#include "Constants.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Shared;

namespace ClientUnitTests
{
    TEST_CLASS(ClientTests)
    {
    public:

        // ============================
        // TEST 1: Serialize Packet
        // ============================
        TEST_METHOD(TestSerializePacket)
        {
            Packet pkt;
            pkt.header.type = PacketType::VERIFY_REQUEST;
            pkt.header.timestamp = Serialization::GetCurrentTimestamp();

            Serialization::BuildTextPayload("HELLO", pkt.payload);
            pkt.UpdatePayloadSize();

            std::vector<uint8_t> buffer;
            bool success = Serialization::SerializePacket(pkt, buffer);

            Assert::IsTrue(success);
            Assert::IsTrue(!buffer.empty());
        }

        // ============================
        // TEST 2: Deserialize Packet
        // ============================
        TEST_METHOD(TestDeserializePacket)
        {
            Packet pkt;
            pkt.header.type = PacketType::VERIFY_REQUEST;
            pkt.header.timestamp = Serialization::GetCurrentTimestamp();

            Serialization::BuildTextPayload("TEST", pkt.payload);
            pkt.UpdatePayloadSize();

            std::vector<uint8_t> buffer;
            Serialization::SerializePacket(pkt, buffer);

            Packet out;
            bool success = Serialization::DeserializePacket(buffer, out);

            Assert::IsTrue(success);
            Assert::AreEqual((int)PacketType::VERIFY_REQUEST, (int)out.header.type);
        }

        // ============================
        // TEST 3: Text Payload
        // ============================
        TEST_METHOD(TestTextPayload)
        {
            std::vector<uint8_t> payload;
            Serialization::BuildTextPayload("VERIFY", payload);

            std::string text = Serialization::ExtractTextPayload(payload);

            Assert::AreEqual(std::string("VERIFY"), text);
        }

        // ============================
        // TEST 4: Payload Size Validation
        // ============================
        TEST_METHOD(TestPayloadSize)
        {
            bool valid = Serialization::ValidatePayloadSize(MAX_PAYLOAD_SIZE + 1);

            Assert::IsFalse(valid);
        }

        // ============================
        // TEST 5: Sensor Serialization
        // ============================
        TEST_METHOD(TestSensorSerialization)
        {
            std::vector<SensorData> sensors;

            SensorData s;
            s.name = "Temp";
            s.value = 25.5;
            s.unit = "C";
            s.timestamp = Serialization::GetCurrentTimestamp();

            sensors.push_back(s);

            std::vector<uint8_t> buffer;
            bool ok1 = Serialization::SerializeSensorDataList(sensors, buffer);

            std::vector<SensorData> out;
            bool ok2 = Serialization::DeserializeSensorDataList(buffer, out);

            Assert::IsTrue(ok1);
            Assert::IsTrue(ok2);
            Assert::AreEqual((size_t)1, out.size());
        }

        // ============================
        // TEST 6: PacketUtils
        // ============================
        TEST_METHOD(TestPacketUtils)
        {
            Packet pkt = PacketUtils::CreateSimplePacket(PacketType::SENSOR_REQUEST);

            Assert::AreEqual((int)PacketType::SENSOR_REQUEST, (int)pkt.header.type);
        }
    };
}