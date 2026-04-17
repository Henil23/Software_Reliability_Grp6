# Aircraft Sensor Data Monitoring System

A C++ client-server communication system that simulates an avionics-style data exchange between a **Ground Station Client** and an **Aircraft Sensor Server**.

The system demonstrates structured TCP/IP communication, server-side state control, verification before operational commands, live aircraft sensor monitoring, telemetry file transfer, logging, and defensive packet handling.

---

## Overview

This system models a ground station connecting to an aircraft-side server and performing controlled communication through a predefined packet protocol.

The client can:

- connect to the server
- verify the session
- request current sensor data
- request a telemetry log file
- disconnect cleanly

The server is responsible for:

- listening for incoming connections
- enforcing verification before commands are accepted
- managing the communication state machine
- returning sensor data
- transferring a telemetry file larger than 1 MB
- logging packet and event activity

---

## Features

- TCP/IP client-server communication
- Structured binary packet protocol
- Session verification before operational commands
- Live aircraft sensor-data response
- Telemetry file transfer in chunks
- Acknowledgement and error packets
- Server-side finite state machine
- Packet and event logging
- Clean disconnect handling
- Static analysis review using PVS-Studio

---

## Architecture

### Ground Station Client
The client acts as the operator-facing application. It connects to the server, sends requests, receives responses, displays sensor data, and reconstructs the received telemetry file.

### Aircraft Sensor Server
The server acts as the aircraft-side communication endpoint. It validates requests, enforces protocol sequencing, generates sensor responses, transfers telemetry data, and manages session state.

### Shared Communication Layer
A shared layer is used by both applications and contains:

- packet definitions
- packet types
- status codes
- sensor data structures
- serialization/deserialization logic
- logging utilities
- constants
- state definitions

---

## Packet Structure

The system uses a **structured binary packet format** with a fixed header and a dynamic payload.

### Packet Header Fields
- `type` — identifies the packet purpose
- `timestamp` — packet creation time
- `payloadSize` — payload length in bytes
- `sensorCount` — number of sensor entries in a response
- `status` — success or error status

### Packet Layout
```cpp
struct PacketHeader
{
    PacketType type;
    uint64_t timestamp;
    uint32_t payloadSize;
    uint32_t sensorCount;
    StatusCode status;
};

struct Packet
{
    PacketHeader header;
    std::vector<uint8_t> payload;
};
