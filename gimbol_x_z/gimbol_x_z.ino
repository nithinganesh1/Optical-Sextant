#include <Wire.h>
#include <math.h>
#include <7Semi_ISM330DHCX.h>

ISM330DHCX_7Semi imu;

// =============================================
// MOTOR PINS
// =============================================

// ROLL - external TB6600
#define ROLL_STEP_PIN 12
#define ROLL_DIR_PIN  13

// PITCH motor 1 - CNC Shield V3, Y axis
#define PITCH1_STEP_PIN 3
#define PITCH1_DIR_PIN  6

// PITCH motor 2 - CNC Shield V3, Z axis
#define PITCH2_STEP_PIN 4
#define PITCH2_DIR_PIN  7

// CNC Shield V3 common enable (active LOW)
#define CNC_ENABLE_PIN 8

#define NO_PIN 255

const bool ROLL_REVERSE   = true;
const bool PITCH1_REVERSE = false;  // flip if pitch runs away
const bool PITCH2_REVERSE = false;  // flip if motor 2 turns opposite to motor 1

// =============================================
// IMU FILTER
// =============================================

const float RAD_TO_DEG_F = 57.2957795f;
const float ALPHA = 0.98f;

const float ROLL_DEADBAND  = 2.0f;
const float PITCH_DEADBAND = 2.0f;

// =============================================
// PD CONTROLLERS
// =============================================

const float ROLL_KP = 12.0f;
const float ROLL_KD = 2.0f;

const float PITCH_KP = 12.0f;
const float PITCH_KD = 2.0f;

const float MAX_STEP_RATE = 500.0f;
const float MIN_STEP_RATE = 25.0f;

const unsigned int STEP_PULSE_US = 5;

// =============================================
// TIMING
// =============================================

const unsigned long IMU_INTERVAL_US    = 10000;
const unsigned long SERIAL_INTERVAL_MS = 100;

// =============================================
// STEPPER AXIS (1 or 2 motors stepped together)
// =============================================

struct StepperAxis {
  uint8_t stepPin1;
  uint8_t dirPin1;
  bool    reverse1;
  uint8_t stepPin2;     // NO_PIN if only one motor
  uint8_t dirPin2;
  bool    reverse2;
  bool    pulseHigh;
  unsigned long pulseStartMicros;
  unsigned long lastStepMicros;
};

StepperAxis rollMotor = {
  ROLL_STEP_PIN, ROLL_DIR_PIN, ROLL_REVERSE,
  NO_PIN, NO_PIN, false,
  false, 0, 0
};

StepperAxis pitchMotor = {
  PITCH1_STEP_PIN, PITCH1_DIR_PIN, PITCH1_REVERSE,
  PITCH2_STEP_PIN, PITCH2_DIR_PIN, PITCH2_REVERSE,
  false, 0, 0
};

float rollRate  = 0.0f;
float pitchRate = 0.0f;

// =============================================
// IMU STATE
// =============================================

float gxBias = 0.0f;
float gyBias = 0.0f;
float gzBias = 0.0f;

float roll  = 0.0f;
float pitch = 0.0f;
float yaw   = 0.0f;

float gx = 0.0f;
float gy = 0.0f;
float gz = 0.0f;

unsigned long previousMicros = 0;
unsigned long lastIMURead    = 0;
unsigned long lastSerialSend = 0;

bool imuValid = false;

// =============================================
// HELPERS
// =============================================

void stopMotors() {
  rollRate  = 0.0f;
  pitchRate = 0.0f;
}

void haltForever() {
  digitalWrite(ROLL_STEP_PIN, LOW);
  digitalWrite(PITCH1_STEP_PIN, LOW);
  digitalWrite(PITCH2_STEP_PIN, LOW);
  digitalWrite(CNC_ENABLE_PIN, HIGH);   // disable CNC shield drivers
  while (true) {
    delay(100);
  }
}

void initAxisPins(const StepperAxis &m) {
  pinMode(m.stepPin1, OUTPUT);
  pinMode(m.dirPin1, OUTPUT);
  digitalWrite(m.stepPin1, LOW);
  digitalWrite(m.dirPin1, LOW);

  if (m.stepPin2 != NO_PIN) {
    pinMode(m.stepPin2, OUTPUT);
    pinMode(m.dirPin2, OUTPUT);
    digitalWrite(m.stepPin2, LOW);
    digitalWrite(m.dirPin2, LOW);
  }
}

// =============================================
// GYRO CALIBRATION
// =============================================

bool calibrateGyro() {

  const int SAMPLES = 200;

  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumZ = 0.0f;

  int count = 0;

  unsigned long startTime = millis();

  while (count < SAMPLES) {

    float x, y, z;

    if (imu.readGyro(x, y, z)) {
      sumX += x;
      sumY += y;
      sumZ += z;
      count++;
    }

    if (millis() - startTime > 10000) {
      return false;
    }

    delay(10);
  }

  gxBias = sumX / SAMPLES;
  gyBias = sumY / SAMPLES;
  gzBias = sumZ / SAMPLES;

  return true;
}

// =============================================
// STEP GENERATION (1 or 2 motors in sync)
// =============================================

