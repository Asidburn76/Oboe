/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef SHARED_SYNTH_SOUND_H
#define SHARED_SYNTH_SOUND_H


#include <cstdint>
#include <atomic>
#include <math.h>
#include <memory>
#include "IRenderableAudio.h"

constexpr double kDefaultFrequency = 440.0;
constexpr int32_t kDefaultSampleRate = 48000;
constexpr double kPi = M_PI;
constexpr double kTwoPi = kPi * 2;
constexpr int32_t kNumSineWaves = 5;

class SynthSound : public IRenderableAudio {

public:

    SynthSound()
    {
        for (int i = 0; i < kNumSineWaves; i++) {
            mFrequencies[i] = kDefaultFrequency * (i + 1);
        }
    }

    ~SynthSound() = default;

    void setWaveOn(bool isWaveOn) {
        mIsWaveOn.store(isWaveOn);
    };

    void setSampleRate(int32_t sampleRate) {
        mSampleRate = sampleRate;
        updatePhaseIncrement();
    };

    void setFrequency(double frequency) {
        for (int i = 0; i < kNumSineWaves; i++) {
            mFrequencies[i] = frequency * (i + 1);
        }

        updatePhaseIncrement();
    };

    // Amplitudes from https://epubs.siam.org/doi/pdf/10.1137/S00361445003822
    inline void setAmplitude(float amplitude) {
        mAmplitudes[0] = amplitude * .2f;
        mAmplitudes[1] = amplitude;
        mAmplitudes[2] = amplitude * .1f;
        mAmplitudes[3] = amplitude * .02f;
        mAmplitudes[4] = amplitude * .15f;
    };

    // From IRenderableAudio
    void renderAudio(float *audioData, int32_t numFrames) override {

        if (mIsWaveOn){
            for (int i = 0; i < numFrames; ++i) {
                audioData[i] = 0;
                for (int j = 0; j < kNumSineWaves; ++j) {
                    audioData[i] += sinf(mPhases[j]) * mAmplitudes[j];

                    mPhases[j] += mPhaseIncrements[j];
                    if (mPhases[j] > kTwoPi) mPhases[j] -= kTwoPi;
                }
            }
        } else {
            memset(audioData, 0, sizeof(float) * numFrames);
        }
    };

private:
    std::atomic<bool> mIsWaveOn { false };
    std::array<float, kNumSineWaves> mAmplitudes;
    std::array<float, kNumSineWaves> mPhases;
    std::array<double, kNumSineWaves> mPhaseIncrements;
    std::array<double, kNumSineWaves> mFrequencies;
    int32_t mSampleRate = kDefaultSampleRate;

    void updatePhaseIncrement(){
        for (int i = 0; i < kNumSineWaves; i++) {
            mPhaseIncrements[i] = (kTwoPi * mFrequencies[i]) / static_cast<double>(mSampleRate);
        }
    };
};

#endif //SHARED_SYNTH_SOUND_H
