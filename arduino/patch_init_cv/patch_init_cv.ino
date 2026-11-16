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

void setup() {
  // initialize hardware
  patch = DAISY.init(DAISY_PATCH_SM);
}

void loop() {

  // update inputs
  patch.ProcessAllControls();

  // cv_1, CV_1 pot
  float cv_1 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_1));

  // cv_2, CV_2 pot
  float cv_2 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_2));

  // cv_3, CV_3 pot
  float cv_3 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_3));

  // cv_4, CV_4 pot
  float cv_4 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_4));

  // cv_5, CV_5 input jack
  float cv_5 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_5));

  // cv_6, CV_6 input jack
  float cv_6 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_6));

  // cv_7, CV_7 input jack
  float cv_7 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_7));

  // cv_8, CV_8 input jack
  float cv_8 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_8));


  float cv_out_1 = cv_1 + (cv_2 * 0.5f) + (cv_3 * 0.25f) + (cv_4 * 0.125f)
    + cv_5 + (cv_6 * 0.5f) + (cv_7 * 0.25f) + (cv_8 * 0.125f);

  // constrain between -5.0v and 5.0v
  cv_out_1 = constrain(cv_out_1, -5.0f, 5.0f);

  // map to 0.0v to 5.0v range
  cv_out_1 = mapf(cv_out_1, -5.0f, 5.0f, 0.0f, 5.0f);


  // cv_out_1, C10 output jack
  patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_1, cv_out_1);

  // cv_out_2, C1 led on front panel
  patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_2, cv_out_1);
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
