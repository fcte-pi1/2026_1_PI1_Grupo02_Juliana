#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "mpu6050.h"
#include <stdio.h>
#include <math.h>

// Update interval in milliseconds (~125 Hz, matches MPU-6050 sample rate)
#define LOOP_MS 8

// LED helpers (Pico W onboard LED is on the CYW43 WiFi chip)
static inline void led_set(bool on)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on ? 1 : 0);
}

// Blink LED count times with given on/off durations in ms
static void led_blink(int count, uint32_t on_ms, uint32_t off_ms)
{
    for (int i = 0; i < count; i++) {
        led_set(true);
        sleep_ms(on_ms);
        led_set(false);
        sleep_ms(off_ms);
    }
}

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

    // Initialise CYW43 chip to enable onboard LED on Pico W
    if (cyw43_arch_init()) {
        // LED unavailable — continue without it
    }

    // --- LED test: 3 fast blinks = firmware booted ---
    led_blink(3, 100, 100);

    printf("=== Micromouse MPU-6050 Gyroscope Demo ===\n");

    // --- Initialise MPU-6050 ---
    if (!mpu6050_init()) {
        printf("ERROR: MPU-6050 not detected. Check wiring (SDA=GP4, SCL=GP5).\n");
        // Rapid blink loop = sensor not found (hardware error)
        while (true) {
            led_blink(1, 50, 50);
        }
    }

    // LED solid ON during calibration (robot must be stationary)
    led_set(true);

    GyroBias    bias        = {};
    Orientation orientation = {};
    mpu6050_calibrate(&bias);
    mpu6050_reset_orientation(&orientation);

    // Calibration done: 2 slow blinks = ready to run
    led_set(false);
    led_blink(2, 400, 200);

    printf("Calibration done. Starting orientation loop...\n\n");
    printf("%-10s %-10s %-10s | %-10s %s\n",
           "Yaw(deg)", "Pitch(deg)", "Roll(deg)", "Heading", "Cardinal");

    absolute_time_t last_time = get_absolute_time();
    bool  led_state = false;
    int   loop_count = 0;

    while (true) {
        absolute_time_t now = get_absolute_time();
        float dt = (float)absolute_time_diff_us(last_time, now) * 1e-6f;
        last_time = now;

        GyroData gyro;
        if (mpu6050_read_gyro(&gyro)) {
            mpu6050_update_orientation(&gyro, &bias, &orientation, dt);

            float heading    = mpu6050_normalise_heading(orientation.yaw);
            CardinalDir card = heading_to_cardinal(heading);


                printf("%+9.2f  %+9.2f  %+9.2f  | %8.2f   %s\n",
                       orientation.yaw,
                       orientation.pitch,
                       orientation.roll,
                       heading,
                       dir_name(card));
            

            // Heartbeat: toggle LED every ~125 iterations (~1 Hz)
            // Shows the main loop is running and sensor reads succeed
            loop_count++;
            if (loop_count >= 125) {
                loop_count = 0;
                led_state  = !led_state;
                led_set(led_state);
            }
        }

        sleep_ms(LOOP_MS);
    }

    return 0;
}
