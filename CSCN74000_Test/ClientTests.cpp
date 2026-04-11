#include "pch.h"
#include "CppUnitTest.h"

#include "../Client/Packet.h"
#include "../Client/Serialization.h"
#include "../Client/Constants.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CSCN74000_Test
{
    TEST_CLASS(ClientPacketTests)
    {
    public:

        // ? Test 1: Packet payload size updates correctly
        TEST_METHOD(Test_PacketPayloadSizeUpdate)
        {
            Shared::Packet packet;
            packet.payload = { 1, 2, 3, 4 };

            packet.UpdatePayloadSize();

            Assert::AreEqual((uint32_t)4, packet.header.payloadSize);
        }

        // ? Test 2: Packet validity check
        TEST_METHOD(Test_PacketValidity)
        {
            Shared::Packet packet;
            packet.payload = { 1, 2, 3 };

            packet.UpdatePayloadSize();

            Assert::IsTrue(packet.IsValid());
        }

        // ? Test 3: Invalid packet (wrong payload size)
        TEST_METHOD(Test_InvalidPacket)
        {
            Shared::Packet packet;
            packet.payload = { 1, 2, 3 };

            packet.header.payloadSize = 10; // wrong

            Assert::IsFalse(packet.IsValid());
        }

        // ? Test 4: Serialize text payload
        TEST_METHOD(Test_TextPayloadSerialization)
        {
            std::vector<uint8_t> payload;

            bool result = Shared::Serialization::BuildTextPayload("HELLO", payload);

            Assert::IsTrue(result);
            Assert::IsTrue(payload.size() > 0);
        }

        // ? Test 5: Payload size validation (too large)
        TEST_METHOD(Test_ValidatePayloadTooLarge)
        {
            bool result = Shared::Serialization::ValidatePayloadSize(
                Shared::MAX_PAYLOAD_SIZE + 1
            );

            Assert::IsFalse(result);
        }
    };
}