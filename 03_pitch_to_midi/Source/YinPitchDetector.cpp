#include "YinPitchDetector.h"

void YinPitchDetector::prepare (double sampleRate, int newWindowSize)
{
    currentSampleRate = sampleRate;
    windowSize = newWindowSize;

    // 지연(τ)은 최대 창의 절반까지 본다.
    difference.assign ((size_t) (windowSize / 2), 0.0f);
}

PitchDetector::Result YinPitchDetector::detect (const float* samples, int numSamples)
{
    // TODO: prototype/yin.py에서 검증한 단계를 그대로 옮긴다.
    //   1) 차이 함수 d(τ)
    //   2) 누적 평균 정규화 d'(τ)
    //   3) 절대 임계값으로 첫 번째 골짜기 τ 찾기
    //   4) 포물선 보간으로 τ를 소수점까지 다듬기
    //   5) f0 = sampleRate / τ, confidence = 1 - d'(τ)
    (void) samples;
    (void) numSamples;
    return {};
}

int YinPitchDetector::getLatencySamples() const
{
    // TODO: 분석 창이 "과거 windowSize 샘플"을 보고 판단하므로 대략 창 크기만큼 늦다.
    //       실제로 몇 샘플인지는 측정해서 확정한다.
    return windowSize;
}
