# Differential-Drive Robot Core – Arduino Nano + BTS7960

Firmware: **1.0.0**
Author: **Shahed Islam Swapnw**
Date: **06 December 2025**

This project is a **generic motion core** for a 4-wheel / 2-side differential-drive robot car.

It uses:

* **Arduino Nano** (ATmega328P) as the brain
* Two **BTS7960** high-current motor drivers (one for each side)
* Four 12 V 37GB 500 RPM DC gear motors (two on the left side and two on the right side, with each pair driven by one BTS7960 channel)
* An **HC-05 Bluetooth module** for command input
* Simple one-character **serial commands** to control motion

The code is written so that:

* You only edit the clearly marked **CONFIG ZONE** at the top.
* The **RESTRICTED ZONE** below contains the motion engine and control logic.

---

## 1. Overview

This firmware drives a differential-drive robot using a simple mathematical mixing of:

* `s` → **straight component** (forward / backward)
* `a` → **turn component** (curve / spin)

The two main equations are:

* **Right side** PWM value: `PWM_R = baseSpeed * (s - a)`
* **Left side** PWM value:  `PWM_L = baseSpeed * (s + a)`

The **sign** (positive/negative) of `PWM_R` and `PWM_L` determines direction (which BTS7960 input pin is active), and the **magnitude** determines the PWM duty cycle (0–255).

All high-level movement (forward, backward, spin, curves) is generated just by choosing different `(s, a)` pairs.

---

## 2. Hardware Summary

* **Controller:** Arduino Nano (ATmega328P, 16 MHz)
* **Power motor supply:** 12 V for the 37GB 500 RPM gear motors
* **Motor drivers:** Two BTS7960 (one for left, one for right)
* **Motors:** 12 V 37GB 500 RPM DC gear motors (two per side)
* **Communication:** HC-05 Bluetooth module (default baud **9600**)
* **Logic supply:** 5 V for Arduino and HC-05 (from regulator or USB)

### Pin mapping in this firmware

