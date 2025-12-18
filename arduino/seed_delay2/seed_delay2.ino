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

#define PIN_POT_1 A1

const size_t BUFFER_SIZE = 96000; // 96000 Hz for 1 second
class Delay {
private:
  float buffer[BUFFER_SIZE];
  size_t readIndex;
  size_t writeIndex;
  float dcBias;

  float feedback;
public:
  Delay() {
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
      buffer[i] = 0.0;
    }

    readIndex = 0;
    dcBias = 0.0;
    feedback = 0.9;

    setDelay(48000);
  }

  void setDelay(size_t delaySamples) {
    writeIndex = (readIndex + delaySamples) % BUFFER_SIZE;

    // todo: fix holes
  }

  void setFeedback(float feedback_) {
    feedback = feedback_;
  }

  float next(float in) {
    // Remove DC offset with high pass filter
    dcBias += (in - dcBias) * 0.001;
    in -= dcBias;

    float delayed = buffer[readIndex];
    float mixed = in + delayed * feedback;
    buffer[writeIndex] = mixed;

    readIndex = (readIndex + 1) % BUFFER_SIZE;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    return mixed;
  }
};

class DelayTimeFilter {
private:
  float value;
public:
  DelayTimeFilter(float initialValue) {
    value = initialValue;
  }

  float apply(float in) {
    value += (in - value) * 0.001;
    return value;
  }
};

DaisyHardware seed;
Delay delayLine;
DelayTimeFilter delayTimeFilter(BUFFER_SIZE / 2);

float mapf(float in, float inMin, float inMax, float outMin, float outMax) {
  return outMin + (in - inMin) / (inMax - inMin) * (outMax - outMin);
}

void audio(float **in, float **out, size_t size) {
  float delaySamples = mapf(analogRead(PIN_POT_1), 1023.0, 0.0, 1.0, BUFFER_SIZE);
  delayLine.setDelay((size_t) delayTimeFilter.apply(delaySamples));

  for (size_t i = 0; i < size; i++) {
    out[0][i] = delayLine.next(in[0][i]);
  }
}

void setup() {
  seed = DAISY.init(DAISY_SEED, AUDIO_SR_96K);
  DAISY.begin(audio);
}

void loop() {
}
