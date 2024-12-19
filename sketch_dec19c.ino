#include <TFT_eSPI.h>
#include <OBD9141.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// การตั้งค่าพิน
#define RX_PIN    16    // K-Line RX
#define TX_PIN    17    // K-Line TX

// สร้างออบเจกต์จอและ K-Line
TFT_eSPI tft = TFT_eSPI();
OBD9141 obd;

// การตั้งค่าคงที่
const int RPM_MAX = 14000;

// ตัวแปร global สำหรับค่าปัจจุบัน
volatile int rpm = 13500, speed = 0, temp = 0;
volatile float volte = 0.0;

void setup() {
  Serial.begin(115200);

  // เริ่มต้นจอ ILI9486
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  drawStaticInterface();

  // เริ่มต้น K-Line
  obd.begin(Serial2, RX_PIN, TX_PIN);

  // สร้าง Task
  xTaskCreatePinnedToCore(taskReadSensors, "Read Sensors Task", 4000, NULL, 1, NULL, 0); // Core 0
  xTaskCreatePinnedToCore(taskUpdateDisplay, "Update Display Task", 4000, NULL, 1, NULL, 1); // Core 1
}

void loop() {
  // ว่าง เพราะงานทั้งหมดอยู่ใน Task
}

// Task 1: อ่านค่าจาก K-Line
void taskReadSensors(void *pvParameters) {
  while (1) {
    // อ่านค่า RPM
    rpm = obd.getCurrentPID(0x0C, rpm) / 4;

    // อ่านค่า Speed
    speed = obd.getCurrentPID(0x0D, speed);

    // อ่านค่า Temperature
    temp = obd.getCurrentPID(0x05, temp) - 40;

    // อ่านค่า Voltage
    volte = obd.getCurrentPID(0x42, volte) / 1000.0;

    vTaskDelay(500 / portTICK_PERIOD_MS); // หน่วงเวลา 500 ms
  }
}

// Task 2: อัพเดตหน้าจอ
void taskUpdateDisplay(void *pvParameters) {
  static int prevRPM = -1, prevSpeed = -1, prevTemp = -1;
  static float prevVolte = -1.0;

  while (1) {
    // ตรวจสอบและอัปเดต RPM
    if (rpm != prevRPM) {
      updateRPM(rpm);
      prevRPM = rpm;
    }

    // ตรวจสอบและอัปเดต Speed
    if (speed != prevSpeed) {
      updateSpeed(speed);
      prevSpeed = speed;
    }

    // ตรวจสอบและอัปเดต Temperature
    if (temp != prevTemp) {
      updateTemp(temp);
      prevTemp = temp;
    }

    // ตรวจสอบและอัปเดต Voltage
    if (volte != prevVolte) {
      updateVolte(volte);
      prevVolte = volte;
    }

    vTaskDelay(250 / portTICK_PERIOD_MS); // หน่วงเวลา 250 ms
  }
}

// ฟังก์ชันวาดอินเทอร์เฟซ
void drawStaticInterface() {
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawLine(30, 90, 390, 40, TFT_WHITE);
  tft.drawLine(30, 100, 390, 100, TFT_WHITE);
  tft.drawLine(390, 40, 450, 30, TFT_RED);
  tft.drawLine(390, 100, 450, 100, TFT_RED);
  tft.drawLine(30, 100, 30, 90, TFT_WHITE);
  tft.drawLine(450, 100, 450, 30, TFT_RED);
}

// ฟังก์ชันอัปเดต RPM
void updateRPM(int rpmValue) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(200, 200);
  tft.printf(" %d RPM   ", rpmValue); // เพิ่มช่องว่างเพื่อเคลียร์ข้อความเก่า
  drawRPMBar(30, 100, 200, rpmValue, RPM_MAX);
}

// ฟังก์ชันวาดแถบ RPM
void drawRPMBar(int x, int y, int length, int rpmValue, int rpmMax) {
  static int prevFilledLength = 0;
  int filledLength = map(rpmValue, 0, rpmMax, 0, length);

  if (filledLength != prevFilledLength) {
    // ลบเฉพาะส่วนที่ลดลง
    if (filledLength < prevFilledLength) {
      tft.fillRect(x + filledLength, y - 20, prevFilledLength - filledLength, 20, TFT_BLACK);
    }
    // วาดเฉพาะส่วนที่เพิ่มขึ้น
    if (filledLength > prevFilledLength) {
      tft.fillRect(x + prevFilledLength, y - 20, filledLength - prevFilledLength, 20, TFT_RED);
    }
    prevFilledLength = filledLength;
  }
}

// ฟังก์ชันอัปเดต Speed
void updateSpeed(int speedValue) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(200, 150);
  tft.printf(" %d KM/H   ", speedValue); // เพิ่มช่องว่างเพื่อเคลียร์ข้อความเก่า
}

// ฟังก์ชันอัปเดต Temp
void updateTemp(int tempValue) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 280);
  tft.printf("Temp: %dC   ", tempValue); // เพิ่มช่องว่างเพื่อเคลียร์ข้อความเก่า
}

// ฟังก์ชันอัปเดต Voltage
void updateVolte(float volteValue) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(150, 280);
  tft.printf("Volt: %.1fV   ", volteValue); // เพิ่มช่องว่างเพื่อเคลียร์ข้อความเก่า
}
