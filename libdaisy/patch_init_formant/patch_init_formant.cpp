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

DaisyPatchSM hw;
Switch b7;
Switch b8;
Parameter freqAPot;
Parameter freqBPot;
Parameter freqCPot;
Parameter resonancePot;
AnalogControl freqACv;
AnalogControl freqBCv;
AnalogControl freqCCv;
AnalogControl resonanceCv;

int filterMode;
float sampleRate;

float freqA;
float freqB;
float freqC;
float resonance;

LadderFilter bandpassA;
LadderFilter bandpassB;
LadderFilter bandpassC;

#define MIN_BANDPASS_A_FREQ_HZ 270.0f
#define MAX_BANDPASS_A_FREQ_HZ 730.0f
#define MIN_BANDPASS_B_FREQ_HZ 870.0f
#define MAX_BANDPASS_B_FREQ_HZ 1990.0f
#define MIN_BANDPASS_C_FREQ_HZ 2240.0f
#define MAX_BANDPASS_C_FREQ_HZ 3010.0f
#define MIN_RESONANCE 0.0f
#define MAX_RESONANCE 1.8f

void toggleFilterMode();
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

  freqA = lerpf(MIN_BANDPASS_A_FREQ_HZ, MAX_BANDPASS_A_FREQ_HZ, clampf(0.0f, 1.0f, freqAPot.Value() + freqACv.Value()));
  freqB = lerpf(MIN_BANDPASS_B_FREQ_HZ, MAX_BANDPASS_B_FREQ_HZ, clampf(0.0f, 1.0f, freqBPot.Value() + freqBCv.Value()));
  freqC = lerpf(MIN_BANDPASS_C_FREQ_HZ, MAX_BANDPASS_C_FREQ_HZ, clampf(0.0f, 1.0f, freqCPot.Value() + freqCCv.Value()));
  resonance = lerpf(MIN_RESONANCE, MAX_RESONANCE, clampf(0.0f, 1.0f, resonancePot.Value() + resonanceCv.Value()));

  bandpassA.SetFreq(freqA);
  bandpassB.SetFreq(freqB);
  bandpassC.SetFreq(freqC);

  bandpassA.SetRes(resonance);
  bandpassB.SetRes(resonance);
  bandpassC.SetRes(resonance);
  
  // process audio buffers
  for (size_t i = 0; i < size; i++) {
    float al = bandpassA.Process(in[0][i]);
    float bl = bandpassB.Process(in[0][i]);
    float cl = bandpassC.Process(in[0][i]);

    float mixL = 0.40f * al + 0.40f * bl + 0.40f * cl;

    float ar = bandpassA.Process(in[1][i]);
    float br = bandpassB.Process(in[1][i]);
    float cr = bandpassC.Process(in[1][i]);

    float mixR = 0.40f * ar + 0.40f * br + 0.40f * cr;

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

  bandpassA.Init(sampleRate);
  bandpassB.Init(sampleRate);
  bandpassC.Init(sampleRate);

  bandpassA.SetFilterMode(LadderFilter::FilterMode::BP12);
  bandpassB.SetFilterMode(LadderFilter::FilterMode::BP12);
  bandpassC.SetFilterMode(LadderFilter::FilterMode::BP12);

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
    bandpassA.SetFilterMode(LadderFilter::FilterMode::BP24);
    bandpassB.SetFilterMode(LadderFilter::FilterMode::BP24);
    bandpassC.SetFilterMode(LadderFilter::FilterMode::BP24);
    filterMode = 1;
  }
  else {
    bandpassA.SetFilterMode(LadderFilter::FilterMode::BP12);
    bandpassB.SetFilterMode(LadderFilter::FilterMode::BP12);
    bandpassC.SetFilterMode(LadderFilter::FilterMode::BP12);
    filterMode = 0;
  }
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
