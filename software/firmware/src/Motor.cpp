#include "Motor.h"

Motor::Motor(uint8_t pwm, uint8_t dir1, uint8_t dir2) 
    : pinPWM(pwm), pinDir1(dir1), pinDir2(dir2) {}

void Motor::inicializar() {
    pinMode(pinPWM, OUTPUT);
    pinMode(pinDir1, OUTPUT);
    pinMode(pinDir2, OUTPUT);
    parar();
}

void Motor::setVelocidade(int velocidade) {
    if (velocidade > 0) {
        // Frente
        digitalWrite(pinDir1, HIGH);
        digitalWrite(pinDir2, LOW);
        analogWrite(pinPWM, velocidade);
    } else if (velocidade < 0) {
        // Trás
        digitalWrite(pinDir1, LOW);
        digitalWrite(pinDir2, HIGH);
        analogWrite(pinPWM, -velocidade); // Converte para positivo
    } else {
        parar();
    }
}

void Motor::parar() {
    digitalWrite(pinDir1, LOW);
    digitalWrite(pinDir2, LOW);
    analogWrite(pinPWM, 0);
}