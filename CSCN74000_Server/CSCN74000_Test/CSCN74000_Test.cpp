#include "pch.h"
#include "CppUnitTest.h"

#include <vector>
#include <cstdint>

#include "Packet.h"
#include "Serialization.h"
#include "Constants.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CSCN74000_Test
{
    TEST_CLASS(ClientPacketTests)
    {
    public:

        TEST_METHOD(Test_PacketPayloadSizeUpdate)
        {
            Shared::Packet packet;
            packet.payload = { 1, 2, 3, 4 };

            packet.UpdatePayloadSize();

            Assert::AreEqual((uint32_t)4, (uint32_t)packet.header.payloadSize);
        }

        TEST_METHOD(Test_PacketValidity)
        {
            Shared::Packet packet;
            packet.payload = { 1, 2, 3 };

            packet.UpdatePayloadSize();

            Assert::IsTrue(packet.IsValid());
        }

        TEST_METHOD(Test_InvalidPacket)
        {
            Shared::Packet packet;
            packet.payload = { 1, 2, 3 };

            packet.header.payloadSize = 10;

            Assert::IsFalse(packet.IsValid());
        }

        TEST_METHOD(Test_TextPayloadSerialization)
        {
            std::vector<uint8_t> payload;

            bool result = Shared::Serialization::BuildTextPayload("HELLO", payload);

            Assert::IsTrue(result);
            Assert::IsTrue(payload.size() > 0);
        }

        TEST_METHOD(Test_ValidatePayloadTooLarge)
        {
            bool result = Shared::Serialization::ValidatePayloadSize(
                Shared::MAX_PAYLOAD_SIZE + 1
            );

            Assert::IsFalse(result);
        }
    };
}