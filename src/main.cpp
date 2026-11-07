#include <Arduino.h>
#include <ESP32Servo.h>

// ============================================================================
// 1. PINS AND DEFINITIONS
// ============================================================================
const int JOYSTICK_X_PIN = 34; // Joystick X-axis analog input pin (ADC1)
const int SERVO_PIN      = 13; // Servo signal pin

Servo myServo;

// Standard pulse widths for SG90 servo in microseconds
const int SERVO_MIN_PULSE = 500;  // 0 degrees
const int SERVO_MAX_PULSE = 2500; // 180 degrees

// Memory state variable for Option 3 (Integrating Control)
float currentAngleState = 90.0;


// ============================================================================
// 2. HELPER FUNCTIONS
// ============================================================================

/**
 * Safely writes an angle (0 to 180) to the servo.
 */
void writeServoAngle(int angle) {
  int clampedAngle = constrain(angle, 0, 180);
  myServo.write(clampedAngle);
}

/**
 * OPTION 1: Simple Linear Transfer Function
 * Maps raw ADC input directly across a customizable output range.
 * 
 * Parameters:
 *   - rawInput: ADC reading (0 to 4095)
 *   - minAngle: Output servo angle at minimum input (0)
 *   - maxAngle: Output servo angle at maximum input (4095)
 */
int transferLinear(int rawInput, int minAngle = 0, int maxAngle = 180) {
  return map(rawInput, 0, 4095, minAngle, maxAngle);
}

/**
 * OPTION 2: Non-Linear Power Curve Transfer Function
 * Provides fine control around center and coarse response near extreme edges.
 * 
 * Parameters:
 *   - rawInput: ADC reading (0 to 4095)
 *   - exponent: Curve exponent (>1.0 increases center deadzone/precision, e.g., 2.0 or 3.0)
 *   - minAngle: Minimum output angle
 *   - maxAngle: Maximum output angle
 */
int transferNonLinear(int rawInput, float exponent = 2.0, int minAngle = 0, int maxAngle = 180) {
  // Normalize input from 0..4095 to range [-1.0, 1.0] centered at 2047.5
  float norm = (rawInput - 2047.5f) / 2047.5f;
  
  // Apply power curve while preserving sign (+/-)
  float sign = (norm < 0) ? -1.0f : 1.0f;
  float curvedNorm = sign * pow(abs(norm), exponent);
  
  // Scale normalized value [-1.0, 1.0] to angle range [minAngle, maxAngle]
  float centerAngle = (minAngle + maxAngle) / 2.0f;
  float halfSpan    = (maxAngle - minAngle) / 2.0f;
  int targetAngle   = (int)(centerAngle + (curvedNorm * halfSpan));
  
  return constrain(targetAngle, minAngle, maxAngle);
}

/**
 * OPTION 3: Transfer Function with Memory (Integrating / Velocity Control)
 * Joystick position determines rotation speed/direction to accumulate onto current angle.
 * 
 * Parameters:
 *   - rawInput: ADC reading (0 to 4095)
 *   - currentAngle: Stored position state of the servo (0 to 180)
 *   - deadband: Idle center region (raw units) to prevent drift
 *   - maxSpeed: Maximum degree shift added/subtracted per iteration loop
 */
float transferWithMemory(int rawInput, float currentAngle, int deadband = 300, float maxSpeed = 3.0) {
  int centerOffset = rawInput - 2048;
  
  // If joystick is near center resting position, do not update position
  if (abs(centerOffset) < deadband) {
    return currentAngle;
  }
  
  // Normalize deflection outside the deadband to range [-1.0, 1.0]
  float deflection = 0.0;
  if (centerOffset > 0) {
    deflection = (float)(centerOffset - deadband) / (2047.0f - deadband);
  } else {
    deflection = (float)(centerOffset + deadband) / (2048.0f - deadband);
  }
  
  // Accumulate velocity step into state variable and clamp to 0-180 degrees
  float updatedAngle = currentAngle + (deflection * maxSpeed);
  return constrain(updatedAngle, 0.0f, 180.0f);
}


// ============================================================================
// 3. SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);

  // Configure Joystick input pin
  pinMode(JOYSTICK_X_PIN, INPUT);

  // Attach servo to GPIO pin with standard SG90 pulse width bounds
  myServo.attach(SERVO_PIN, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);

  // Initialize servo position to center (90 degrees)
  writeServoAngle((int)currentAngleState);

  Serial.println("Setup complete. Select a transfer function in section 4.2.");
}


// ============================================================================
// 4. LOOP
// ============================================================================
void loop() {
  // --------------------------------------------------------------------------
  // 4.1 GET INPUT
  // --------------------------------------------------------------------------
  // Read ESP32 raw 12-bit analog input (0 to 4095)
  int rawJoystickX = analogRead(JOYSTICK_X_PIN);


  // --------------------------------------------------------------------------
  // 4.2 TRANSFORM FUNCTION (STUDENT TASK)
  // --------------------------------------------------------------------------
  int targetAngle = 90;

  // --- UNCOMMENT ONE OPTION BELOW TO RUN ---

  // OPTION 1: Linear Map (Control output range limits: e.g. 0 to 180 or 45 to 135)
  // targetAngle = transferLinear(rawJoystickX, 0, 180);

  // OPTION 2: Non-Linear Map (Control curve exponent: 1.0 = linear, 2.0 = quadratic, 3.0 = cubic)
  // targetAngle = transferNonLinear(rawJoystickX, 2.5, 0, 180);

  // OPTION 3: Memory Transfer Function (Control deadband size and angular step speed)
  // currentAngleState = transferWithMemory(rawJoystickX, currentAngleState, 300, 2.5);
  // targetAngle       = (int)currentAngleState;


  // --------------------------------------------------------------------------
  // 4.3 WRITE ANGLE TO SERVO
  // --------------------------------------------------------------------------
  writeServoAngle(targetAngle);

  // Plotter & Monitor debug output
  Serial.print("RawInput:");
  Serial.print(rawJoystickX);
  Serial.print("\tTargetAngle:");
  Serial.println(targetAngle);

  delay(20); // Refresh interval for stable servo control
}