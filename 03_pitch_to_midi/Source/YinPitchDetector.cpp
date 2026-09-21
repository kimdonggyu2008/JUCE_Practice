// ----------------------------------------------------------------------------
// YIN — prototype/yin.py를 C++로 옮긴 것.
//
// 실습: 빈칸(___)이 5개 있다. 빈칸마다 바로 위에 파이썬 원본 줄이 적혀 있다.
//       같은 계산을 C++로 쓰면 된다. `___` 세 글자만 지우고 그 자리에 쓴다.
//
// 채점: 5개를 다 채운 뒤
//         cd 03_pitch_to_midi\prototype
//         .venv\Scripts\python.exe null_test.py
//       빌드 → 테스트 신호 7개를 파이썬과 C++에 똑같이 넣고 → 같은 답인지 확인한다.
//       (C++은 파일 전체가 한 번에 컴파일되므로, 빈칸이 하나라도 남으면 빌드가 실패한다.
//        그때 나오는 에러 "'___': 선언되지 않은 식별자"가 아직 안 채운 빈칸의 줄 번호다.)
//
// 파이썬 → C++ 번역에서 알아둘 것
//   - numpy는 배열 전체를 한 번에 계산했지만, C++은 for 문으로 하나씩 돈다.
//     그래서 파이썬 한 줄이 C++에서는 반복문 안의 한 줄이 된다.
//   - 파이썬 d[tau]  →  C++ difference[(size_t) tau]
//     (size_t)는 배열 인덱스용 타입으로 바꾸는 것. 안 붙이면 경고가 난다.
//   - 파이썬 x ** 2  →  C++ x * x   (C++에는 ** 연산자가 없다)
// ----------------------------------------------------------------------------
#include "YinPitchDetector.h"

#include <algorithm>

void YinPitchDetector::prepare (double sampleRate, int newWindowSize)
{
    currentSampleRate = sampleRate;
    windowSize = newWindowSize;

    // 미는 거리 τ는 0 ~ 창의 절반까지. 버퍼 크기를 여기서 미리 잡는다 (오디오 스레드 할당 금지).
    difference.assign ((size_t) (windowSize / 2), 0.0);
    normalised.assign ((size_t) (windowSize / 2), 1.0);
}

