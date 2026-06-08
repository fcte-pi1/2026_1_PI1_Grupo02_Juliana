#include "WatchdogMotor.h"

WatchdogMotor::WatchdogMotor(Motor* esq, Motor* dir, uint32_t timeoutMs)
    : motorEsq(esq), motorDir(dir), sistemaAtivo(true) {
    ultimoSinal = get_absolute_time();
    tempoLimiteUs = (uint64_t)timeoutMs * 1000; // Converte ms para microsegundos
}

void WatchdogMotor::alimentar() {
    ultimoSinal = get_absolute_time();
    sistemaAtivo = true;
}

void WatchdogMotor::verificar() {
    absolute_time_t agora = get_absolute_time();
    uint64_t tempo_decorrido = absolute_time_diff_us(ultimoSinal, agora);
    
    if (sistemaAtivo && (tempo_decorrido > tempoLimiteUs)) {
        // Timeout! Corta os motores imediatamente.
        motorEsq->parar();
        motorDir->parar();
        sistemaAtivo = false;
        printf("[CRITICO] Watchdog desarmou os motores!\r\n");
    }
}

bool WatchdogMotor::isAtivo() {
    return sistemaAtivo;
}