#pragma once

// ----------------------------------------------------------------------------
// 음정 검출기의 공통 모양 (인터페이스).
//
// 고전 DSP 방법(YIN)과 나중에 붙일 신경망 모델이 "같은 자리"에 끼워지도록
// 이 모양만 약속한다. 플러그인과 콘솔 도구는 구체적인 방법을 모른 채
// 이 인터페이스만 보고 쓴다 — 그래서 방법을 바꿔 끼워 같은 조건에서 비교할 수 있다.
//
// 파이썬으로 치면 abc.ABC를 상속한 추상 클래스와 같다.
// "= 0"이 붙은 함수는 몸체가 없고, 상속받는 쪽이 반드시 채워야 한다.
// ----------------------------------------------------------------------------
class PitchDetector
{
public:
    struct Result
    {
        float frequencyHz = 0.0f;   // 0이면 "음정 없음" (무음·잡음·무성음)
        float confidence  = 0.0f;   // 0~1. 얼마나 확신하는지
    };

    virtual ~PitchDetector() = default;

    // 재생 시작 전에 한 번 불린다. 필요한 작업 메모리는 전부 여기서 잡는다.
    // windowSize = detect()에 한 번에 넘겨줄 샘플 수.
    virtual void prepare (double sampleRate, int windowSize) = 0;

    // 분석 창 하나를 받아 음정을 추정한다.
    // 플러그인에서는 오디오 스레드에서 불리므로 메모리 할당·락·파일 접근 금지.
    virtual Result detect (const float* samples, int numSamples) = 0;

    // 이 방법이 결과를 내기까지 필요한 지연(샘플 수).
    // 고전 방법과 신경망을 비교할 때 정확도와 함께 반드시 같이 봐야 하는 값이다.
    virtual int getLatencySamples() const = 0;
};