PitchDetector::Result YinPitchDetector::detect (const float* samples, int numSamples)
{
    const int maxTau = (int) difference.size();   // 1024
    const int W = numSamples - maxTau;            // 비교할 구간 길이. 1024
    if (W <= 0)
        return {};

    // ── 1단계: 차이 함수 d(τ) ────────────────────────────────────────────────
    // 파이썬:
    //     for tau in range(max_tau):
    //         b = frame[tau:tau + W]
    //         d[tau] = np.sum((a - b) ** 2)
    // numpy의 np.sum((a - b) ** 2)는 "1024개를 하나씩 빼고, 제곱하고, 더하기"다.
    // C++에서는 그 "하나씩"을 안쪽 for 문으로 직접 돈다.
    for (int tau = 0; tau < maxTau; ++tau)
    {
        double sum = 0.0;
        for (int j = 0; j < W; ++j)
        {
            const double delta = (double) samples[j] - (double) samples[j + tau];   // a[j] - b[j]

            // ┌─ 빈칸 1 ───────────────────────────────────────────────────────┐
            // │ 파이썬: (a - b) ** 2                                            │
            // │ delta가 (a - b)다. 그걸 제곱해서 sum에 더한다.                 │
            // │ C++에는 ** 가 없다 — 제곱은 자기 자신을 곱한다.                │
            // └─────────────────────────────────────────────────────────────────┘
            sum += (delta * delta);
        }
        difference[(size_t) tau] = sum;
    }

    // ── 2단계: 누적 평균 정규화 d'(τ) = d(τ) ÷ (지금까지의 평균) ─────────────
    // 파이썬:
    //     running_sum = np.cumsum(d[1:])        # 누적 합
    //     mean = running_sum / tau              # 지금까지의 평균
    //     d_prime = d / mean                    # (mean이 0이면 1)
    normalised[0] = 1.0;
    double runningSum = 0.0;
    for (int tau = 1; tau < maxTau; ++tau)
    {
        runningSum += difference[(size_t) tau];
        const double mean = runningSum / tau;

        // ┌─ 빈칸 2 ───────────────────────────────────────────────────────────┐
        // │ 파이썬: d[tau] / mean                                               │
        // │ 이 줄은 "조건 ? 참일 때 : 거짓일 때" 문법이다 (삼항 연산자).        │
        // │ mean이 0보다 크면 빈칸 값을, 아니면(무음) 1.0을 넣는다.             │
        // │ 파이썬의 d[tau]는 여기서 difference[(size_t) tau]다.                │
        // └─────────────────────────────────────────────────────────────────────┘
        normalised[(size_t) tau] = mean > 0.0 ? difference[(size_t) tau] / mean : 1.0;
    }

    // ── 3단계: 기준값 아래로 내려가는 첫 골짜기 ──────────────────────────────
    // 파이썬:
    //     tau = 2
    //     while tau < len(d_prime):
    //         if d_prime[tau] < threshold:
    //             ... (골짜기 바닥까지 내려감)
    //             return tau
    //         tau += 1
    //     return -1                              # 못 찾음 = 음정 없음
    int tau = 2;
    for (; tau < maxTau; ++tau)
    {
        // ┌─ 빈칸 3 ───────────────────────────────────────────────────────────┐
        // │ 파이썬: d_prime[tau] < threshold                                    │
        // │ 조건이 참이면 break로 반복을 멈춘다 — 그 tau가 첫 골짜기.           │
        // │ 파이썬의 d_prime는 여기서 normalised다.                              │
        // └─────────────────────────────────────────────────────────────────────┘
        if (normalised[(size_t) tau] < threshold)
            break;
    }

    if (tau >= maxTau)
        return {};                                // 끝까지 못 찾음 → 음정 없음 (파이썬의 return -1)

    // 기준 아래로 들어왔으면, 계속 내려가는 동안 따라가서 골짜기 바닥을 찾는다.
    while (tau + 1 < maxTau && normalised[(size_t) tau + 1] < normalised[(size_t) tau])
        ++tau;

    // ── 4단계: 포물선 보간 — τ를 소수점까지 다듬기 ────────────────────────────
    // 파이썬:
    //     s0, s1, s2 = d_prime[tau - 1], d_prime[tau], d_prime[tau + 1]
    //     denominator = s0 - 2 * s1 + s2
    //     return tau + (s0 - s2) / (2 * denominator)
    double betterTau = tau;
    if (tau > 0 && tau < maxTau - 1)
    {
        const double s0 = normalised[(size_t) tau - 1];
        const double s1 = normalised[(size_t) tau];
        const double s2 = normalised[(size_t) tau + 1];
        const double denominator = s0 - 2.0 * s1 + s2;

        if (denominator != 0.0)
        {
            // ┌─ 빈칸 4 ───────────────────────────────────────────────────────┐
            // │ 파이썬: tau + (s0 - s2) / (2 * denominator)                     │
            // │ 거의 그대로 옮기면 된다. 괄호 위치만 조심.                      │
            // └─────────────────────────────────────────────────────────────────┘
            betterTau = tau + (s0 - s2) / (2 * denominator);
        }
    }

    // ── 5단계: 주기(샘플 수) → 주파수(Hz) ─────────────────────────────────────
    // 파이썬:
    //     return sample_rate / better_tau, confidence
    Result result;

    // ┌─ 빈칸 5 ───────────────────────────────────────────────────────────────┐
    // │ 파이썬: sample_rate / better_tau                                        │
    // │ 이 클래스에서 샘플레이트는 currentSampleRate라는 멤버 변수에 있다.       │
    // └─────────────────────────────────────────────────────────────────────────┘
    result.frequencyHz = (float) (currentSampleRate / betterTau);

    result.confidence = (float) std::clamp (1.0 - normalised[(size_t) tau], 0.0, 1.0);   // 파이썬: np.clip(1 - d_prime[tau], 0, 1)
    return result;
}

int YinPitchDetector::getLatencySamples() const
{
    // TODO: 분석 창이 "과거 windowSize 샘플"을 보고 판단하므로 대략 창 크기만큼 늦다.
    //       실제로 몇 샘플인지는 측정해서 확정한다.
    return windowSize;
}
