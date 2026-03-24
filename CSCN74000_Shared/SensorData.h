#pragma once

#include <cstdint>
#include <string>

namespace Shared
{
    struct SensorData
    {
        std::string name;
        double value;
        std::string unit;
        std::uint64_t timestamp;

        SensorData()
            : value(0.0), timestamp(0)
        {
        }

        SensorData(const std::string& sensorName,
            double sensorValue,
            const std::string& sensorUnit,
            std::uint64_t sensorTimestamp)
            : name(sensorName),
            value(sensorValue),
            unit(sensorUnit),
            timestamp(sensorTimestamp)
        {
        }
    };
}