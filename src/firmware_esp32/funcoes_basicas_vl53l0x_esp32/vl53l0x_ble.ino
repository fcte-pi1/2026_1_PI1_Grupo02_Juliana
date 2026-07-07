#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "src/vl53l0x.h"

// ---------------------------------------------------------------------------
// Pinos I2C
// ---------------------------------------------------------------------------
#define I2C0_SDA 21
#define I2C0_SCL 22
#define I2C1_SDA 17
#define I2C1_SCL 16

// ---------------------------------------------------------------------------
// UUIDs BLE — devem bater com o index.html
// ---------------------------------------------------------------------------
#define SERVICE_UUID    "0f0e0d0c-0b0a-0908-0706-050403020100"
#define CHAR_ESQ_UUID   "1f1e1d1c-1b1a-1918-1716-151413121110"
#define CHAR_DIR_UUID   "2f2e2d2c-2b2a-2928-2726-252423222120"

// ---------------------------------------------------------------------------
// Objetos globais
// ---------------------------------------------------------------------------
TwoWire I2C_ESQ = TwoWire(0); // i2c0
TwoWire I2C_DIR = TwoWire(1); // i2c1

VL53L0X sensor_esq;
VL53L0X sensor_dir;
bool ok_esq = false;
bool ok_dir = false;

BLEServer          *pServer    = nullptr;
BLECharacteristic  *pCharEsq  = nullptr;
BLECharacteristic  *pCharDir  = nullptr;
bool deviceConnected = false;

// ---------------------------------------------------------------------------
// Callbacks de conexão BLE
// ---------------------------------------------------------------------------
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *s) override {
        deviceConnected = true;
        Serial.println("BLE: cliente conectado.");
    }
    void onDisconnect(BLEServer *s) override {
        deviceConnected = false;
        Serial.println("BLE: cliente desconectado. Reiniciando advertising...");
        BLEDevice::startAdvertising();
    }
};

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(100);

    // I2C sensor esquerdo
    I2C_ESQ.begin(I2C0_SDA, I2C0_SCL, 400000);
    ok_esq = vl53l0x_init(&sensor_esq, &I2C_ESQ, true);
    Serial.printf("Sensor ESQ: %s\n", ok_esq ? "OK" : "FALHOU");

    // I2C sensor direito
    I2C_DIR.begin(I2C1_SDA, I2C1_SCL, 400000);
    ok_dir = vl53l0x_init(&sensor_dir, &I2C_DIR, true);
    Serial.printf("Sensor DIR: %s\n", ok_dir ? "OK" : "FALHOU");

    // Timing budget opcional (descomente se quiser mais precisão)
    // vl53l0x_set_measurement_timing_budget(&sensor_esq, 50000);
    // vl53l0x_set_measurement_timing_budget(&sensor_dir, 50000);

    // BLE
    BLEDevice::init("VL53L0X_Micromouse");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Característica esquerda
    pCharEsq = pService->createCharacteristic(
        CHAR_ESQ_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharEsq->addDescriptor(new BLE2902());

    // Característica direita
    pCharDir = pService->createCharacteristic(
        CHAR_DIR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharDir->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true);
    BLEDevice::startAdvertising();

    Serial.println("BLE advertising iniciado: VL53L0X_Micromouse");
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
    uint16_t dist_esq = ok_esq
        ? vl53l0x_read_range_single_millimeters(&sensor_esq)
        : 0xFFFF;
    uint16_t dist_dir = ok_dir
        ? vl53l0x_read_range_single_millimeters(&sensor_dir)
        : 0xFFFF;

    if (deviceConnected) {
        // Envia como little-endian (igual ao index.html espera)
        uint8_t buf_esq[2] = { (uint8_t)(dist_esq & 0xFF), (uint8_t)(dist_esq >> 8) };
        uint8_t buf_dir[2] = { (uint8_t)(dist_dir & 0xFF), (uint8_t)(dist_dir >> 8) };

        pCharEsq->setValue(buf_esq, 2);
        pCharEsq->notify();
        pCharDir->setValue(buf_dir, 2);
        pCharDir->notify();
    }

    delay(50);
}
