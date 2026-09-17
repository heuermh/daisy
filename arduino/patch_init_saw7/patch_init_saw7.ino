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

Oscillator saw1;
Oscillator saw2;
Oscillator saw3;
Oscillator saw4;
Oscillator saw5;
Oscillator saw6;
Oscillator saw7;

// offsets and curves from Szabo
// https://www.adamszabo.com/internet/adam_szabo_how_to_emulate_the_super_saw.pdf

const float SAW1_OFFSET = -0.11002313f;
const float SAW2_OFFSET = -0.06288439f;
const float SAW3_OFFSET = -0.01952356f;

const float SAW5_OFFSET = 0.01991221f;
const float SAW6_OFFSET = 0.06216538f;
const float SAW7_OFFSET = 0.10745242f;

void audioCallback(float** in, float** out, size_t size) {

  patch.ProcessAllControls();

  b7.Debounce();
  bool rising = b7.RisingEdge();
  bool pressed = b7.Pressed();
  bool trigger = patch.gateIns[0].Trig();
  bool gate = patch.gateIns[0].State();

  if (rising || trigger) {
    randomizePhase();
  }

  // cv_out_2, C1 led on front panel
  digitalWrite(PIN_PATCH_SM_CV_OUT_2, (pressed || gate));

  // 0.0f to 1.0f
  float tunePot = roundf(patch.controls[0].Value() * 1000.0f) / 1000.0f; // truncate to 3 digits
  float fineTunePot = roundf(patch.controls[1].Value() * 1000.0f) / 1000.0f; // truncate to 3 digits
  float detunePot = patch.controls[2].Value();
  float mixPot = patch.controls[3].Value();

  // -1.0f to 1.0f
  float freqCv = patch.controls[4].Value();
  float detuneCv = patch.controls[5].Value();
  float mixCv = patch.controls[6].Value();

  // 1 v per octave
  float freqVolts = freqCv * 5.0f; // -5.0 v to +5.0 v
  float tuneVolts = (tunePot - 0.5f) * 2.5f; // -1.25 v to +1.25 v 
  float fineTuneVolts = (fineTunePot - 0.5f) * 0.125f; // -0.0625 v to +0.0625 v
  float f = vtof(freqVolts + tuneVolts + fineTuneVolts);

  // detune amount
  float d = detune(detunePot + detuneCv);

  saw1.SetFreq((1.0f + (SAW1_OFFSET * d)) * f);
  saw2.SetFreq((1.0f + (SAW2_OFFSET * d)) * f);
  saw3.SetFreq((1.0f + (SAW3_OFFSET * d)) * f);
  saw4.SetFreq(f);
  saw5.SetFreq((1.0f + (SAW5_OFFSET * d)) * f);
  saw6.SetFreq((1.0f + (SAW6_OFFSET * d)) * f);
  saw7.SetFreq((1.0f + (SAW7_OFFSET * d)) * f);

  // mix between center and sides
  float cm = centerMix(mixPot + mixCv);
  float sm = sideMix(mixPot + mixCv);

  saw1.SetAmp(sm);
  saw2.SetAmp(sm);
  saw3.SetAmp(sm);
  saw4.SetAmp(cm);
  saw5.SetAmp(sm);
  saw6.SetAmp(sm);
  saw7.SetAmp(sm);

  /*
  if (rising || trigger) {
    Serial.print(tunePot, 6);
    Serial.print("\t");
    Serial.print(fineTunePot, 6);
    Serial.print("\t");
    Serial.print(detunePot);
    Serial.print("\t");
    Serial.print(mixPot);
    Serial.print("\t");
    Serial.print(freqCv, 6);
    Serial.print("\t");
    Serial.print(detuneCv);
    Serial.print("\t");
    Serial.print(mixCv);
    Serial.print("\t");
    Serial.print(freqVolts, 6);
    Serial.print("\t");
    Serial.print(fineTuneVolts, 6);
    Serial.print("\t");
    Serial.print(f, 6);
    Serial.print("\t");
    Serial.print(d);
    Serial.print("\t");
    Serial.print(cm);
    Serial.print("\t");
    Serial.print(sm);
    Serial.println("");
  }
  */

  // process audio buffers
  for (size_t i = 0; i < size; i++) {
    out[0][i] = out[1][i] =
      saw1.Process() +
      saw2.Process() +
      saw3.Process() +
      saw4.Process() +
      saw5.Process() +
      saw6.Process() +
      saw7.Process();
  }
}

void setup() {
  //Serial.begin(9600);

  // initialize hardware
  patch = DAISY.init(DAISY_PATCH_SM, AUDIO_SR_96K);

  // b7, momentary button B7, trigger
  b7.Init(1000, true, PIN_PATCH_SM_B7, INPUT_PULLUP);

   // initialize oscillators
  saw1.Init(DAISY.AudioSampleRate());
  saw2.Init(DAISY.AudioSampleRate());
  saw3.Init(DAISY.AudioSampleRate());
  saw4.Init(DAISY.AudioSampleRate());
  saw5.Init(DAISY.AudioSampleRate());
  saw6.Init(DAISY.AudioSampleRate());
  saw7.Init(DAISY.AudioSampleRate());

  saw1.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
  saw2.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
  saw3.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
  saw4.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
  saw5.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
  saw6.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
  saw7.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);

  // randomize initial phase
  randomizePhase();

  DAISY.StartAudio(audioCallback);
}

void loop() {
  // empty
}

float vtof(float v) {  
	return powf(2, v) * 261.625565f;
}

float randomf() {
  return random(0, 100000) / 100000.0f;
}

void randomizePhase() {
  saw1.Reset(randomf());
  saw2.Reset(randomf());
  saw3.Reset(randomf());
  // do not change phase of saw4
  //saw4.Reset(randomf());
  saw5.Reset(randomf());
  saw6.Reset(randomf());
  saw7.Reset(randomf());
}

// y = -0.55366*x + 0.99785
float centerMix(float m) {
  if (m < 0.0f) {
    return 1.0f;
  }
  else if (m > 1.0f) {
    return 0.44419f;
  }
  return -0.55366f * m + 0.99785f;
}

// y = -0.73764*x^2 + 1.2841*x + 0.044372
float sideMix(float m) {
  if (m < 0.0f) {
    return 0.0f;
  }
  else if (m > 1.0f) {
    return 0.5508972f;
  }
  return -0.73764 * m * m + 1.2841f * m + 0.044372f;
}

// y = 0.8*x^7 + 0.02*x^3 + 0.18x + 0.003
//
// approximated from
//
// y = 10028.7312891634*x^11 - 50818.8652045924*x^10 + 111363.4808729368*x^9 - 138150.6761080548*x^8 +
//     106649.6679158292*x^7 - 53046.9642751875*x^6 + 17019.9518580080*x^5 - 3425.0836591318*x^4 +
//     404.2703938388*x^3 - 24.1878824391*x^2 + 0.6717417634*x + 0.0030115596
float detune(float d) {
  if (d < 0.0005f) {
    return 0.0f;
  }
  else if (d >= 1.0f) {
    return 1.0f;
  }
  return 0.9f * d * d * d * d * d * d * d * d * d - 0.02f * d * d * d + 0.18f * d + 0.003f;
}
