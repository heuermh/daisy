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

Oscillator osc;
Adsr env;

CpuLoadMeter cpuLoad;

void audioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {

  cpuLoad.OnBlockStart();

  hw.ProcessAllControls();

  b7.Debounce();
  b8.Debounce();

  bool rising = b7.RisingEdge();
  bool hard = b8.Pressed();

  if (rising) {
    // random MIDI note 48 to 72
    osc.SetFreq(mtof(48 + Random::GetValue() % 25));

    // true is hard retrigger
    env.Retrigger(false);
  }

  // cv_out_2, C1 led on front panel
  hw.WriteCvOut(CV_OUT_2, hard ? 5.0f : 0.0f);

  // process audio buffers
  for (size_t i = 0; i < size; i++) {
    float v = osc.Process() * env.Process(false);
    out[0][i] = out[1][i] = hard ? fclamp(v, -1.0f, 1.0f) : SoftClip(v);
  }

  cpuLoad.OnBlockEnd();
}

int main(void) {

  // initialize hardware
  hw.Init();
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);

  // b7, momentary button B7, trigger
  b7.Init(DaisyPatchSM::B7);

  // b8, toggle switch, hard clip/soft clip
  b8.Init(DaisyPatchSM::B8);

  // sine oscillator
  osc.Init(hw.AudioSampleRate());
  osc.SetWaveform(Oscillator::WAVE_SIN);
  osc.SetFreq(220.0f);
  osc.SetAmp(0.5f);

  // envelope, 5 ms attack, 100 ms decay, 0.0 sustain, 10 ms release
  env.Init(hw.AudioSampleRate());
  env.SetTime(ADSR_SEG_ATTACK, 0.005f);
  env.SetTime(ADSR_SEG_DECAY, 0.1f);
  env.SetSustainLevel(0.0f);
  env.SetTime(ADSR_SEG_RELEASE, 0.01f);

  // log output over USB serial
  DaisyPatchSM::StartLog(false);

  cpuLoad.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
  hw.StartAudio(audioCallback);

  while (1) {
    // report audio callback CPU load once per second
    System::Delay(1000);
    DaisyPatchSM::PrintLine("CPU load avg: " FLT_FMT3 " max: " FLT_FMT3,
                            FLT_VAR3(cpuLoad.GetAvgCpuLoad()),
                            FLT_VAR3(cpuLoad.GetMaxCpuLoad()));
  }
}
