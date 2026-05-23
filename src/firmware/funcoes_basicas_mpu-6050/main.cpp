#include "pico/stdlib.h"
#include "mpu6050.h"
#include <stdio.h>
#include <math.h>

// Update interval in milliseconds
#define LOOP_MS 8   // ~125 Hz, matches the MPU-6050 sample rate

// Heading threshold in degrees to consider the robot aligned to a cardinal direction
#define HEADING_THRESHOLD_DEG 2.0f

// Cardinal directions for maze navigation
typedef enum {
    DIR_NORTH = 0,
    DIR_EAST  = 90,
    DIR_SOUTH = 180,
    DIR_WEST  = 270
} CardinalDir;

static const char *dir_name(CardinalDir d)
{
    switch (d) {
        case DIR_NORTH: return "NORTH";
        case DIR_EAST:  return "EAST";
        case DIR_SOUTH: return "SOUTH";
        case DIR_WEST:  return "WEST";
        default:        return "UNKNOWN";
    }
}

// Return the cardinal direction closest to the current heading
static CardinalDir heading_to_cardinal(float heading_deg)
{
    float h = mpu6050_normalise_heading(heading_deg);
    if (h < 45.0f || h >= 315.0f) return DIR_NORTH;
    if (h < 135.0f)               return DIR_EAST;
    if (h < 225.0f)               return DIR_SOUTH;
    return DIR_WEST;
}

int main(void)
{
    stdio_init_all();
    sleep_ms(2000); // wait for USB serial to connect

    printf("=== Micromouse MPU-6050 Gyroscope Demo ===\n");

    // --- Initialise MPU-6050 ---
    if (!mpu6050_init()) {
        printf("ERROR: MPU-6050 not detected. Check wiring.\n");
        while (true) tight_loop_contents();
    }

    // --- Calibrate (robot must be stationary) ---
    GyroBias   bias        = {0};
    Orientation orientation = {0};
    mpu6050_calibrate(&bias);
    mpu6050_reset_orientation(&orientation);

    printf("Calibration done. Starting orientation loop...\n\n");

    absolute_time_t last_time = get_absolute_time();

    while (true) {
        absolute_time_t now = get_absolute_time();
        float dt = (float)absolute_time_diff_us(last_time, now) * 1e-6f;
        last_time = now;

        GyroData gyro;
        if (mpu6050_read_gyro(&gyro)) {
            mpu6050_update_orientation(&gyro, &bias, &orientation, dt);

            float heading = mpu6050_normalise_heading(orientation.yaw);
            CardinalDir cardinal = heading_to_cardinal(heading);

            printf("Yaw: %7.2f°  Pitch: %6.2f°  Roll: %6.2f°  | Heading: %6.2f° (%s)\n",
                   orientation.yaw,
                   orientation.pitch,
                   orientation.roll,
                   heading,
                   dir_name(cardinal));
        }

        sleep_ms(LOOP_MS);
    }

    return 0;
}
