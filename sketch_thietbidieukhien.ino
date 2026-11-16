#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050_tockn.h>

// ======================================================
// MPU6050
// ======================================================
const int SDA_PIN = 27;
const int SCL_PIN = 26;

MPU6050 mpu6050(Wire);

// ======================================================
// MAC ESP32 XE
// 58:2A:BD:7C:ED:04
// ======================================================
uint8_t carMAC[] = {
  0x58, 0x2A, 0xBD,
  0x7C, 0xED, 0x04
};

// ======================================================
// CẤU TRÚC DỮ LIỆU ESP-NOW
// ======================================================
typedef struct struct_message {
  char command;
} struct_message;

struct_message dataSend;
esp_now_peer_info_t peerInfo;

// ======================================================
// GÓC GỐC
// ======================================================
float baseX = 0.0;
float baseY = 0.0;

// ======================================================
// NGƯỠNG ĐIỀU KHIỂN
// ======================================================

// Vùng trung tâm an toàn
const float STOP_X = 15.0;
const float STOP_Y = 15.0;

// Ngưỡng bắt đầu chạy
const float MOVE_X = 25.0;
const float MOVE_Y = 25.0;

// ======================================================
// CALLBACK ESP-NOW
// ESP32 Core 3.x
// ======================================================
void OnDataSent(const wifi_tx_info_t *info,
                esp_now_send_status_t status) {

  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.print(" | SEND OK");
  } else {
    Serial.print(" | SEND FAIL");
  }
}

// ======================================================
// SETUP
// ======================================================
void setup() {

  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("VONG TAY DIEU KHIEN ESP-NOW");
  Serial.println("================================");

  // ==================================================
  // MPU6050
  // ==================================================
  Wire.begin(SDA_PIN, SCL_PIN);

  mpu6050.begin();

  Serial.println();
  Serial.println("DAT VONG TAY O TU THE THANG");
  Serial.println("GIU THAT YEN TRONG LUC CALIB");
  Serial.println();

  delay(1000);

  // Hiệu chỉnh gyro
  mpu6050.calcGyroOffsets(true);

  delay(1000);

  // ==================================================
  // CHỜ MPU ỔN ĐỊNH
  // ==================================================
  Serial.println("CHO MPU ON DINH...");

  for (int i = 0; i < 200; i++) {
    mpu6050.update();
    delay(10);
  }

  // ==================================================
  // LẤY GỐC TRUNG BÌNH 200 MẪU
  // ==================================================
  Serial.println("DANG LAY GOC TRUNG TAM...");

  float sumX = 0.0;
  float sumY = 0.0;

  for (int i = 0; i < 200; i++) {

    mpu6050.update();

    sumX += mpu6050.getAngleX();
    sumY += mpu6050.getAngleY();

    delay(10);
  }

  baseX = sumX / 200.0;
  baseY = sumY / 200.0;

  Serial.println();
  Serial.println("DA LAY GOC");

  Serial.print("Base X = ");
  Serial.println(baseX, 2);

  Serial.print("Base Y = ");
  Serial.println(baseY, 2);

  // ==================================================
  // WIFI
  // ==================================================
  WiFi.mode(WIFI_STA);

  delay(100);

  Serial.print("MAC VONG TAY: ");
  Serial.println(WiFi.macAddress());

  // ==================================================
  // ESP-NOW
  // ==================================================
  if (esp_now_init() != ESP_OK) {

    Serial.println("LOI KHOI TAO ESP-NOW!");

    return;
  }

  esp_now_register_send_cb(OnDataSent);

  // ==================================================
  // THÊM XE LÀM PEER
  // ==================================================
  memset(&peerInfo, 0, sizeof(peerInfo));

  memcpy(peerInfo.peer_addr, carMAC, 6);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {

    Serial.println("LOI THEM PEER!");

    return;
  }

  Serial.println();
  Serial.println("ESP-NOW READY");
  Serial.println("XE MAC: 58:2A:BD:7C:ED:04");

  Serial.println();
  Serial.println("--------------------------------");
  Serial.println("S: |X| <= 15 va |Y| <= 15");
  Serial.println("F: Y <= -25");
  Serial.println("B: Y >= +25");
  Serial.println("L: X >= +25");
  Serial.println("R: X <= -25");
  Serial.println("--------------------------------");
}

// ======================================================
// LOOP
// ======================================================
void loop() {

  mpu6050.update();

  // ==================================================
  // GÓC TƯƠNG ĐỐI SO VỚI TƯ THẾ THẲNG
  // ==================================================
  float x = mpu6050.getAngleX() - baseX;
  float y = mpu6050.getAngleY() - baseY;

  char command = 'S';

  // ==================================================
  // ƯU TIÊN DỪNG
  // ĐƯA TAY VỀ THẲNG -> S NGAY
  // ==================================================
  if (abs(x) <= STOP_X &&
      abs(y) <= STOP_Y) {

    command = 'S';
  }

  // ==================================================
  // TIẾN
  // ==================================================
  else if (y <= -MOVE_Y) {

    command = 'F';
  }

  // ==================================================
  // LÙI
  // ==================================================
  else if (y >= MOVE_Y) {

    command = 'B';
  }

  // ==================================================
  // TRÁI
  // ==================================================
  else if (x >= MOVE_X) {

    command = 'L';
  }

  // ==================================================
  // PHẢI
  // ==================================================
  else if (x <= -MOVE_X) {

    command = 'R';
  }

  // ==================================================
  // VÙNG ĐỆM 15 -> 25 ĐỘ
  // GIỮ STOP
  // ==================================================
  else {

    command = 'S';
  }

  // ==================================================
  // GỬI ESP-NOW
  // ==================================================
  dataSend.command = command;

  esp_err_t result = esp_now_send(
    carMAC,
    (uint8_t *)&dataSend,
    sizeof(dataSend)
  );

  // ==================================================
  // SERIAL
  // ==================================================
  Serial.print("X: ");
  Serial.print(x, 1);

  Serial.print(" | Y: ");
  Serial.print(y, 1);

  Serial.print(" | CMD: ");
  Serial.print(command);

  if (result != ESP_OK) {
    Serial.print(" | SEND ERROR");
  }

  Serial.println();

  // Gửi nhanh để STOP phản ứng gần như ngay lập tức
  delay(30);
}