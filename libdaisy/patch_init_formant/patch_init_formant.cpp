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
#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "util/CpuLoadMeter.h"

using namespace daisy;
using namespace daisy::patch_sm;
using namespace daisysp;

typedef struct {
  float x;
  float y;
  float z;
} Formant;

constexpr int FORMANTS_SIZE = 10;

// from Allen Strange's Electronic Music: Systems, Techniques, and Controls
Formant FORMANTS[FORMANTS_SIZE] = {
  { 270.0f, 1990.0f, 3010.0f },
  { 390.0f, 1840.0f, 2550.0f },
  { 530.0f, 1720.0f, 2480.0f },
  { 660.0f, 1090.0f, 2410.0f },
  { 730.0f, 1840.0f, 2440.0f },
  { 570.0f, 1020.0f, 2410.0f },
  { 440.0f,  870.0f, 2240.0f },
  { 330.0f, 1190.0f, 2240.0f },
  { 640.0f, 1350.0f, 2390.0f },
  { 490.0f, 20000.0f, 1690.0f }
};

DaisyPatchSM hw;
Switch b7;
Switch b8;
GateIn gate1;
Parameter freqAPot;
Parameter freqBPot;
Parameter freqCPot;
Parameter resonancePot;
AnalogControl freqACv;
AnalogControl freqBCv;
AnalogControl freqCCv;
AnalogControl resonanceCv;

int filterMode = 0;
float sampleRate;

LadderFilter bandpassAL;
LadderFilter bandpassBL;
LadderFilter bandpassCL;
LadderFilter bandpassAR;
LadderFilter bandpassBR;
LadderFilter bandpassCR;

constexpr float MIX_GAIN = 0.42f;
constexpr float MIN_BANDPASS_A_FREQ_HZ = 270.0f;
constexpr float MAX_BANDPASS_A_FREQ_HZ = 730.0f;
constexpr float MIN_BANDPASS_B_FREQ_HZ = 870.0f;
constexpr float MAX_BANDPASS_B_FREQ_HZ = 1990.0f;
constexpr float MIN_BANDPASS_C_FREQ_HZ = 2240.0f;
constexpr float MAX_BANDPASS_C_FREQ_HZ = 3010.0f;
constexpr float MIN_RESONANCE = 0.0f;
constexpr float MAX_RESONANCE = 1.8f;

void toggleFilterMode();
void setFilterMode(LadderFilter::FilterMode mode);
float distance(Formant a, Formant b);
Formant nearestFormant(float freqA, float freqB, float freqC);
float clampf(float min, float max, float x);
float lerpf(float min, float max, float x);
float hardClip(float x);
float softClip(float x);

CpuLoadMeter cpuLoad;

void audioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {

  cpuLoad.OnBlockStart();

  hw.ProcessAllControls();

  b7.Debounce();
  b8.Debounce();

  freqAPot.Process();
  freqBPot.Process();
  freqCPot.Process();
  resonancePot.Process();

  freqACv.Process();
  freqBCv.Process();
  freqCCv.Process();
  resonanceCv.Process();

  bool rising = b7.RisingEdge();
  bool hard = b8.Pressed();

  if (rising) {
    toggleFilterMode();
  }

  // cv_out_2, C1 led on front panel
  hw.WriteCvOut(CV_OUT_2, hard ? 5.0f : 0.0f);

  float freqA = lerpf(MIN_BANDPASS_A_FREQ_HZ, MAX_BANDPASS_A_FREQ_HZ, clampf(0.0f, 1.0f, freqAPot.Value() + freqACv.Value()));
  float freqB = lerpf(MIN_BANDPASS_B_FREQ_HZ, MAX_BANDPASS_B_FREQ_HZ, clampf(0.0f, 1.0f, freqBPot.Value() + freqBCv.Value()));
  float freqC = lerpf(MIN_BANDPASS_C_FREQ_HZ, MAX_BANDPASS_C_FREQ_HZ, clampf(0.0f, 1.0f, freqCPot.Value() + freqCCv.Value()));
  float resonance = lerpf(MIN_RESONANCE, MAX_RESONANCE, clampf(0.0f, 1.8f, resonancePot.Value() + resonanceCv.Value()));

  // if gate1 is on, snap to nearest formant
  if (gate1.State()) {
    Formant nearest = nearestFormant(freqA, freqB, freqC);
    freqA = nearest.x;
    freqB = nearest.y;
    freqC = nearest.z;
  }

  bandpassAL.SetFreq(freqA);
  bandpassBL.SetFreq(freqB);
  bandpassCL.SetFreq(freqC);
  bandpassAR.SetFreq(freqA);
  bandpassBR.SetFreq(freqB);
  bandpassCR.SetFreq(freqC);

  bandpassAL.SetRes(resonance);
  bandpassBL.SetRes(resonance);
  bandpassCL.SetRes(resonance);
  bandpassAR.SetRes(resonance);
  bandpassBR.SetRes(resonance);
  bandpassCR.SetRes(resonance);

  // process audio buffers
  for (size_t i = 0; i < size; i++) {
    float al = bandpassAL.Process(in[0][i]);
    float bl = bandpassBL.Process(in[0][i]);
    float cl = bandpassCL.Process(in[0][i]);

    float mixL = MIX_GAIN * al + MIX_GAIN * bl + MIX_GAIN * cl;

    float ar = bandpassAR.Process(in[1][i]);
    float br = bandpassBR.Process(in[1][i]);
    float cr = bandpassCR.Process(in[1][i]);

    float mixR = MIX_GAIN * ar + MIX_GAIN * br + MIX_GAIN * cr;

    out[0][i] = hard ? hardClip(mixL) : softClip(mixL);
    out[1][i] = hard ? hardClip(mixR) : softClip(mixR);
  }

  cpuLoad.OnBlockEnd();
}

