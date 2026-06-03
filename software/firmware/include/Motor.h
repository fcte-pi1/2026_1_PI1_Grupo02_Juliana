#pragma once
#include <Arduino.h>

class Motor {
private:
    uint8_t pinPWM;
    uint8_t pinDir1;
    uint8_t pinDir2;

public:
    Motor(uint8_t pwm, uint8_t dir1, uint8_t dir2);
    void inicializar();
    void setVelocidade(int velocidade); // Recebe de -255 a 255
    void parar();
};