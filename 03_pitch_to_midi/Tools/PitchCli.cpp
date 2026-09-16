// ----------------------------------------------------------------------------
// wav 파일을 프레임 단위로 분석해 CSV로 쓰는 콘솔 도구.
//
// 왜 필요한가: 플러그인은 호스트 안에서 실시간으로 돌기 때문에 같은 입력을
// 똑같이 재현하기 어렵다. 이 도구는 파일을 읽어 항상 같은 순서로 처리하므로
//   - 파이썬 참조 구현(prototype/yin.py)과 숫자로 비교하는 널 테스트
//   - 정답을 아는 신호로 정확도 측정
// 을 할 수 있다. 플러그인과 "같은 검출기 코드"를 컴파일해서 쓴다는 점이 핵심이다.
//
// 사용: PitchToMidiCli <input.wav> <output.csv>
// 출력 열: frame_start, time_sec, frequency_hz, confidence
//   frame_start(샘플 인덱스)는 반올림 오차가 없으므로 파이썬 결과와 줄을 맞출 때 쓴다.
// ----------------------------------------------------------------------------
#include <juce_audio_formats/juce_audio_formats.h>

#include "../Source/YinPitchDetector.h"

#include <iostream>

int main (int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: PitchToMidiCli <input.wav> <output.csv>\n";
        return 1;
    }

    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto inputFile  = cwd.getChildFile (juce::String::fromUTF8 (argv[1]));
    const auto outputFile = cwd.getChildFile (juce::String::fromUTF8 (argv[2]));

    juce::AudioFormatManager formats;
    formats.registerFormat (new juce::WavAudioFormat(), true);

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (inputFile));

    if (reader == nullptr)
    {
        std::cout << "cannot read: " << inputFile.getFullPathName() << "\n";
        return 1;
    }

    // 분석은 모노로 한다 — 왼쪽(모노 파일이면 그 채널)만 읽는다.
    const int numSamples = (int) reader->lengthInSamples;
    juce::AudioBuffer<float> audio (1, numSamples);
    reader->read (&audio, 0, numSamples, 0, true, false);

    // 파이썬 쪽(prototype/yin.py)과 반드시 같은 값을 써야 결과를 비교할 수 있다.
    static constexpr int windowSize = 2048;
    static constexpr int hopSize = 512;

    YinPitchDetector detector;
    detector.prepare (reader->sampleRate, windowSize);

    juce::String csv ("frame_start,time_sec,frequency_hz,confidence\n");

    for (int start = 0; start + windowSize <= numSamples; start += hopSize)
    {
        const auto result = detector.detect (audio.getReadPointer (0, start), windowSize);
        const double timeSec = start / reader->sampleRate;

        csv << start << ","
            << juce::String (timeSec, 6) << ","
            << juce::String (result.frequencyHz, 4) << ","
            << juce::String (result.confidence, 4) << "\n";
    }

    if (! outputFile.replaceWithText (csv))
    {
        std::cout << "cannot write: " << outputFile.getFullPathName() << "\n";
        return 1;
    }

    std::cout << "wrote " << outputFile.getFullPathName() << "\n";
    return 0;
}
