#pragma once

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "Motor.h"

class WatchdogMotor {
private:
    Motor* motorEsq;
    Motor* motorDir;
    absolute_time_t ultimoSinal;
    uint64_t tempoLimiteUs; // microsegundos
    bool sistemaAtivo;

public:
    WatchdogMotor(Motor* esq, Motor* dir, uint32_t timeoutMs = 500);
    void alimentar(); 
    void verificar(); 
    bool isAtivo();
};