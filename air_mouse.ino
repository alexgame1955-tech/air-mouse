#include <Wire.h>
#include "USB.h"
#include "USBHIDMouse.h"

USBHIDMouse Mouse;

// ========== ПИНЫ ==========
#define I2C_SDA 8
#define I2C_SCL 9
#define CLUTCH_PIN 1      // Сенсор на GPIO1 — зажал = курсор едет
#define LEFT_PIN 2        // Левый клик (GPIO2)
#define RIGHT_PIN 3       // Правый клик (GPIO3)

#define MPU6050_ADDR 0x68

// ========== ПЕРЕМЕННЫЕ ГИРОСКОПА ==========
int16_t GyX, GyY, GyZ;
float gyroXoffset = 0, gyroYoffset = 0;
float angleX = 0, angleY = 0;
unsigned long lastTime = 0;

// ========== НАСТРОЙКИ ==========
float sensitivity = 1.5;      // Чувствительность
bool invertY = true;          // Инверсия оси Y

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(CLUTCH_PIN, INPUT);
  pinMode(LEFT_PIN, INPUT);
  pinMode(RIGHT_PIN, INPUT);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  
  // Пробуждение MPU6050
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  delay(100);
  
  // Калибровка гироскопа (500 выборок)
  Serial.println("CALIBRATING GYRO... DO NOT MOVE");
  long sumX = 0, sumY = 0;
  for (int i = 0; i < 500; i++) {
    readGyro();
    sumX += GyX;
    sumY += GyY;
    delay(3);
  }
  gyroXoffset = sumX / 500;
  gyroYoffset = sumY / 500;
  
  Serial.print("Offsets: X="); Serial.print(gyroXoffset);
  Serial.print(" Y="); Serial.println(gyroYoffset);
  
  USB.begin();
  Mouse.begin();
  
  lastTime = micros();
  Serial.println("=== HEISENBERG AIR MOUSE (3 BUTTONS) ===");
  Serial.println("GPIO1: Clutch | GPIO2: Left Click | GPIO3: Right Click");
}

void loop() {
  bool clutch = (digitalRead(CLUTCH_PIN) == HIGH);
  bool left   = (digitalRead(LEFT_PIN) == HIGH);
  bool right  = (digitalRead(RIGHT_PIN) == HIGH);
  
  // === ДВИЖЕНИЕ КУРСОРА (только когда Clutch зажат) ===
  if (clutch) {
    unsigned long now = micros();
    float dt = (now - lastTime) / 1000000.0;
    lastTime = now;
    if (dt > 0.05) dt = 0.02;
    
    readGyro();
    
    float gyroXrate = (GyX - gyroXoffset) / 131.0;
    float gyroYrate = (GyY - gyroYoffset) / 131.0;
    
    angleX += gyroXrate * dt;
    angleY += gyroYrate * dt;
    
    int moveX = constrain(angleY * sensitivity, -127, 127);
    int moveY = constrain(angleX * sensitivity, -127, 127);
    
    if (invertY) moveY = -moveY;
    
    if (abs(moveX) > 0 || abs(moveY) > 0) {
      Mouse.move(moveX, moveY);
    }
  } else {
    // Clutch не зажат — сбрасываем углы, курсор стоит на месте
    angleX = 0;
    angleY = 0;
  }
  
  // === ЛЕВЫЙ КЛИК (GPIO2) ===
  if (left) {
    Mouse.click(MOUSE_LEFT);
    delay(150);
  }
  
  // === ПРАВЫЙ КЛИК (GPIO3) ===
  if (right) {
    Mouse.click(MOUSE_RIGHT);
    delay(150);
  }
  
  delay(10);
}

void readGyro() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true);
  
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();
}
