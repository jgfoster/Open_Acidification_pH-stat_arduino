#include <Arduino.h>
#include <ArduinoUnitTests.h>
#include <ci/ObservableDataStream.h>

#include "Devices/DateTime_TC.h"
#include "MainMenu.h"
#include "PHCalibrationMid.h"
#include "PHControl.h"
#include "TankControllerLib.h"

const uint16_t PIN = 49;

/**
 * cycle the control through to a point of being off
 */
void reset() {
  PHControl* singleton = PHControl::instance();
  singleton->enablePID(false);
  singleton->setTargetPh(7.00);
  singleton->updateControl(7.00);
  delay(10000);
  singleton->updateControl(7.00);
  TankControllerLib* tc = TankControllerLib::instance();
  tc->setNextState(new MainMenu(tc), true);
}

unittest_setup() {
  reset();
}

unittest_teardown() {
  reset();
}

// updateControl function
unittest(beforeTenSeconds) {
  GodmodeState* state = GODMODE();
  PHControl* controlSolenoid = PHControl::instance();
  TankControllerLib::instance()->loop();
  state->resetClock();
  DateTime_TC january(2021, 1, 15, 1, 48, 24);
  january.setAsCurrent();
  delay(1000);
  state->serialPort[0].dataOut = "";  // the history of data written
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
  controlSolenoid->setTargetPh(7.00);
  controlSolenoid->updateControl(8.00);
  assertEqual(TURN_SOLENOID_ON, state->digitalPin[PIN]);
  assertEqual("2021-01-15 01:48:25\r\nCO2 bubbler turned on after 1000 ms\r\n", state->serialPort[0].dataOut);
  delay(9500);
  controlSolenoid->updateControl(8.00);
  assertEqual(TURN_SOLENOID_ON, state->digitalPin[PIN]);
}

unittest(afterTenSecondsButPhStillHigher) {
  GodmodeState* state = GODMODE();
  PHControl* controlSolenoid = PHControl::instance();
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
  controlSolenoid->setTargetPh(7.00);
  controlSolenoid->updateControl(8.00);
  assertEqual(TURN_SOLENOID_ON, state->digitalPin[PIN]);
  delay(9500);
  controlSolenoid->updateControl(8.00);
  assertEqual(TURN_SOLENOID_ON, state->digitalPin[PIN]);
  delay(1000);
  controlSolenoid->updateControl(7.25);
  assertEqual(TURN_SOLENOID_ON, state->digitalPin[PIN]);
}

unittest(afterTenSecondsAndPhIsLower) {
  GodmodeState* state = GODMODE();
  PHControl* controlSolenoid = PHControl::instance();
  state->serialPort[0].dataOut = "";  // the history of data written
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
  controlSolenoid->setTargetPh(7.00);
  controlSolenoid->updateControl(8.00);
  assertEqual(TURN_SOLENOID_ON, state->digitalPin[PIN]);
  assertEqual("2021-01-15 01:49:25\r\nCO2 bubbler turned on after 20014 ms\r\n", state->serialPort[0].dataOut);
  state->serialPort[0].dataOut = "";  // the history of data written
  delay(9500);
  controlSolenoid->updateControl(8.00);
  assertEqual(TURN_SOLENOID_ON, state->digitalPin[PIN]);
  delay(1000);
  controlSolenoid->updateControl(6.75);
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
  assertEqual("2021-01-15 01:49:35\r\nCO2 bubbler turned off after 10500 ms\r\n", state->serialPort[0].dataOut);
}

/**
 * Test that CO2 b is turned on when needed
 * \see unittest(disableDuringCalibration)
 */
unittest(beforeTenSecondsButPhIsLower) {
  GodmodeState* state = GODMODE();
  PHControl* controlSolenoid = PHControl::instance();
  // device is initially off but turns on when needed
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
  controlSolenoid->setTargetPh(7.00);
  controlSolenoid->updateControl(8.00);
  assertEqual(TURN_SOLENOID_ON, state->digitalPin[PIN]);
  delay(7500);
  controlSolenoid->updateControl(6.75);
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
}

unittest(PhEvenWithTarget) {
  GodmodeState* state = GODMODE();
  PHControl* controlSolenoid = PHControl::instance();
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
  controlSolenoid->setTargetPh(7.00);
  controlSolenoid->updateControl(7.00);
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
}

/**
 * Test that CO2 bubbler is turned on when needed
 * \see unittest(beforeTenSecondsButPhIsLower)
 */
unittest(disableDuringCalibration) {
  TankControllerLib* tc = TankControllerLib::instance();
  assertFalse(tc->isInCalibration());
  PHCalibrationMid* test = new PHCalibrationMid(tc);
  tc->setNextState(test, true);
  assertTrue(tc->isInCalibration());
  GodmodeState* state = GODMODE();
  PHControl* controlSolenoid = PHControl::instance();
  // device is initially off and stays off due to calibration
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
  controlSolenoid->setTargetPh(7.00);
  controlSolenoid->updateControl(8.00);
  assertEqual(TURN_SOLENOID_OFF, state->digitalPin[PIN]);
}

unittest_main()
