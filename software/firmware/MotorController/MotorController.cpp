#include "MotorController.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

MotorController::MotorController(Motor& left, Motor& right, PID& pid_heading)
    : motor_left_(left), motor_right_(right), pid_heading_(pid_heading) {}

// ──────────────────────────────────────────────────────────────────────────────

void MotorController::init() {
    motor_left_.inicializar();
    motor_right_.inicializar();

    if (!mpu6050_init()) {
        printf("[MC] AVISO: MPU6050 nao inicializado — movimentos sem correcao IMU\r\n");
    }

    printf("[MC] Calibrando giroscopio — mantenha o robo parado...\r\n");
    mpu6050_calibrate(&gyro_bias_);
    mpu6050_reset_orientation(&orientation_);
    printf("[MC] Calibracao concluida\r\n");
}

void MotorController::stop() {
    motor_left_.parar();
    motor_right_.parar();
}

void MotorController::setSpeed(int left_pwm, int right_pwm) {
    motor_left_.setVelocidade(left_pwm);
    motor_right_.setVelocidade(right_pwm);
}

const Orientation& MotorController::orientation() const {
    return orientation_;
}

// ──────────────────────────────────────────────────────────────────────────────

void MotorController::updateImu(float dt) {
    GyroData gyro;
    if (mpu6050_read_gyro(&gyro)) {
        mpu6050_update_orientation(&gyro, &gyro_bias_, &orientation_, dt);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Movimento reto: usa PID sobre o yaw para manter direção.
//
// Convenção de sinal (verificar com o IMU montado no robô):
//   yaw > 0 → robô desviou para a esquerda (anti-horário visto de cima)
//   correction > 0 → motor esquerdo mais rápido → corrige para direita
// ──────────────────────────────────────────────────────────────────────────────

void MotorController::moveForward() {
    mpu6050_reset_orientation(&orientation_);
    pid_heading_.resetar();

    uint32_t start = to_ms_since_boot(get_absolute_time());
    uint32_t last  = start;

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - start) >= MC_CELL_TIME_MS) break;

        float dt = (now - last) * 0.001f;
        if (dt < 0.001f) dt = 0.001f;  // evita divisão por zero
        last = now;

        updateImu(dt);

        float correction = pid_heading_.calcular(0.0f, orientation_.yaw, dt);

        int l = (int)(MC_BASE_SPEED + correction);
        int r = (int)(MC_BASE_SPEED - correction);
        // Clamp para intervalo válido do Motor
        if (l > 255) l = 255; else if (l < 0) l = 0;
        if (r > 255) r = 255; else if (r < 0) r = 0;

        setSpeed(l, r);
        sleep_ms(10);
    }

    stop();
    sleep_ms(MC_SETTLE_MS);
}

// ──────────────────────────────────────────────────────────────────────────────
// Giro para a esquerda: motor esq. para trás, motor dir. para frente.
// Para quando yaw atingir +MC_TURN_TARGET_DEG (anti-horário = yaw positivo).
// Timeout de segurança evita loop infinito se IMU falhar.
// ──────────────────────────────────────────────────────────────────────────────

void MotorController::turnLeft() {
    mpu6050_reset_orientation(&orientation_);

    uint32_t start = to_ms_since_boot(get_absolute_time());
    uint32_t last  = start;

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        float dt = (now - last) * 0.001f;
        if (dt < 0.001f) dt = 0.001f;
        last = now;

        updateImu(dt);

        if (orientation_.yaw >= MC_TURN_TARGET_DEG) break;
        if ((now - start) >= MC_TURN_TIMEOUT_MS) {
            printf("[MC] AVISO: timeout no turnLeft (yaw=%.1f)\r\n", orientation_.yaw);
            break;
        }

        // Motor esquerdo negativo (ré), motor direito positivo (frente)
        setSpeed(-MC_TURN_SPEED, MC_TURN_SPEED);
        sleep_ms(5);
    }

    stop();
    sleep_ms(MC_SETTLE_MS);
}

// ──────────────────────────────────────────────────────────────────────────────
// Giro para a direita: motor esq. para frente, motor dir. para trás.
// Para quando yaw atingir -MC_TURN_TARGET_DEG (horário = yaw negativo).
// ──────────────────────────────────────────────────────────────────────────────

void MotorController::turnRight() {
    mpu6050_reset_orientation(&orientation_);

    uint32_t start = to_ms_since_boot(get_absolute_time());
    uint32_t last  = start;

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        float dt = (now - last) * 0.001f;
        if (dt < 0.001f) dt = 0.001f;
        last = now;

        updateImu(dt);

        if (orientation_.yaw <= -MC_TURN_TARGET_DEG) break;
        if ((now - start) >= MC_TURN_TIMEOUT_MS) {
            printf("[MC] AVISO: timeout no turnRight (yaw=%.1f)\r\n", orientation_.yaw);
            break;
        }

        // Motor esquerdo positivo (frente), motor direito negativo (ré)
        setSpeed(MC_TURN_SPEED, -MC_TURN_SPEED);
        sleep_ms(5);
    }

    stop();
    sleep_ms(MC_SETTLE_MS);
}

void MotorController::turnAround() {
    turnRight();
    turnRight();
}