* **Right motor driver (BTS7960 #1)**

  * `RRpwm = 10`
  * `RLpwm = 11`

* **Left motor driver (BTS7960 #2)**

  * `LRpwm = 9`
  * `LLpwm = 3`

The exact meaning of “forward” and “backward” depends on how the motor wires are connected. In this code:

* For the **right motor**:

  * When the computed `Rpwm > 0`, `RLpwm` is driven and `RRpwm` is 0.
  * When `Rpwm < 0`, `RRpwm` is driven and `RLpwm` is 0.

* For the **left motor**:

  * When the computed `Lpwm > 0`, `LLpwm` is driven and `LRpwm` is 0.
  * When `Lpwm < 0`, `LRpwm` is driven and `LLpwm` is 0.

This mapping matches the actual wiring used during testing.
If the robot moves in the opposite direction from what you expect, you can either:

* Swap the motor wires on the BTS7960, or
* Swap which pin is treated as “forward” vs “backward” inside `setMotor()`.

---

## 3. Motion Model: s (straight) and a (turn)

### 3.1 Straight component `s`

`s` is in the range `[-1.0, 1.0]` and is implemented as:

```cpp
float s[] = { -1.0, 0, 1.0 };
```

Mapping:

* `s[0] = -1.0` → full backward
* `s[1] =  0.0` → no forward/backward (used for spins or stop)
* `s[2] =  1.0` → full forward

The firmware uses an index `i` (0, 1, or 2), and the actual value used is `s[i]`.

### 3.2 Turn component `a`

`a` is also in the range approximately `[-1.0, 1.0]` and is implemented as:

```cpp
float a[] = { -0.3, 0, 0.3 };
```

Mapping:

* `a[0] = -0.3` → turn in one direction (one spin/curve side)
* `a[1] =  0.0` → no turn (go straight)
* `a[2] =  0.3` → turn in the opposite direction

The firmware uses an index `j` (0, 1, or 2), and the actual value used is `a[j]`.

The absolute value (`0.3`) controls how “strong” the curve is:

* Closer to `0` → gentle curve (large turning radius)
* Closer to `1` → tight curve (small turning radius)

---

## 4. How the Mixing Works

Given `baseSpeed`, `s`, and `a`, the motor values are:

* `Rpwm = baseSpeed * (s - a)`
* `Lpwm = baseSpeed * (s + a)`

These values are then:

* **Clamped** to the range [-255, +255] using `clamp()`.
* Used as **signed speeds**:

  * Positive → one BTS7960 input pin gets PWM, the other is 0.
  * Negative → the opposite pin gets PWM, the current one is 0.
  * Zero → both pins are 0 (motor off).

This allows one formula to handle:

* Forward
* Backward
* Spin in place
* Forward curves
* Backward curves

without writing separate logic for each movement.

---

## 5. Serial Command Interface (HC-05 / USB)

The robot is controlled using **single characters** sent over Serial/UART at `9600` baud (suitable for HC-05 default settings).

The valid commands implemented in `loop()` are:

* `F` → Forward
* `B` → Backward
* `L` → Spin one way (labeled “Left” in the code comments)
* `R` → Spin the other way (“Right”)
* `G` → Forward curve (one side)
* `H` → Forward curve (other side)
* `I` → Backward curve
* `J` → Backward curve (currently same index as `I` in this version; can be adjusted)
* `S` → Stop

Internally, two indices `i` and `j` select the corresponding entries from `s[]` and `a[]`.
For example:

* `F` (Forward): `i = 2; j = 1;` → `s = 1.0`, `a = 0.0`
* `B` (Backward): `i = 0; j = 1;` → `s = -1.0`, `a = 0.0`
* `L` (Spin): `i = 1; j = 2;` → `s = 0.0`, `a = 0.3`
* `R` (Spin opposite): `i = 1; j = 0;` → `s = 0.0`, `a = -0.3`
* `G` / `H` / `I` / `J` combine forward/back with turning.

After setting `i` and `j`, the code calls:

```cpp
setMotor(baseSpeed, s[i], a[j]);
```

in every loop iteration, so the robot continues performing the **last received command** until a new one arrives.

---

## 6. CONFIG ZONE vs RESTRICTED ZONE

### 6.1 CONFIG ZONE

This section at the top is meant for safe editing:

* `BAUD_RATE`
* `numMotorDrivers`
* Motor pin assignments (`RRpwm`, `RLpwm`, `LRpwm`, `LLpwm`)
* `baseSpeed`
* Arrays `s[]` and `a[]`

Here you can:

* Change the baud rate if you reconfigure HC-05.
* Change pin assignments if you rewire the Nano.
* Tune `baseSpeed` (e.g. 150–200) for comfort and safety.
* Tune `s[]` and `a[]` for different speed or curve strength.

### 6.2 RESTRICTED ZONE

This part contains:

* `setup()` – initializes Serial and motor pins.
* `clamp()` – keeps PWM values in the safe range.
* `setMotor()` – does the mixing and drives the BTS7960 pins.
* `loop()` – reads serial commands and selects `(i, j)`.

It is fully working and tested with:

* Arduino Nano
* Dual BTS7960
* 12 V 37GB 500 RPM DC gear motors
* HC-05 at 9600 baud

You normally don’t need to edit this part. Only change it if you understand the math and the wiring.

---

## 7. How to Use This Firmware

1. **Wire the hardware:**

   * Connect Arduino Nano PWM pins 10, 11, 9, and 3 to the respective BTS7960 RPWM/LPWM inputs as defined in the code.
   * Connect 12 V supply to the motor power side of the BTS7960 modules.
   * Connect two 37GB 500 RPM motor to one BTS7960 output.
   * Connect the HC-05 module to the Arduino Nano’s RX/TX.

2. **Upload the firmware:**

   * Open the `.ino` file in the Arduino IDE.
   * Select the correct board (Arduino Nano) and port.
   * Upload the sketch.

3. **Connect over Serial:**

   * Use the Serial Monitor (USB) or any Bluetooth terminal app (for HC-05).
   * Set baud rate to `9600`.
   * Make sure to send **single characters** like `F`, `B`, `L`, etc.

4. **Drive the robot:**

   * `F` → move forward
   * `B` → move backward
   * `L` / `R` → spin on the spot (one direction or the other)
   * `G`, `H`, `I`, `J` → curved motion forward/backward
   * `S` → stop

If the curves or spins feel reversed, you can swap the `j` indices for `L`/`R` or `G`/`H` in the `switch(cmd)` block to match your preferred “left/right” sense.

---

## 8. Notes for Future You

When you come back to this project later, remember:

1. The robot is fully defined by:

   * `s` (straight component) and
   * `a` (turn component)

2. The control formulas are:

   * Right: `baseSpeed * (s - a)`
   * Left:  `baseSpeed * (s + a)`

3. The sign of each value picks **which BTS7960 input pin** gets PWM.

4. The **CONFIG ZONE** is safe to edit for:

   * Pins
   * Speed
   * Motion tuning

5. The **RESTRICTED ZONE** is the general engine that:

   * Initializes hardware
   * Computes motor commands
   * Maps serial commands (`F, B, L, R, G, H, I, J, S`) into motion.

With just these points, you can quickly understand, reuse, or extend this firmware for other robots and experiments.