int main(void) {
  // initialize hardware, including SDRAM
  hw.Init();
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
  sampleRate = 96000.0f;

  // b7, momentary button B7, toggle mode
  b7.Init(DaisyPatchSM::B7);

  // b8, toggle switch, hard clip/soft clip
  b8.Init(DaisyPatchSM::B8);

  // gate1, snap to nearest formant
  gate1 = hw.gate_in_1;

  // four knobs, freqA freqB freqC resonance
  freqAPot.Init(hw.controls[0], 0.0f, 1.0f, Parameter::LINEAR);
  freqBPot.Init(hw.controls[1], 0.0f, 1.0f, Parameter::LINEAR);
  freqCPot.Init(hw.controls[2], 0.0f, 1.0f, Parameter::LINEAR);
  resonancePot.Init(hw.controls[3], 0.0f, 1.0f, Parameter::LINEAR);

  // four CV inputs, freqA freqB freqC resonance
  freqACv = hw.controls[4];
  freqBCv = hw.controls[5];
  freqCCv = hw.controls[6];
  resonanceCv = hw.controls[7];

  bandpassAL.Init(sampleRate);
  bandpassBL.Init(sampleRate);
  bandpassCL.Init(sampleRate);
  bandpassAR.Init(sampleRate);
  bandpassBR.Init(sampleRate);
  bandpassCR.Init(sampleRate);

  setFilterMode(LadderFilter::FilterMode::BP12);

  DaisyPatchSM::StartLog(false);
  cpuLoad.Init(hw.AudioSampleRate(), hw.AudioBlockSize());

  DaisyPatchSM::PrintLine("Starting audio callback...");
  hw.StartAudio(audioCallback);

  while (1) {
    // report audio callback CPU load once per second
    System::Delay(1000);
    DaisyPatchSM::PrintLine("CPU load avg: " FLT_FMT3 " max: " FLT_FMT3,
                            FLT_VAR3(cpuLoad.GetAvgCpuLoad()),
                            FLT_VAR3(cpuLoad.GetMaxCpuLoad()));
  }
}

void toggleFilterMode() {
  if (filterMode == 0) {
    setFilterMode(LadderFilter::FilterMode::BP24);
    filterMode = 1;
  }
  else {
    setFilterMode(LadderFilter::FilterMode::BP12);
    filterMode = 0;
  }
}

void setFilterMode(LadderFilter::FilterMode mode) {
  bandpassAL.SetFilterMode(mode);
  bandpassBL.SetFilterMode(mode);
  bandpassCL.SetFilterMode(mode);
  bandpassAR.SetFilterMode(mode);
  bandpassBR.SetFilterMode(mode);
  bandpassCR.SetFilterMode(mode);
}

float distance(Formant a, Formant b) {
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  float dz = a.z - b.z;
  return (dx * dx) + (dy * dy) + (dz * dz);
}

Formant nearestFormant(float freqA, float freqB, float freqC) {
  Formant nearest = FORMANTS[0];
  Formant target = { freqA, freqB, freqC };
  float min = distance(FORMANTS[0], target);

  for (int i = 1; i < FORMANTS_SIZE; i++) {
    float d = distance(FORMANTS[i], target);
    if (d < min) {
      min = d;
      nearest = FORMANTS[i];
    }
  }
  return nearest;
}

float clampf(float min, float max, float x) {
  if (x < min) {
    return min;
  }
  else if (x > max) {
    return max;
  }
  return x;
}

float lerpf(float min, float max, float x) {
  return min + (max - min) * x;
}

float hardClip(float x) {
  return clampf(-1.0f, 1.0f, x);
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
