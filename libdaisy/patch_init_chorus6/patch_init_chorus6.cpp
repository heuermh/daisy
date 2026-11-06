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
Parameter ratePot;
Parameter depthPot;
Parameter phasePot;
Parameter shapePot;
AnalogControl rateCv;
AnalogControl depthCv;
AnalogControl phaseCv;
AnalogControl shapeCv;

int mode;
float sampleRate;

float rate;
float depth;
float phase;
float shape;
float lastPhase;

// max delay 20 ms
#define MAX_DELAY static_cast<size_t>(96000 * 0.020f)
#define PRE_LPF_FREQ_HZ 7237.0f
#define POST_LPF_FREQ_HZ 10644.0f

static DelayLine<float, MAX_DELAY> delayL;
static DelayLine<float, MAX_DELAY> delayR;

LadderFilter preLpfL;
LadderFilter preLpfR;
LadderFilter postLpfL;
LadderFilter postLpfR;
Oscillator sinLfoL;
Oscillator sinLfoR;
Oscillator triLfoL;
Oscillator triLfoR;

void toggleMode();
float clampf(float min, float max, float x);
float lerpf(float min, float max, float x);
float truncf(float f);
float hardClip(float x);
float softClip(float x);

CpuLoadMeter cpuLoad;

void audioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {

  cpuLoad.OnBlockStart();

  hw.ProcessAllControls();

  b7.Debounce();
  b8.Debounce();

  ratePot.Process();
  depthPot.Process();
  phasePot.Process();
  shapePot.Process();
  rateCv.Process();
  depthCv.Process();
  phaseCv.Process();
  shapeCv.Process();

  bool rising = b7.RisingEdge();
  bool hard = b8.Pressed();

  if (rising) {
    toggleMode();
  }

  // cv_out_2, C1 led on front panel
  hw.WriteCvOut(CV_OUT_2, hard ? 5.0f : 0.0f);

  if (mode == 2) {
    lastPhase = phase;

    // y = 0.0125 * 81^x - 0.0125
    rate = lerpf(0.001f, 2.0f, 0.0125f * pow(81.0f, clampf(0.0f, 1.0f, ratePot.Value())) - 0.0125f); // 0.001 to 2.0, exp-ish
    depth = clampf(0.0f, 3.0f, depthPot.Value() + (depthCv.Value() * 3.0f)); // 0.0 to 3.0, linear
    phase = clampf(0.0f, 1.0f, phasePot.Value() + phaseCv.Value()); // 0.0 to 1.0, linear
    shape = clampf(0.0f, 1.0f, shapePot.Value() + shapeCv.Value()); // 0.0 to 1.0, linear

    triLfoL.SetFreq(rate);
    triLfoR.SetFreq(rate);
    sinLfoL.SetFreq(rate);
    sinLfoR.SetFreq(rate);

    triLfoL.SetAmp(depth);
    triLfoR.SetAmp(depth);
    sinLfoL.SetAmp(depth);
    sinLfoR.SetAmp(depth);

    triLfoR.PhaseAdd(phase - lastPhase);
    sinLfoR.PhaseAdd(phase - lastPhase);
  }

  // process audio buffers
  for (size_t i = 0; i < size; i++) {

    // apply lfo to left and right delays
    switch (mode) {
    case 0:
    case 1:
      delayL.SetDelay(((triLfoL.Process() + 3.345f) / 1000.0f) * sampleRate);
      delayR.SetDelay(((triLfoR.Process() + 3.345f) / 1000.0f) * sampleRate);
      sinLfoL.Process();
      sinLfoR.Process();
      break;
    case 2:
      float shapeL = triLfoL.Process() * shape + sinLfoL.Process() * (1.0f - shape);
      float shapeR = triLfoR.Process() * shape + sinLfoR.Process() * (1.0f - shape);
      delayL.SetDelay(((shapeL + 3.345f) / 1000.0f) * sampleRate);
      delayR.SetDelay(((shapeR + 3.345f) / 1000.0f) * sampleRate);
      break;
    }

    // read dry from input
    float dryL = in[0][i];
    float dryR = in[1][i];

    // read wet from delay line
    //float wetL = postLpfL.Process(delayL.Read());
    //float wetR = postLpfR.Process(delayR.Read());
    float wetL = delayL.Read();
    float wetR = delayR.Read();

    // write to delay line
    delayL.Write(preLpfL.Process(dryL));
    delayR.Write(preLpfR.Process(dryR));
    //delayL.Write(dryL);
    //delayR.Write(dryR);

    // mix wet and dry
    //float mixL = wetL * 0.5646f + dryL * 0.4354f;
    //float mixR = wetR * 0.5646f + dryR * 0.4354f;

    // bump by 1.3f
    float mixL = wetL * 0.73398f + dryL * 0.56602f;
    float mixR = wetR * 0.73398f + dryR * 0.56602f;

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

  // four knobs, rate depth phase mix
  ratePot.Init(hw.controls[0], 0.0f, 1.0f, Parameter::LINEAR);
  depthPot.Init(hw.controls[1], 0.0f, 3.0f, Parameter::LINEAR);
  phasePot.Init(hw.controls[2], 0.0f, 1.0f, Parameter::LINEAR);
  shapePot.Init(hw.controls[3], 0.0f, 1.0f, Parameter::LINEAR);

  // four CV inputs, rate depth phase mix
  rateCv = hw.controls[4];
  depthCv = hw.controls[5];
  phaseCv = hw.controls[6];
  shapeCv = hw.controls[7];

  // init left channel
  delayL.Init();
  preLpfL.Init(sampleRate);
  postLpfL.Init(sampleRate);
  sinLfoL.Init(sampleRate);
  triLfoL.Init(sampleRate);

  preLpfL.SetFreq(PRE_LPF_FREQ_HZ);
  postLpfL.SetFreq(POST_LPF_FREQ_HZ);

  preLpfL.SetFilterMode(LadderFilter::FilterMode::LP12);
  postLpfL.SetFilterMode(LadderFilter::FilterMode::LP12);

  sinLfoL.SetWaveform(Oscillator::WAVE_SIN);
  triLfoL.SetWaveform(Oscillator::WAVE_TRI);

  // init right channel
  delayR.Init();
  preLpfR.Init(sampleRate);
  postLpfR.Init(sampleRate);
  sinLfoR.Init(sampleRate);
  triLfoR.Init(sampleRate);

  preLpfR.SetFreq(PRE_LPF_FREQ_HZ);
  postLpfR.SetFreq(POST_LPF_FREQ_HZ);

  preLpfR.SetFilterMode(LadderFilter::FilterMode::LP12);
  postLpfR.SetFilterMode(LadderFilter::FilterMode::LP12);

  sinLfoR.SetWaveform(Oscillator::WAVE_SIN);
  triLfoR.SetWaveform(Oscillator::WAVE_TRI);

  // start with mode I
  mode = -1;
  toggleMode();

  DaisyPatchSM::StartLog(false);
  cpuLoad.Init(hw.AudioSampleRate(), hw.AudioBlockSize());

  DaisyPatchSM::PrintLine("Starting audio callback...");
  hw.StartAudio(audioCallback);

  while (1) {
    // report audio callback CPU load once per second
    System::Delay(1000);
    DaisyPatchSM::PrintLine("Current mode: %u", (unsigned) mode);
    DaisyPatchSM::PrintLine("CPU load avg: " FLT_FMT3 " max: " FLT_FMT3,
                            FLT_VAR3(cpuLoad.GetAvgCpuLoad()),
                            FLT_VAR3(cpuLoad.GetMaxCpuLoad()));
  }
}

void toggleMode() {
  mode++;
  if (mode > 2) {
    mode = 0;
  }
  switch (mode) {
  case 0:
    // mode I
    triLfoL.SetFreq(0.4f);
    triLfoL.Reset(0.0f); // phase
    triLfoL.SetAmp(1.805f);

    triLfoR.SetFreq(0.4f);
    triLfoR.Reset(0.5f); // phase
    triLfoR.SetAmp(1.945f);
    break;
  case 1:
    // mode II
    triLfoL.SetFreq(0.67f);
    triLfoL.Reset(0.0f); // phase
    triLfoL.SetAmp(1.805f);

    triLfoR.SetFreq(0.67f);
    triLfoR.Reset(0.5f); // phase
    triLfoR.SetAmp(1.945f);
    break;
  case 2:
    // manual
    break;
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

float truncf(float f) {
  return roundf(f * 1000.0f) / 1000.0f;
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
