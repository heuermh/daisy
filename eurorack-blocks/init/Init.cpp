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
#include "Init.h"

void Init::process() {
  float cv_1 = ui.cv_1;
  float cv_2 = ui.cv_2 * 0.5;
  float cv_3 = ui.cv_3 * 0.25;
  float cv_4 = ui.cv_4 * 0.125;

  float cv_5 = ui.cv_5;
  float cv_6 = ui.cv_6 * 0.5;
  float cv_7 = ui.cv_7 * 0.25;
  float cv_8 = ui.cv_8 * 0.125;

  float cv_out_1 = cv_1 + cv_2 + cv_3 + cv_4 + cv_5 + cv_6 + cv_7 + cv_8;

  if (cv_out_1 > 1.0) {
    cv_out_1 = 1.0;
  }
  if (cv_out_1 < -1.0) {
    cv_out_1 = -1.0;
  }
  ui.cv_out_1 = cv_out_1;

  if (ui.b8.position_first() || ui.b7.pressed() || ui.b7.held()) {
    ui.cv_out_2.on(1.0);
  }
  else {
    ui.cv_out_2.off();
  }

  ui.gate_out_1 = ui.gate_in_1;
  ui.gate_out_2 = ui.gate_in_2;

  ui.audio_out_left = ui.audio_in_left;
  ui.audio_out_right = ui.audio_in_right;
}
