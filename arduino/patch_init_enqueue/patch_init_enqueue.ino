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

bool _clockState = false;
bool _resetState = false;

static uint8_t queue = 0;
const uint8_t MAX_QUEUE = 8;

void setup() {
  Serial.begin(9600);

  // initialize hardware
  patch = DAISY.init(DAISY_PATCH_SM);

  // b7, momentary button B7, enqueue/push
  b7.Init(1000, true, PIN_PATCH_SM_B7, INPUT_PULLUP);

  // b8, toggle switch, invert
  b8.Init(1000, true, PIN_PATCH_SM_B8, INPUT_PULLUP);
}

void loop() {

  // update inputs
  patch.ProcessAllControls();

  b7.Debounce();
  b8.Debounce();
  bool b7_rising = b7.RisingEdge();
  bool b8_pressed = b8.Pressed();

  // gate_in_1, B10 input jack, clock in/pop
  patch.gateIns[0].Debounce();
  bool _clock = _clockState && !patch.gateIns[0].State();
  _clockState = patch.gateIns[0].State();

  // gate_in_2, B9 input jack, reset/clear
  patch.gateIns[1].Debounce();
  bool _reset = patch.gateIns[1].Trig() || (!_resetState && patch.gateIns[1].State());
  _resetState = patch.gateIns[1].State();

  // add'l hw: clock in/pop trigger/gate in

  if (_reset) {
    queue = 0;
  }
  if (_clock && queue > 0) {
    queue--;
  }
  if (b7_rising && queue < MAX_QUEUE) {
    queue++;
  }

  bool gate = queue != 0;
  bool clock = gate && _clockState;
  bool notClock = !gate && _clockState;

  if (b8_pressed) {
    // gate_out_1, B5 output jack, clock out
    digitalWrite(PIN_PATCH_SM_GATE_OUT_1, clock);
    // gate_out_2, B6 output jack, inverse clock out/not out
    digitalWrite(PIN_PATCH_SM_GATE_OUT_2, notClock);
    // cv_out_2, C1 led on front panel, clock out
    digitalWrite(PIN_PATCH_SM_CV_OUT_2, clock);
  }
  else {
    // invert outputs
    // gate_out_1, B5 output jack, clock out
    digitalWrite(PIN_PATCH_SM_GATE_OUT_1, notClock);
    // gate_out_2, B6 output jack, inverse clock out/not out
    digitalWrite(PIN_PATCH_SM_GATE_OUT_2, clock);
    // cv_out_2, C1 led on front panel, clock out
    digitalWrite(PIN_PATCH_SM_CV_OUT_2, notClock);
  }

  // add'l hw: peek gate out, not peek gate out, input/output/queue LEDs
}