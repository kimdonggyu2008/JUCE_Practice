#pragma once

#include "PitchDetector.h"

#include <vector>

// ----------------------------------------------------------------------------
// YIN 음정 검출 (de Cheveigné & Kawahara, 2002).
//
// 구현 순서는 prototype/yin.py와 같게 맞춘다 — 파이썬에서 먼저 검증하고,
// 같은 단계를 여기로 옮긴 뒤, 콘솔 도구로 두 결과를 숫자로 비교한다.
// ----------------------------------------------------------------------------
class YinPitchDetector final : public PitchDetector
{
public:
    void prepare (double sampleRate, int windowSize) override;
    Result detect (const float* samples, int numSamples) override;
    int getLatencySamples() const override;

private:
    double currentSampleRate = 44100.0;
    int windowSize = 2048;

    // 작업 버퍼. detect()에서 새로 만들면 오디오 스레드 할당이 되므로 prepare()에서 미리 잡는다.
    std::vector<float> difference;
};
