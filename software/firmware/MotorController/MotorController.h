#pragma once
#include "../motores/Motor.h"
#include "../motores/PID.h"
#include "../mpu6050/mpu6050.h"

// ──────────────────────────────────────────────────────────────────────────────
// MotorController — abstração dos movimentos do robô
//
// Combina Motor (PWM), PID (correção de heading) e MPU6050 (giroscópio) para
// oferecer movimentos de alto nível: andar uma célula para frente e girar 90°.
//
// Parâmetros de tempo/velocidade DEVEM ser calibrados para o robô físico.
// Com encoders disponíveis no futuro, substituir MC_CELL_TIME_MS por odometria.
// ──────────────────────────────────────────────────────────────────────────────

// ── Parâmetros de movimento (ajustar na calibração) ───────────────────────────
#define MC_BASE_SPEED       150    // PWM base para movimento reto (0..255)
#define MC_TURN_SPEED       120    // PWM para giro (0..255)
#define MC_CELL_TIME_MS     750    // duração para percorrer uma célula (ms)
#define MC_TURN_TIMEOUT_MS  2000   // timeout de segurança para giros (ms)
#define MC_TURN_TARGET_DEG  85.0f  // giro para quando yaw atingir ±85° (≈90°)
#define MC_SETTLE_MS        60     // tempo de repouso após cada movimento (ms)

class MotorController {
public:
    // Recebe referências aos motores e ao PID de heading.
    // PID recomendado para heading: kp=1.5, ki=0.0, kd=0.05, limite=50
    MotorController(Motor& left, Motor& right, PID& pid_heading);

    // Inicializa motores e IMU, executa calibração do giroscópio.
    // Manter o robô parado durante init() (~2 s de calibração).
    void init();

    // Para ambos os motores imediatamente.
    void stop();

    // Avança uma célula para frente, corrigindo deriva com IMU.
    void moveForward();

    // Gira 90° para a esquerda usando feedback do giroscópio.
    void turnLeft();

    // Gira 90° para a direita usando feedback do giroscópio.
    void turnRight();

    // Meia-volta (180°) — executa dois turnRight().
    void turnAround();

    // Controle direto de PWM (usado internamente e para testes).
    void setSpeed(int left_pwm, int right_pwm);

    // Acesso aos dados do IMU para telemetria
    const Orientation& orientation() const;

private:
    Motor& motor_left_;
    Motor& motor_right_;
    PID&   pid_heading_;

    GyroBias    gyro_bias_;
    Orientation orientation_;

    // Atualiza orientação a partir do giroscópio com intervalo dt (segundos).
    void updateImu(float dt);
};
