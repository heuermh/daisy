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
#include "slew.h"

DaisyHardware seed;

Slew delayTime;
Slew mix;
Slew feedback;

#define MAX_DELAY static_cast<size_t>(96000 * 1.2f)
static DelayLine<float, MAX_DELAY> delayLine;

#define MAX_GAIN 1.2f
#define MAX_FEEDBACK 1.2f

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

void audio(float **in, float **out, size_t size) {

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

  // slew pot_1 to time, pot_2 to mix, and pot_3 to feedback
  float t = delayTime.Process(mapf(pot_1, 0.0f, 1023.0f, 0.0f, 1.0f));
  float m = mix.Process(mapf(pot_2, 0.0f, 1023.0f, 0.0, MAX_GAIN));
  float f = feedback.Process(mapf(pot_3, 0.0f, 1023.0f, 0.0f, MAX_FEEDBACK));

  delayLine.SetDelay(t * MAX_DELAY);

  for (size_t i = 0; i < size; i++) {

    // read dry from input
    float dry_l = in[0][i];

    // read wet from delay line
    float wet_l = delayLine.Read();

    // write to delay line, with feedback
    delayLine.Write((wet_l * f) + dry_l);

    // mix wet and dry and soft clip
    out[0][i] = softClip(wet_l * m + dry_l * (MAX_GAIN - m));
  }
}

void setup() {

  // pull up switch digital pins
  pinMode(PIN_SW_1, INPUT_PULLUP);
  pinMode(PIN_FS_1, INPUT_PULLUP);

  // initialize daisy hardware
  seed = DAISY.init(DAISY_SEED, AUDIO_SR_96K);

  delayTime.Init();
  mix.Init();
  feedback.Init();

  delayLine.Init();
  delayLine.SetDelay(MAX_DELAY);

  // register audio callback
  DAISY.begin(audio);
}

void loop() {}

int readSwitch(int32_t pin) {
  return digitalRead(pin);
}

uint32_t readPot(int32_t pin) {
  return constrain(1023 - analogRead(pin), 0, 1023);
}

void writeLed(int32_t pin, int32_t value) {
  analogWrite(pin, constrain(value / 4, 0, 255));
}

float softClip(float x) {
  if (x < -3.0f) {
    return -1.0f;
	}
  else if (x > 3.0f) {
    return 1.0f;
  }
  return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
