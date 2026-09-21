#pragma once

#include "PitchDetector.h"

#include <vector>

// ----------------------------------------------------------------------------
// YIN 음정 검출 (de Cheveigné & Kawahara, 2002).
//
// prototype/yin.py를 한 줄씩 옮긴 것. 두 구현이 같은 답을 내는지는
// prototype/null_test.py가 테스트 신호로 숫자를 비교해서 확인한다.
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

    // 이 값보다 d'(τ)가 작아지는 첫 골짜기를 음정으로 본다. (yin.py의 threshold=0.1)
    static constexpr double threshold = 0.1;

    // 작업 버퍼. detect()에서 새로 만들면 오디오 스레드에서 메모리를 할당하게 되므로
    // prepare()에서 크기를 미리 잡아둔다.
    // double로 두는 이유: 제곱을 1024번 더하므로 float보다 오차가 덜 쌓이고, 파이썬(numpy)과 결과가 더 가깝다.
    std::vector<double> difference;   // 1단계 d(τ)   — yin.py의 d
    std::vector<double> normalised;   // 2단계 d'(τ)  — yin.py의 d_prime
};
