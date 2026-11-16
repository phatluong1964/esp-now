#include <esp_now.h>
#include <WiFi.h>

// =====================================
// LED BÁO LIÊN KẾT ESP-NOW
// =====================================
const int ESP_NOW_LED = 13;

// =====================================
// MOTOR A
// =====================================
const int motorAPin1 = 27;
const int motorAPin2 = 26;
const int enableAPin = 14;

// =====================================
// MOTOR B
// =====================================
const int motorBPin1 = 25;
const int motorBPin2 = 33;
const int enableBPin = 32;

// =====================================
// TỐC ĐỘ
// =====================================
const int FIXED_SPEED = 255;

// Nếu mất tín hiệu quá 500ms thì dừng xe
const unsigned long SIGNAL_TIMEOUT = 500;

// =====================================
// ESP-NOW DATA
// Phải giống hệt bên vòng tay
// =====================================
typedef struct struct_message {
  char command;
} struct_message;

struct_message receiveData;

// Lệnh hiện tại
volatile char currentCommand = 'S';

// Thời gian nhận gói gần nhất
volatile unsigned long lastPacketTime = 0;

// Đã từng nhận được gói ESP-NOW hay chưa
volatile bool hasReceivedPacket = false;


// =====================================
// CALLBACK NHẬN ESP-NOW
// ESP32 Core 3.x
// =====================================
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(struct_message)) {
    return;
  }

  memcpy(&receiveData, incomingData, sizeof(receiveData));

  char cmd = receiveData.command;

  // Chỉ nhận lệnh hợp lệ
  if (cmd == 'F' ||
      cmd == 'B' ||
      cmd == 'L' ||
      cmd == 'R' ||
      cmd == 'S') {

    currentCommand = cmd;

    // Cập nhật thời gian nhận gói gần nhất
    lastPacketTime = millis();

    // Xác nhận đã có dữ liệu ESP-NOW hợp lệ
    hasReceivedPacket = true;

    Serial.print("Nhan lenh: ");
    Serial.println(cmd);
  }
}


// =====================================
// ĐIỀU KHIỂN MOTOR
// =====================================
void setMotor(int enablePin,
              int in1,
              int in2,
              int motorSpeed,
              int direction) {

  int pwm = constrain(abs(motorSpeed), 0, 255);

  if (direction == 1) {

    // Tiến
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

    analogWrite(enablePin, pwm);
  }

  else if (direction == -1) {

    // Lùi
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);

    analogWrite(enablePin, pwm);
  }

  else {

    // Dừng
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);

    analogWrite(enablePin, 0);
  }
}


// =====================================
// TIẾN
// =====================================
void moveForward() {

  setMotor(
    enableAPin,
    motorAPin1,
    motorAPin2,
    FIXED_SPEED,
    1
  );

  setMotor(
    enableBPin,
    motorBPin1,
    motorBPin2,
    FIXED_SPEED,
    1
  );
}


// =====================================
// LÙI
// =====================================
void moveBackward() {

  setMotor(
    enableAPin,
    motorAPin1,
    motorAPin2,
    FIXED_SPEED,
    -1
  );

  setMotor(
    enableBPin,
    motorBPin1,
    motorBPin2,
    FIXED_SPEED,
    -1
  );
}


// =====================================
// TRÁI
// =====================================
void turnLeft() {

  setMotor(
    enableAPin,
    motorAPin1,
    motorAPin2,
    FIXED_SPEED,
    -1
  );

  setMotor(
    enableBPin,
    motorBPin1,
    motorBPin2,
    FIXED_SPEED,
    1
  );
}


// =====================================
// PHẢI
// =====================================
void turnRight() {

  setMotor(
    enableAPin,
    motorAPin1,
    motorAPin2,
    FIXED_SPEED,
    1
  );

  setMotor(
    enableBPin,
    motorBPin1,
    motorBPin2,
    FIXED_SPEED,
    -1
  );
}


// =====================================
// DỪNG
// =====================================
void stopCar() {

  setMotor(
    enableAPin,
    motorAPin1,
    motorAPin2,
    0,
    0
  );

  setMotor(
    enableBPin,
    motorBPin1,
    motorBPin2,
    0,
    0
  );
}


// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);

  // -----------------------------
  // LED ESP-NOW
  // -----------------------------
  pinMode(ESP_NOW_LED, OUTPUT);

  // Ban đầu chưa có liên kết
  digitalWrite(ESP_NOW_LED, LOW);

  // -----------------------------
  // MOTOR
  // -----------------------------
  pinMode(motorAPin1, OUTPUT);
  pinMode(motorAPin2, OUTPUT);
  pinMode(enableAPin, OUTPUT);

  pinMode(motorBPin1, OUTPUT);
  pinMode(motorBPin2, OUTPUT);
  pinMode(enableBPin, OUTPUT);

  // Bắt đầu phải dừng
  stopCar();

  // -----------------------------
  // WIFI
  // -----------------------------
  WiFi.mode(WIFI_STA);

  delay(100);

  Serial.println();
  Serial.println("============================");
  Serial.println("XE DIEU KHIEN ESP-NOW");
  Serial.println("============================");

  Serial.print("MAC XE: ");
  Serial.println(WiFi.macAddress());

  // -----------------------------
  // ESP-NOW
  // -----------------------------
  if (esp_now_init() != ESP_OK) {

    Serial.println("LOI KHOI TAO ESP-NOW");

    digitalWrite(ESP_NOW_LED, LOW);

    stopCar();

    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW READY");
  Serial.println("Dang cho lenh tu vong tay...");
}


// =====================================
// LOOP
// =====================================
void loop() {

  // =================================
  // CHƯA TỪNG NHẬN GÓI ESP-NOW
  // =================================
  if (!hasReceivedPacket) {

    digitalWrite(ESP_NOW_LED, LOW);

    currentCommand = 'S';

    stopCar();

    delay(10);

    return;
  }


  // =================================
  // FAILSAFE
  // Mất tín hiệu quá 500ms -> STOP
  // LED ESP-NOW -> TẮT
  // =================================
  if (millis() - lastPacketTime > SIGNAL_TIMEOUT) {

    digitalWrite(ESP_NOW_LED, LOW);

    currentCommand = 'S';

    stopCar();

    delay(10);

    return;
  }


  // =================================
  // ESP-NOW ĐANG HOẠT ĐỘNG
  // Có gói nhận đều -> LED SÁNG
  // =================================
  digitalWrite(ESP_NOW_LED, HIGH);


  // Lấy lệnh hiện tại
  char command = currentCommand;


  // =================================
  // XỬ LÝ LỆNH
  // =================================
  switch (command) {

    case 'F':

      moveForward();

      break;


    case 'B':

      moveBackward();

      break;


    case 'L':

      turnLeft();

      break;


    case 'R':

      turnRight();

      break;


    case 'S':

      stopCar();

      break;


    default:

      stopCar();

      break;
  }

  delay(5);
}