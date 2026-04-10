#include "pch.h"
#include "CppUnitTest.h"

#include "Server.h"
#include "Packet.h"
#include "Serialization.h"
#include "PacketUtils.h"
#include "Constants.h"
#include "../CSCN74000_Server/Server.cpp" 

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Shared;

namespace ServerUnitTests
{
    TEST_CLASS(ServerTests)
    {
    public:

        // ================================
        // TEST 1: Successful Verification
        // ================================
        TEST_METHOD(Test_Verification_Success)
        {
            Server server(true);   

            Packet pkt;
            Serialization::BuildTextPayload(EXPECTED_VERIFICATION_TOKEN, pkt.payload);
            pkt.header.type = PacketType::VERIFY_REQUEST;
            pkt.UpdatePayloadSize();

            Packet response = server.HandleVerification(pkt);

            Assert::AreEqual((int)StatusCode::SUCCESS, (int)response.header.status);
        }

        // ================================
        // TEST 2: Failed Verification
        // ================================
        TEST_METHOD(Test_Verification_Fail)
        {
            Server server(true);

            Packet pkt;
            Serialization::BuildTextPayload("WRONG_TOKEN", pkt.payload);
            pkt.header.type = PacketType::VERIFY_REQUEST;
            pkt.UpdatePayloadSize();

            Packet response = server.HandleVerification(pkt);

            Assert::AreEqual((int)StatusCode::AUTH_FAILED, (int)response.header.status);
        }

        // ================================
        // TEST 3: Sensor Request WITHOUT verification
        // ================================
        TEST_METHOD(Test_SensorRequest_NotVerified)
        {
            Server server(true);

            Packet response = server.HandleSensorRequest();

            Assert::AreEqual((int)StatusCode::NOT_VERIFIED, (int)response.header.status);
        }

        // ================================
        // TEST 4: Sensor Request AFTER verification
        // ================================
        TEST_METHOD(Test_SensorRequest_Verified)
        {
            Server server(true);

            Packet verifyPkt;
            Serialization::BuildTextPayload(EXPECTED_VERIFICATION_TOKEN, verifyPkt.payload);
            verifyPkt.header.type = PacketType::VERIFY_REQUEST;
            verifyPkt.UpdatePayloadSize();

            server.HandleVerification(verifyPkt);

            Packet response = server.HandleSensorRequest();

            Assert::AreEqual((int)StatusCode::SUCCESS, (int)response.header.status);
            Assert::IsTrue(response.payload.size() > 0);
        }

        // ================================
        // TEST 5: Disconnect Request
        // ================================
        TEST_METHOD(Test_Disconnect_Request)
        {
            Server server(true);

            Packet response = server.HandleDisconnectRequest();

            Assert::AreEqual((int)StatusCode::SUCCESS, (int)response.header.status);
            Assert::AreEqual((int)PacketType::DISCONNECT_RESPONSE, (int)response.header.type);
        }

        // ================================
        // TEST 6: Server State after Verification
        // ================================
        TEST_METHOD(Test_State_After_Verification)
        {
            Server server(true);

            Packet pkt;
            Serialization::BuildTextPayload(EXPECTED_VERIFICATION_TOKEN, pkt.payload);
            pkt.header.type = PacketType::VERIFY_REQUEST;
            pkt.UpdatePayloadSize();

            server.HandleVerification(pkt);

            Assert::IsTrue(server.IsVerified());
        }

        // ================================
        // TEST 7: Invalid Command Handling
        // ================================
        TEST_METHOD(Test_Invalid_Command)
        {
            Packet response = PacketUtils::CreateTextPacket(
                PacketType::ERROR_PACKET,
                "Invalid command",
                StatusCode::INVALID_COMMAND
            );

            Assert::AreEqual((int)PacketType::ERROR_PACKET, (int)response.header.type);
        }

        // ================================
        // TEST 8: State Transition Sensor Mode
        // ================================
        TEST_METHOD(Test_State_Transition_Sensor)
        {
            Server server(true);

            Packet verifyPkt;
            Serialization::BuildTextPayload(EXPECTED_VERIFICATION_TOKEN, verifyPkt.payload);
            verifyPkt.header.type = PacketType::VERIFY_REQUEST;
            verifyPkt.UpdatePayloadSize();

            server.HandleVerification(verifyPkt);
            server.HandleSensorRequest();

            Assert::IsTrue(server.IsVerified());
        }
    };
}