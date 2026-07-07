#include "mpu6050.h"
#include <math.h>

// Update interval in milliseconds (~125 Hz, matches MPU-6050 sample rate)
#define LOOP_MS 8

// Onboard LED pin (GPIO2 on most ESP32 dev boards; change if yours differs)
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

static inline void led_set(bool on)
{
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
}

// Blink LED count times with given on/off durations in ms
static void led_blink(int count, uint32_t on_ms, uint32_t off_ms)
{
    for (int i = 0; i < count; i++) {
        led_set(true);
        delay(on_ms);
        led_set(false);
        delay(off_ms);
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

static GyroBias    bias        = {};
static Orientation orientation = {};
static unsigned long last_time_us;
static bool  led_state  = false;
static int   loop_count = 0;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    delay(2000); // wait for USB serial to connect

    // --- LED test: 3 fast blinks = firmware booted ---
    led_blink(3, 100, 100);

    Serial.println("=== Micromouse MPU-6050 Gyroscope Demo (ESP32) ===");

    // --- Initialise MPU-6050 ---
    if (!mpu6050_init()) {
        Serial.printf("ERROR: MPU-6050 not detected. Check wiring (SDA=GPIO%d, SCL=GPIO%d).\n",
                      MPU6050_SDA_PIN, MPU6050_SCL_PIN);
        led_set(false);
        while (true) {
            delay(1000);
        }
    }

    // LED solid ON during calibration (robot must be stationary)
    led_set(true);

    mpu6050_calibrate(&bias);
    mpu6050_reset_orientation(&orientation);

    // Calibration done: 2 slow blinks = ready to run
    led_set(false);
    led_blink(2, 400, 200);

    Serial.println("Calibration done. Starting orientation loop...\n");
    Serial.printf("%-10s %-10s %-10s | %-10s %s\n",
                  "Yaw(deg)", "Pitch(deg)", "Roll(deg)", "Heading", "Cardinal");

    last_time_us = micros();
}

void loop()
{
    unsigned long now_us = micros();
    float dt = (float)(now_us - last_time_us) * 1e-6f;
    last_time_us = now_us;

    GyroData gyro;
    if (mpu6050_read_gyro(&gyro)) {
        mpu6050_update_orientation(&gyro, &bias, &orientation, dt);

        float heading    = mpu6050_normalise_heading(orientation.yaw);
        CardinalDir card = heading_to_cardinal(heading);

        Serial.printf("%+9.2f  %+9.2f  %+9.2f  | %8.2f   %s\n",
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

    delay(LOOP_MS);
}
