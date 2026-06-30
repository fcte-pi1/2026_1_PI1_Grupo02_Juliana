#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

constexpr uint TRIG_PIN             = 15;
constexpr uint ECHO_PIN             = 14;
constexpr float VELOCIDADE_SOM_US_CM = 58.0f;
constexpr uint32_t TIMEOUT_US       = 30000;  // 30ms = ~400cm

class SensorHCSR04 {
public:
    SensorHCSR04() {
        stdio_init_all();

        gpio_init(TRIG_PIN);
        gpio_init(ECHO_PIN);
        gpio_set_dir(TRIG_PIN, GPIO_OUT);
        gpio_set_dir(ECHO_PIN, GPIO_IN);

        gpio_put(TRIG_PIN, 0);
        sleep_ms(500);
    }

    float medirDistancia() {
        dispararBurst();

        if (!aguardarEchoHigh())
            return -1.0f;

        return calcularDistancia();
    }

private:
    void dispararBurst() {
        gpio_put(TRIG_PIN, 0);
        sleep_us(2);
        gpio_put(TRIG_PIN, 1);
        sleep_us(10);   // 10µs conforme datasheet
        gpio_put(TRIG_PIN, 0);
    }

    bool aguardarEchoHigh() {
        uint32_t inicio = time_us_32();
        while (gpio_get(ECHO_PIN) == 0) {
            if ((time_us_32() - inicio) > TIMEOUT_US) {
                printf("Timeout aguardando ECHO HIGH\n");
                return false;
            }
        }
        return true;
    }

    float calcularDistancia() {
        uint32_t inicio = time_us_32();
        while (gpio_get(ECHO_PIN) == 1) {
            if ((time_us_32() - inicio) > TIMEOUT_US) {
                printf("Timeout aguardando ECHO LOW\n");
                return -1.0f;
            }
        }
        uint32_t fim = time_us_32();

        float distancia = (float)(fim - inicio) / VELOCIDADE_SOM_US_CM;

        if (distancia < 2.0f || distancia > 400.0f) {
            printf("Fora do range: %.2f cm\n", distancia);
            return -1.0f;
        }

        return distancia;
    }
};

int main() {
    SensorHCSR04 sensor;

    while (true) {
        float distancia = sensor.medirDistancia();
        if (distancia >= 0)
            printf("Distancia: %.2f cm\n", distancia);
        sleep_ms(100);
    }

    return 0;
}