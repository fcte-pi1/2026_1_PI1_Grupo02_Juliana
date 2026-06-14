#pragma once

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

class Motor {
private:
    uint pinPWM;
    uint pinDir1;
    uint pinDir2;
    uint pwm_slice_num;

public:
    Motor(uint pwm, uint dir1, uint dir2);
    void inicializar();
    void setVelocidade(int velocidade); // Recebe de -255 a 255
    void parar();
};