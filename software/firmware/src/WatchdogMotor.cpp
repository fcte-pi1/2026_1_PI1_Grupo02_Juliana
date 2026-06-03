#include "WatchdogMotor.h"

WatchdogMotor::WatchdogMotor(Motor* esq, Motor* dir, unsigned long timeoutMs)
    : motorEsq(esq), motorDir(dir), tempoLimite(timeoutMs), sistemaAtivo(true) {
    ultimoSinal = millis();
}

void WatchdogMotor::alimentar() {
    ultimoSinal = millis();
    sistemaAtivo = true;
}

void WatchdogMotor::verificar() {
    if (sistemaAtivo && (millis() - ultimoSinal > tempoLimite)) {
        // Timeout! Corta os motores imediatamente.
        motorEsq->parar();
        motorDir->parar();
        sistemaAtivo = false;
        Serial.println("[CRITICO] Watchdog desarmou os motores!");
    }
}

bool WatchdogMotor::isAtivo() {
    return sistemaAtivo;
}