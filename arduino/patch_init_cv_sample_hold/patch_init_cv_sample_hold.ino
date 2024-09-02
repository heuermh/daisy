/*

    mplsartindustry/daisy
    Copyright (c) 2024 held jointly by the individual authors.

    This file is part of mplsartindustry/daisy.

    mplsartindustry/daisy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    mplsartindustry/daisy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with mplsartindustry/daisy.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "DaisyDuino.h"

DaisyHardware patch;
float sample = 0.0f;
Switch b7;
Switch b8;

void setup() {
  // initialize hardware
  patch = DAISY.init(DAISY_PATCH_SM);

  // b7, momentary button B7
  b7.Init(1000, true, PIN_PATCH_SM_B7, INPUT_PULLUP);

  // b8, toggle switch
  b8.Init(1000, true, PIN_PATCH_SM_B8, INPUT_PULLUP);
}

void hold(float cv) {
  sample = cv;
}

void loop() {

  // update inputs
  patch.ProcessAllControls();

  // debounce buttons
  b7.Debounce();
  b8.Debounce();
  bool b7_pressed = b7.Pressed();
  bool b8_pressed = b8.Pressed();

  // cv_1, CV_1 pot
  float cv_1 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_1));

  // make sure off is fully off
  if (cv_1 < 0.05f) {
    cv_1 = 0.0f;
  }

  // cv_5, CV_5 input jack
  float cv_5 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_5));

  float cv_out_1 = cv_1 + cv_5;

  if (b8_pressed) {
    // constrain between -5.0v and 5.0v
    cv_out_1 = constrain(cv_out_1, -5.0f, 5.0f);
  }
  else {
    // constrain between 0.0v and 5.0v
    cv_out_1 = constrain(cv_out_1, 0.0f, 5.0f);
  }

  // gate_in_1, B10 input jack
  GateIn gate_in_1 = patch.gateIns[0];
  if (gate_in_1.State() || b7_pressed) {
    // gate_out_1, B5 output jack
    digitalWrite(PIN_PATCH_SM_GATE_OUT_1, HIGH);

    // cv_out_2, C1 led on front panel
    digitalWrite(PIN_PATCH_SM_CV_OUT_2, HIGH);
  }
  else {
    // gate_out_1, B5 output jack
    digitalWrite(PIN_PATCH_SM_GATE_OUT_1, LOW);

    // cv_out_2, C1 led on front panel
    digitalWrite(PIN_PATCH_SM_CV_OUT_2, LOW);
  }

  // hold if trigger or button pressed
  if (gate_in_1.Trig() || b7_pressed) {
    hold(cv_out_1);
  }

  // cv_out_1, C10 output jack
  patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_1, sample);
}
