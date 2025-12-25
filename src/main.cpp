#include <Arduino.h>
#include <ESP32Servo.h>

/*
Filtro Media Móvil como Pasa Bajos
An=a*M+(1-a)*An
alpha 1: Sin filtro
alpha 0: Filtrado totalmente
alpha mas comun 0.05
*/
#define alpha 0.5

// === Servo Pins ===
#define SERVO_PAN_PIN   19
#define SERVO_TILT_A    18
#define SERVO_TILT_B    5


// === Joystick Pins ===
#define JOY_X_PIN 36
#define JOY_Y_PIN 12
#define JOY_BUTTON_X 33


#define SPEED_BUTTON 13   // ← USADO PARA CAMBIO DE VELOCIDAD

// ======= Variables Joystick =======
int centerX = 0;
int centerY = 0;
int threshold = 500;

// === Servo Objects ===
Servo servoPan;
Servo servoTiltA;
Servo servoTiltB;

// === Servo Pulse Configuration ===
const int minPulse = 500;
const int maxPulse = 2400;
int centerPulse = 1450;

// === Current positions ===
int panPulse = centerPulse;
int tiltAPulse = centerPulse;
int tiltBPulse = centerPulse;

int rawX = 0;
int rawX_filtrado = 0;
int rawY = 0;
int rawY_filtrado = 0;

int delayRead = 0;
int mappedDelay = 0;

// ==== NUEVO: VARIABLES PARA 3 ESTADOS DE VELOCIDAD ====
int speedMode = 0;             // 0=lento, 1=medio, 2=rápido
int currentServoStep = 1;      // valor inicial según estado 0
int currentLoopDelay = 100;

int lastButtonYState = HIGH;   // para detección de flanco

void calibrateJoystick() {
  long sumX = 0;
  long sumY = 0;

  for (int i = 0; i < 10; i++) {
    sumX += analogRead(JOY_X_PIN);
    sumY += analogRead(JOY_Y_PIN);
    delay(50);
  }

  centerX = sumX / 10;
  centerY = sumY / 10;
}

void updateSpeedMode() {

/*  if (speedMode == 0) {
    currentServoStep = 1;
    currentLoopDelay = 10;
    threshold = 500;
  }
  else if (speedMode == 1) {
    currentServoStep = 10;
    currentLoopDelay = 1;
  }
  else if (speedMode == 2) {
    currentServoStep = 5;
    currentLoopDelay = 5;
  }
  else if (speedMode == 3) {
    currentServoStep = 2;
    currentLoopDelay = 10;
  }
  else if (speedMode == 4) {
    currentServoStep = 1;
    currentLoopDelay = 15;
    threshold = 100;
  }
 */

  if (speedMode == 0) {
    currentServoStep = 5;
    currentLoopDelay = 5;
  }
  else if (speedMode == 1) {
    currentServoStep = 2;
    currentLoopDelay = 10;
  }
  else if (speedMode == 2) {
    currentServoStep = 1;
    currentLoopDelay = 15;
    threshold = 100;
  }

  Serial.printf("Modo velocidad: %d  | step=%d  | loopDelay=%d\n", speedMode, currentServoStep, currentLoopDelay);
}

void setup() {
  delay(2000);
  // Attaching servos
  servoPan.attach(SERVO_PAN_PIN, minPulse, maxPulse);
  servoTiltA.attach(SERVO_TILT_A, minPulse, maxPulse);
  servoTiltB.attach(SERVO_TILT_B, minPulse, maxPulse);

  pinMode(JOY_BUTTON_X, INPUT_PULLUP);
  pinMode(SPEED_BUTTON, INPUT_PULLUP);

  delay(100);
  calibrateJoystick();

  servoPan.writeMicroseconds(centerPulse);
  delay(500);

  servoTiltA.writeMicroseconds(centerPulse);
  servoTiltB.writeMicroseconds(centerPulse);
  delay(500);

  Serial.begin(115200);

  // iniciar en modo 0 (lento)
  updateSpeedMode();
}
int previousMilis = 0;
int currentMilis = 0;
void loop() {
  //previousMilis = currentMilis;
  currentMilis = millis();
  // ======= CAMBIO DE MODO POR JOY_BUTTON_Y =======
  int buttonYState = digitalRead(SPEED_BUTTON);

  if (buttonYState == LOW && lastButtonYState == HIGH) {
    speedMode = (speedMode + 1) % 3;
    updateSpeedMode();
    //Serial.printf("Velocidad\n");

  }
  lastButtonYState = buttonYState;


  // ======= BOTÓN X → CENTRAR =======
  if (!digitalRead(JOY_BUTTON_X)) {
    servoPan.writeMicroseconds(centerPulse);
    delay(500);

    servoTiltA.writeMicroseconds(centerPulse);
    servoTiltB.writeMicroseconds(centerPulse);
    delay(500);

    panPulse = centerPulse;
    tiltAPulse = centerPulse;
    tiltBPulse = centerPulse;

    return;
  }

  rawX = analogRead(JOY_X_PIN);
  rawX_filtrado = (alpha * rawX) + ((1 - alpha) * rawX_filtrado);
  //delayMicroseconds(10);
  rawY = analogRead(JOY_Y_PIN);
  rawY_filtrado = (alpha * rawY) + ((1 - alpha) * rawY_filtrado);

  //Serial.printf("rawX_filtrado: %d  | rawY_filtrado=%d\n", rawX_filtrado, rawY_filtrado);


  int deltaX = rawX_filtrado - centerX;
  int deltaY = rawY_filtrado - centerY;

  // ======== PAN ===========
  if (abs(deltaX) > threshold) {
    if (deltaX > 0)
      panPulse += currentServoStep;
    else
      panPulse -= currentServoStep;

    panPulse = constrain(panPulse, minPulse, maxPulse);
    servoPan.writeMicroseconds(panPulse);
  }

  // ======== TILT (servo espejo) ===========
  if (abs(deltaY) > threshold) {
    if (deltaY > 0) {
      tiltAPulse -= currentServoStep;
      tiltBPulse += currentServoStep;
    } else {
      tiltAPulse += currentServoStep;
      tiltBPulse -= currentServoStep;
    }

    tiltAPulse = constrain(tiltAPulse, minPulse, maxPulse);
    tiltBPulse = constrain(tiltBPulse, minPulse, maxPulse);

    servoTiltA.writeMicroseconds(tiltAPulse);
    servoTiltB.writeMicroseconds(tiltBPulse);
  }

  //=== DELAY FINAL SEGÚN MODO ===
  delay(currentLoopDelay);
  delayRead = currentMilis - millis();
  Serial.printf("Modo velocidad: %d  | step=%d  | loopDelay=%d ", speedMode, currentServoStep, currentLoopDelay);
  Serial.printf(" Loop delay real: %d ms\n", delayRead);
}

