/*

    mplsartindustry/daisy
    Copyright (c) 2024-2025 held jointly by the individual authors.

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

DaisyHardware seed;

/*

  Daisy Seed pin mapping with MA&I pcb board

  11  D10     --> SW1
  16  in[0]   --> AUDIO_IN_L
  17  in[1]   --> AUDIO_IN_R
  18  out[0]  --> AUDIO_OUT_L
  19  out[1]  --> AUDIO_OUT_R
  23  A1/D16  --> POT1/J7 pin 2
  24  A2/D17  --> POT2/J8 pin 2
  25  A3/D18  --> POT3/J9 pin 2
  29  A7/D22/DAC OUT 2 --> LEDs pin 1
  30  A8/D23/DAC OUT 1 --> LEDs pin 3
  31  A9/D24  --> FS1

 */

#define PIN_SW_1 D10
#define PIN_POT_1 A1
#define PIN_POT_2 A2
#define PIN_POT_3 A3
#define PIN_LED_1 A7
#define PIN_LED_2 A8
#define PIN_FS_1 D24

void setup() {
  Serial.begin(9600);

  // pull up switch digital pins
  pinMode(PIN_SW_1, INPUT_PULLUP);
  pinMode(PIN_FS_1, INPUT_PULLUP);

  // initialize daisy hardware
  seed = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
}

void loop() {

  // digital read switch
  int sw_1 = readSwitch(PIN_SW_1);

  // analog read pots; pots are wired backward
  uint32_t pot_1 = readPot(PIN_POT_1);
  uint32_t pot_2 = readPot(PIN_POT_2);
  uint32_t pot_3 = readPot(PIN_POT_3);

  // analog write to leds
  if (sw_1 == LOW) {
    writeLed(PIN_LED_1, 0);
    writeLed(PIN_LED_2, 0);
  }
  else if (sw_1 == HIGH) {
    writeLed(PIN_LED_1, (pot_1 + pot_2) / 2);
    writeLed(PIN_LED_2, (pot_3 + pot_2) / 2);
  }

  Serial.print(sw_1);
  Serial.print("\t");
  Serial.print(pot_1);
  Serial.print("\t");
  Serial.print(pot_2);
  Serial.print("\t");
  Serial.println(pot_3);
  delay(100);
}

int readSwitch(int32_t pin) {
  return digitalRead(pin);
}

uint32_t readPot(int32_t pin) {
  return constrain(1023 - analogRead(pin), 0, 1023);
}

void writeLed(int32_t pin, int32_t value) {
  analogWrite(pin, constrain(value / 4, 0, 255));
}
