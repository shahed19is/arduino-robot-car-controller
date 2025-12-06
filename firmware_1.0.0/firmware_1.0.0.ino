/*
 * ================================================================
 *  Differential-Drive Robot Core  |  BTS7960 + Arduino
 * ------------------------------------------------
 *  Author      : Shahed Islam Swapnw
 *  Firmware    : 1.0.0
 *  Created     : 06 December 2025
 *  Description :
 *    Generic motion core for a 4-wheel / 2-side robotic car.
 *    Uses (s, a) mixing:
 *       - s = straight component  [-1.0 .. +1.0]
 *       - a = turn component      [0 .. +1.0]
 *    to generate smooth forward, backward, curves and spins
 *    using two BTS7960 motor drivers.
 *
 *  Notes:
 *    - Only edit the CONFIG ZONE at the top.
 *    - The RESTRICTED ZONE is the motion engine. 
 *      Change it only if you fully understand the math.
 *
 *  Credits:
 *    Designed and tuned by Shahed Islam Swapnw for educational
 *    & hobby robotics.
 *    If you reuse or modify this file, please keep this header.
 * ================================================================
 */



/* **** CONFIG ZONE **** */

#define BAUD_RATE 9600  // HC-05 Bluetooth m module usually comes with 9600. To change it you first change the modules BAUD_RATE

// Motor driver count
int numMotorDrivers = 2;  // 2 x BTS7960

// Right motor (BTS7960 #1)
int RRpwm = 10;  // Right motor forward  (RPWM)
int RLpwm = 11;  // Right motor backward (LPWM)

// Left motor (BTS7960 #2)
int LRpwm = 9;  // Left motor forward   (RPWM)
int LLpwm = 3;  // Left motor backward  (LPWM)

int baseSpeed = 200;  // baseSpeed of the motors 150-200
// straight component [-1, +1];
float s[] = { -1.0, 0, 1.0 };  // the format must follow (negative, 0, positive)
// turn component [0, +1];
float a[] = { -0.3, 0, 0.3 };  // the format must follow (negative, 0, positive)

// commands list
// Forward, Backward, Left, Right, ForwardLeft, ForwardRight, BackwardLeft, BackwardRight, Stop
//const char cmdList[] = { 'F', 'B', 'L', 'R', 'G', 'H', 'I', 'J', 'S' };

/* **** RESTRICTED ZONE **** */

/*
------ This comment is for the future editor.
------ Do not touch below unless you know what are you doing.
------ For proper understanding read the readme file first.
*/

void setup() {
  Serial.begin(BAUD_RATE);

  // Motor driver pins
  int motorPins[] = { RRpwm, RLpwm, LRpwm, LLpwm };

  // Set motor pins as outputs and stop all initially
  for (int i = 0; i < 2 * numMotorDrivers; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);  // motors OFF at startup
  }
}

int clamp(int v) {
  if (v > 255) return 255;
  if (v < -255) return -255;
  return v;
}

void setMotor(int speed, float s, float a) {
  int Rpwm = speed * (s - a);
  int Lpwm = speed * (s + a);

  // Value fit in the -255 to +255 range
  Rpwm = clamp(Rpwm);
  Lpwm = clamp(Lpwm);

  // Direction Control for Right Motor
  if (Rpwm > 0) {
    // Forward direction
    analogWrite(RRpwm, 0);
    analogWrite(RLpwm, Rpwm);
  } else if (Rpwm < 0) {
    // Backward direction
    Rpwm = abs(Rpwm);
    analogWrite(RRpwm, Rpwm);
    analogWrite(RLpwm, 0);
  } else {
    analogWrite(RRpwm, 0);
    analogWrite(RLpwm, 0);
  }

  // Direction Control for Left Motor
  if (Lpwm > 0) {
    // Forward direction
    analogWrite(LRpwm, 0);
    analogWrite(LLpwm, Lpwm);
  } else if (Lpwm < 0) {
    // Backward direction
    Lpwm = abs(Lpwm);
    analogWrite(LRpwm, Lpwm);
    analogWrite(LLpwm, 0);
  } else {
    analogWrite(LRpwm, 0);
    analogWrite(LLpwm, 0);
  }
}

// Value Selector
int i = 1;
int j = 1;

// --- Main loop ---
void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    //Serial.println("BAT:75,SPEED:25");

    switch (cmd) {
      case 'F':  // Forward
        i = 2;
        j = 1;
        break;

      case 'B':  // Backward
        i = 0;
        j = 1;
        break;

      case 'L':  // Left (spin left)
        i = 1;
        j = 2;
        break;

      case 'R':  // Right (spin right)
        i = 1;
        j = 0;
        break;

      case 'G':  // Forward Left
        i = 2;
        j = 2;
        break;

      case 'H':  // Forward Right
        i = 2;
        j = 0;
        break;

      case 'I':  // Backward Left
        i = 0;
        j = 2;
        break;

      case 'J':  // Backward Right
        i = 0;
        j = 2;
        break;

      case 'S':  // Stop
        i = j = 1;
        break;

      default:
        // Unknown command – optionally ignore or stop
        break;
    }
  }

  setMotor(baseSpeed, s[i], a[j]);
}