import csv
import random
from datetime import datetime, timedelta
from pathlib import Path
# this file creates a file (>1 MB) with arbitary data
OUTPUT_FILE = Path("data/telemetry_log.txt")
TARGET_SIZE_BYTES = 1_300_000  # safely above 1 MB
START_TIME = datetime(2026, 3, 30, 12, 0, 0)
TIME_STEP_SECONDS = 1

def clamp(value, low, high):
    return max(low, min(high, value))

def main():
    OUTPUT_FILE.parent.mkdir(parents=True, exist_ok=True)

    # Initial aircraft state
    altitude = 35000.0
    speed = 450.0
    temperature = -50.0
    fuel_level = 85.0
    engine_rpm = 8400.0
    oil_pressure = 58.0
    cabin_pressure = 10.9
    heading = 180.0
    pitch = 0.5
    roll = 0.0
    vertical_speed = 0.0

    current_time = START_TIME
    rows_written = 0

    with OUTPUT_FILE.open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)

        writer.writerow([
            "timestamp",
            "altitude_ft",
            "speed_knots",
            "temperature_c",
            "fuel_level_percent",
            "engine_rpm",
            "oil_pressure_psi",
            "cabin_pressure_psi",
            "heading_deg",
            "pitch_deg",
            "roll_deg",
            "vertical_speed_fpm"
        ])

        while OUTPUT_FILE.stat().st_size < TARGET_SIZE_BYTES:
            # Smooth state updates
            vertical_speed += random.uniform(-120.0, 120.0)
            vertical_speed = clamp(vertical_speed, -1800.0, 1800.0)

            altitude += vertical_speed / 60.0
            altitude = clamp(altitude, 30000.0, 39000.0)

            speed += random.uniform(-2.0, 2.0)
            speed = clamp(speed, 420.0, 480.0)

            temperature += random.uniform(-0.15, 0.15)
            temperature = clamp(temperature, -58.0, -42.0)

            fuel_level -= random.uniform(0.003, 0.02)
            fuel_level = clamp(fuel_level, 0.0, 100.0)

            engine_rpm += random.uniform(-20.0, 20.0)
            engine_rpm = clamp(engine_rpm, 7800.0, 9100.0)

            oil_pressure += random.uniform(-0.25, 0.25)
            oil_pressure = clamp(oil_pressure, 45.0, 70.0)

            cabin_pressure += random.uniform(-0.02, 0.02)
            cabin_pressure = clamp(cabin_pressure, 10.2, 11.4)

            heading += random.uniform(-1.5, 1.5)
            if heading < 0.0:
                heading += 360.0
            elif heading >= 360.0:
                heading -= 360.0

            pitch += random.uniform(-0.12, 0.12)
            pitch = clamp(pitch, -6.0, 6.0)

            roll += random.uniform(-0.2, 0.2)
            roll = clamp(roll, -12.0, 12.0)

            writer.writerow([
                current_time.strftime("%Y-%m-%d %H:%M:%S"),
                f"{altitude:.2f}",
                f"{speed:.2f}",
                f"{temperature:.2f}",
                f"{fuel_level:.2f}",
                f"{engine_rpm:.2f}",
                f"{oil_pressure:.2f}",
                f"{cabin_pressure:.2f}",
                f"{heading:.2f}",
                f"{pitch:.2f}",
                f"{roll:.2f}",
                f"{vertical_speed:.2f}"
            ])

            current_time += timedelta(seconds=TIME_STEP_SECONDS)
            rows_written += 1

            if rows_written % 5000 == 0:
                file.flush()

    print(f"Generated: {OUTPUT_FILE.resolve()}")
    print(f"Size: {OUTPUT_FILE.stat().st_size} bytes")
    print(f"Rows: {rows_written}")

if __name__ == "__main__":
    main()
