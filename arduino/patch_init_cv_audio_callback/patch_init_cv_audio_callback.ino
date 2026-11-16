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

void audioCallback(float** in, float** out, size_t size) {

  // pots CV_1 through CV_4 are control indices 0-3
  // bi-polar cv voltages are normalized between -1.0f and 1.0f
  float cv_1 = patch.controls[0].Process();
  float cv_2 = patch.controls[1].Process();
  float cv_3 = patch.controls[2].Process();
  float cv_4 = patch.controls[3].Process();

  // input jacks CV_5 through CV_8 map to control indices 4-7
  // bi-polar cv voltages are normalized between -1.0f and 1.0f
  float cv_5 = patch.controls[4].Process();
  float cv_6 = patch.controls[5].Process();
  float cv_7 = patch.controls[6].Process();
  float cv_8 = patch.controls[7].Process();


  float cv_out_1 = cv_1 + (cv_2 * 0.5f) + (cv_3 * 0.25f) + (cv_4 * 0.125f)
    + cv_5 + (cv_6 * 0.5f) + (cv_7 * 0.25f) + (cv_8 * 0.125f);

  // constrain between -1.0f and 1.0f
  cv_out_1 = constrain(cv_out_1, -1.0f, 1.0f);

  // map to 0.0v to 5.0v range
  cv_out_1 = mapf(cv_out_1, -1.0f, 1.0f, 0.0f, 5.0f);


  // cv_out_1, C10 output jack
  patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_1, cv_out_1);

  // cv_out_2, C1 led on front panel
  patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_2, cv_out_1);


  // buffer audio in to audio out on both L and R channels
  for (size_t i = 0; i < size; i++) {
    out[0][i] = in[0][i];
    out[1][i] = in[1][i];
  }
}

void setup() {
  // initialize hardware
  patch = DAISY.init(DAISY_PATCH_SM);

  // use audioCallback at default audio rate (4 samples per channel at 16kHz)
  DAISY.StartAudio(audioCallback);
}

void loop() {
  // empty
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