void serviceMotor(StepperAxis &m, float signedRate) {

  unsigned long now = micros();

  // Finish current pulse (both motors together)
  if (m.pulseHigh) {
    if ((unsigned long)(now - m.pulseStartMicros) >= STEP_PULSE_US) {
      digitalWrite(m.stepPin1, LOW);
      if (m.stepPin2 != NO_PIN) {
        digitalWrite(m.stepPin2, LOW);
      }
      m.pulseHigh = false;
    }
    return;
  }

  float rate = fabs(signedRate);

  if (rate < MIN_STEP_RATE) {
    return;
  }

  if (rate > MAX_STEP_RATE) {
    rate = MAX_STEP_RATE;
  }

  bool direction = signedRate > 0.0f;

  unsigned long interval = (unsigned long)(1000000.0f / rate);

  if ((unsigned long)(now - m.lastStepMicros) >= interval) {

    // Set direction for each motor
    digitalWrite(m.dirPin1, (direction ^ m.reverse1) ? HIGH : LOW);
    if (m.stepPin2 != NO_PIN) {
      digitalWrite(m.dirPin2, (direction ^ m.reverse2) ? HIGH : LOW);
    }

    m.lastStepMicros = now;

    // Start the step pulse on both motors
    digitalWrite(m.stepPin1, HIGH);
    if (m.stepPin2 != NO_PIN) {
      digitalWrite(m.stepPin2, HIGH);
    }

    m.pulseHigh = true;
    m.pulseStartMicros = now;
  }
}

// =============================================
// IMU INITIALIZATION
// =============================================

bool initializeIMU() {

  Wire.begin();
  Wire.setClock(100000);

  if (!imu.begin(Wire, 0x6B, 100000)) {
    return false;
  }

  imu.setAccelConfig(XL_ODR_104Hz, XL_FS_4G, true);
  imu.setGyroConfig(G_ODR_104Hz, G_FS_500, true);

  return true;
}

// =============================================
// UPDATE IMU + ROLL/PITCH CONTROL
// =============================================

void updateIMU() {

  float ax, ay, az;
  float gxRaw, gyRaw, gzRaw;

  if (!imu.readAccel(ax, ay, az) ||
      !imu.readGyro(gxRaw, gyRaw, gzRaw)) {

    imuValid = false;
    stopMotors();
    return;
  }

  unsigned long now = micros();

  float dt = (unsigned long)(now - previousMicros) / 1000000.0f;
  previousMicros = now;

  if (dt <= 0.0f || dt > 0.2f) {
    imuValid = false;
    stopMotors();
    return;
  }

  gx = gxRaw - gxBias;
  gy = gyRaw - gyBias;
  gz = gzRaw - gzBias;

  // ---------- ROLL ----------
  float accRoll = atan2(ay, az) * RAD_TO_DEG_F;
  roll = ALPHA * (roll + gx * dt) + (1.0f - ALPHA) * accRoll;

  // ---------- PITCH ----------
  float accPitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG_F;
  pitch = ALPHA * (pitch - gy * dt) + (1.0f - ALPHA) * accPitch;

  // ---------- YAW (relative) ----------
  yaw += gz * dt;

  if (yaw > 180.0f)  yaw -= 360.0f;
  if (yaw < -180.0f) yaw += 360.0f;

  // ---------- ROLL PD ----------
  rollRate = 0.0f;

  if (fabs(roll) > ROLL_DEADBAND) {
    rollRate = -(ROLL_KP * roll + ROLL_KD * gx);
    rollRate = constrain(rollRate, -MAX_STEP_RATE, MAX_STEP_RATE);
  }

  // ---------- PITCH PD (drives both pitch motors) ----------
  pitchRate = 0.0f;

  if (fabs(pitch) > PITCH_DEADBAND) {
    pitchRate = -(PITCH_KP * pitch + PITCH_KD * (-gy));
    pitchRate = constrain(pitchRate, -MAX_STEP_RATE, MAX_STEP_RATE);
  }

  imuValid = true;
}

// =============================================
// SEND IMU DATA TO ESP32
// =============================================

void sendIMUData() {

  if (!imuValid) {
    if (Serial.availableForWrite() >= 16) {
      Serial.println("IMU,ERR");
    }
    return;
  }

  if (Serial.availableForWrite() < 48) {
    return;
  }

  Serial.print("IMU,");
  Serial.print(roll, 2);
  Serial.print(",");
  Serial.print(pitch, 2);
  Serial.print(",");
  Serial.println(yaw, 2);
}

// =============================================
// SETUP
// =============================================

void setup() {

  Serial.begin(115200);

  initAxisPins(rollMotor);
  initAxisPins(pitchMotor);

  pinMode(CNC_ENABLE_PIN, OUTPUT);

  // Keep CNC shield drivers disabled until the IMU is ready
  digitalWrite(CNC_ENABLE_PIN, HIGH);

  if (!initializeIMU()) {
    Serial.println("IMU,ERR");
    haltForever();
  }

  delay(500);

  // Keep IMU stationary during calibration
  if (!calibrateGyro()) {
    Serial.println("IMU,ERR");
    haltForever();
  }

  float ax, ay, az;

  if (!imu.readAccel(ax, ay, az)) {
    Serial.println("IMU,ERR");
    haltForever();
  }

  roll  = atan2(ay, az) * RAD_TO_DEG_F;
  pitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG_F;
  yaw   = 0.0f;

  previousMicros = micros();
  lastIMURead    = previousMicros;
  rollMotor.lastStepMicros  = previousMicros;
  pitchMotor.lastStepMicros = previousMicros;
  lastSerialSend = millis();

  imuValid = true;

  // Everything is ready: enable CNC shield drivers
  digitalWrite(CNC_ENABLE_PIN, LOW);
}

// =============================================
// MAIN LOOP
// =============================================

void loop() {

  unsigned long now = micros();

  // IMU + control every ~10 ms
  if ((unsigned long)(now - lastIMURead) >= IMU_INTERVAL_US) {
    lastIMURead = now;
    updateIMU();
  }

  serviceMotor(rollMotor,  rollRate);
  serviceMotor(pitchMotor, pitchRate);

  // Telemetry every 100 ms
  if ((unsigned long)(millis() - lastSerialSend) >= SERIAL_INTERVAL_MS) {
    lastSerialSend = millis();
    sendIMUData();
  }
}