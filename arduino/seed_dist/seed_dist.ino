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

float PI_OVER_TWO = PI / 2.0f;

int tick = 0;

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

  // map pot_1 to dist
  int dist = pot_1 / 146;

  // map pot_2 to gain
  float gain = mapf(pot_2, 0.0f, 1023.0f, 0.0f, 1.8f);

  if ((tick % 400) == 0) {
    Serial.print(tick);
    Serial.print("\t");
    Serial.print(sw_1);
    Serial.print("\t");
    Serial.print(pot_1);
    Serial.print("\t");
    Serial.print(dist);
    Serial.print("\t");
    Serial.println(gain);
  }

  // apply gain, dist or soft clip
  for (size_t i = 0; i < size; i++) {
    if (sw_1 == LOW) {
      out[0][i] = softClip(in[0][i] * gain);
      out[1][i] = softClip(in[1][i] * gain);
    }
    else if (sw_1 == HIGH) {
      out[0][i] = applyDist(dist, in[0][i] * gain);
      out[1][i] = applyDist(dist, in[1][i] * gain);
    }
  }
  tick++;
}

void setup() {
  Serial.begin(9600);

  // pull up switch digital pins
  pinMode(PIN_SW_1, INPUT_PULLUP);
  pinMode(PIN_FS_1, INPUT_PULLUP);

  // initialize daisy hardware
  seed = DAISY.init(DAISY_SEED, AUDIO_SR_96K);

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

// see dist package in LiCK
// these are all rather subtle on guitar

float applyDist(int dist, float x) {
  switch (dist) {
    case 0: return absDist(x);
    case 1: return atanDist(x);
    case 2: return fastTanhDist(x);
    case 3: return frostburnDist(x);
    case 4: return kijjazDist(x);
    case 5: return kijjaz3Dist(x);
    case 6: return ribbonDist(x);
  }
  return x;
}

float absDist(float x) {
  return x / (1.0f + fabsf(x));
}

float atanDist(float x) {
  return atan(x) / PI_OVER_TWO;
}

float fastTanhDist(float x) {
  return x * (9.0f + x * x) / (9.0f + 3.0f * x * x);
}

float frostburnDist(float x) {
  return (x * fabsf(x) + x) / (x * x + fabsf(x) + 1.0f);
}

float kijjazDist(float x) {
  return x / (1.0f + x * x);
}

float kijjaz3Dist(float x) {
  return (x * (1.0f + x * x)) / (1.0f + fabsf((x * (1.0f + x * x))));
}

float ribbonDist(float x) {
  return x / (0.25f * x * x + 1.0f);
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
