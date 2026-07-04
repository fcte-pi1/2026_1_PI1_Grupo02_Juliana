#include "SensorManager.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

SensorManager::SensorManager(uint32_t pin_front, uint32_t pin_left, uint32_t pin_right)
    : pin_front_(pin_front),
      pin_left_(pin_left),
      pin_right_(pin_right),
      raw_front_(0),
      raw_left_(0),
      raw_right_(0),
      threshold_(SENSOR_WALL_THRESHOLD_RAW) {}

void SensorManager::init() {
    adc_init();
    // Pinos GP26..GP28 mapeiam para ADC0..ADC2 no Pico
    adc_gpio_init(pin_front_);
    adc_gpio_init(pin_left_);
    adc_gpio_init(pin_right_);
}

// ──────────────────────────────────────────────────────────────────────────────
// Lê o canal ADC correspondente ao pino GPIO.
// Pino → canal: GP26=0, GP27=1, GP28=2, GP29=3
// Sensores Sharp GP2Y0A21: tensão AUMENTA quando objeto fica mais próximo.
// Portanto raw_front_ > threshold_ indica parede presente.
// ──────────────────────────────────────────────────────────────────────────────
uint16_t SensorManager::readAdc(uint32_t gpio) const {
    adc_select_input(gpio - 26u);
    // Média de 4 amostras para reduzir ruído ADC
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++) sum += adc_read();
    return (uint16_t)(sum >> 2);
}

void SensorManager::update() {
    raw_front_ = readAdc(pin_front_);
    raw_left_  = readAdc(pin_left_);
    raw_right_ = readAdc(pin_right_);
}

bool SensorManager::hasWallFront() const { return raw_front_ >= threshold_; }
bool SensorManager::hasWallLeft()  const { return raw_left_  >= threshold_; }
bool SensorManager::hasWallRight() const { return raw_right_ >= threshold_; }

uint16_t SensorManager::rawFront() const { return raw_front_; }
uint16_t SensorManager::rawLeft()  const { return raw_left_;  }
uint16_t SensorManager::rawRight() const { return raw_right_; }

void SensorManager::setThreshold(uint16_t threshold) { threshold_ = threshold; }
