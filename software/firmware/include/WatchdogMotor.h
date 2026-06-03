#pragma once
#include <Arduino.h>
#include "Motor.h"

class WatchdogMotor {
private:
    Motor* motorEsq;
    Motor* motorDir;
    unsigned long ultimoSinal;
    unsigned long tempoLimite;
    bool sistemaAtivo;

public:
    WatchdogMotor(Motor* esq, Motor* dir, unsigned long timeoutMs = 500);
    void alimentar(); // O sistema principal chama isso para dizer que está vivo
    void verificar(); // Roda no loop para checar o tempo
    bool isAtivo();
};