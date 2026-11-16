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

// see https://github.com/id-Software/DOOM/blob/master/linuxdoom-1.10/m_random.c
// Licensed GPL v2.0 https://github.com/id-Software/DOOM/blob/master/LICENSE.TXT

int values[256] = {
    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
    120, 163, 236, 249
};

int _index = 0;
int _offset = 0;

bool _clockState = false;
bool _resetState = false;

void setup() {
  // initialize hardware
  patch = DAISY.init(DAISY_PATCH_SM);

  // b7, momentary button B7, clock advance
  b7.Init(1000, true, PIN_PATCH_SM_B7, INPUT_PULLUP);

  // b8, toggle switch, unipolar/bipolar
  b8.Init(1000, true, PIN_PATCH_SM_B8, INPUT_PULLUP);
}

void loop() {

  // update inputs
  patch.ProcessAllControls();

  // debounce button and switch, necessary if also updating inputs?
  b7.Debounce();
  b8.Debounce();

  bool b7_rising = b7.RisingEdge();
  bool b8_pressed = b8.Pressed();

  // gate_in_1, B10 input jack
  patch.gateIns[0].Debounce();
  bool _clock = patch.gateIns[0].Trig() || (!_clockState && patch.gateIns[0].State());
  _clockState = patch.gateIns[0].State();

  // gate_in_2, B9 input jack
  patch.gateIns[1].Debounce();
  bool _reset = patch.gateIns[1].Trig() || (!_resetState && patch.gateIns[1].State());
  _resetState = patch.gateIns[1].State();

  // cv_1, CV_1 pot
  float cv_1 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_1));

  // cv_5, CV_5 input jack
  float cv_5 = patch.AnalogReadToVolts(analogRead(PIN_PATCH_SM_CV_5));

  float cv = constrain(cv_1 + cv_5, -5.0f, 5.0f);
  float normalized_cv = mapf(cv, -5.0f, 5.0f, -1.0f, 1.0f);
  _offset = 255 * normalized_cv;

  if (_clock || b7_rising) {

    // offset index
    int i = _index + _offset;
    if (i > 255) {
      i = i % 255;
    }
    if (i < 0) {
      i = 256 + (i % 255);
    }

    // cv_out_1, C10 output jack
    float low = b8_pressed ? 0.0f : -5.0f;
    float cv_out_1 = mapf(values[i], 0.0f, 255.0, low, 5.0f);
    patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_1, cv_out_1);

    // cv_out_2, C1 led on front panel
    float cv_out_2 = mapf(values[i], 0.0f, 255.0, 1.0f, 4.0f);
    patch.WriteCvOut(PIN_PATCH_SM_CV_OUT_2, cv_out_2);

    _index++;
    if (_index > 255) {
      _index = 0;
    }
  }

  if (_reset) {
     _index = 0;
  }
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
