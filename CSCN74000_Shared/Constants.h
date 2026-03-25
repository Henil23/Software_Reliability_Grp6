#pragma once

#include <cstddef>
#include <cstdint>

namespace Shared
{
    constexpr std::uint16_t DEFAULT_SERVER_PORT = 54000;

    constexpr std::size_t MAX_PAYLOAD_SIZE = 2 * 1024 * 1024;
    constexpr std::size_t TELEMETRY_CHUNK_SIZE = 4096;

    constexpr std::size_t MAX_SENSOR_NAME_LENGTH = 32;
    constexpr std::size_t MAX_SENSOR_UNIT_LENGTH = 16;

    constexpr const char* CLIENT_LOG_FILE = "client_packets.log";
    constexpr const char* SERVER_LOG_FILE = "server_packets.log";

    constexpr const char* EXPECTED_VERIFICATION_TOKEN = "GROUND_STATION_AUTH";
}