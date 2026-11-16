/*

    mplsartindustry/daisy
    Copyright (c) 2024-2026 held jointly by the individual authors.

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
Switch b7;
Switch b8;

void setup() {

  // initialize hardware
  patch = DAISY.init(DAISY_PATCH_SM);

  // b7, momentary button B7
  b7.Init(1000, true, PIN_PATCH_SM_B7, INPUT_PULLUP);

  // b8, toggle switch
  b8.Init(1000, true, PIN_PATCH_SM_B8, INPUT_PULLUP);

  // led on back panel
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {

  // debounce button and switch
  b7.Debounce();
  b8.Debounce();

  bool b7_pressed = b7.Pressed();
  bool b8_pressed = b8.Pressed();

  // led on back panel
  digitalWrite(LED_BUILTIN, b7_pressed || b8_pressed);

  // cv_out_1, C10 output jack
  float cv_out_1 = 0.0;
  if (b7_pressed && b8_pressed) {
    cv_out_1 = 4.0;
  }
  else if (b7_pressed || b8_pressed) {
    cv_out_1 = 3.0;
  }
  patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_1, cv_out_1);

  // cv_out_2, led on front panel
  float cv_out_2 = 0.0;
  if (b7_pressed && b8_pressed) {
    cv_out_2 = 2.0;
  }
  else if (b7_pressed || b8_pressed) {
    cv_out_2 = 1.0;
  }
  patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_2, cv_out_2);
}
