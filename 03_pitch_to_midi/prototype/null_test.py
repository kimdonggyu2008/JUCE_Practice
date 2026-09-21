"""널 테스트: 파이썬 YIN(yin.py)과 C++ YIN(YinPitchDetector.cpp)이 같은 답을 내는지 확인한다.

    cd 03_pitch_to_midi\\prototype
    .venv\\Scripts\\python.exe null_test.py

하는 일:
  1. C++ 분석 도구(PitchToMidiCli)를 빌드한다. 빌드가 실패하면 에러를 보여주고 멈춘다.
  2. 테스트 신호마다 같은 wav를
       - 파이썬 yin.track()으로 분석하고
       - C++ PitchToMidiCli로 분석해서
     frame_start로 줄을 맞춰 프레임마다 비교한다.
  3. 신호마다 ✅/❌. ❌면 처음 어긋난 프레임을 보여준다.

완전히 0 차이는 아니다. 파이썬(numpy)과 C++이 소수 계산 순서가 조금 달라서
아주 작은 차이가 남는다. 그 허용 범위가 TOLERANCE_HZ다.
이걸 통과해야 "파이썬에서 검증한 알고리즘을 C++로 정확히 옮겼다"고 말할 수 있다.
"""

import csv
import subprocess
import sys
from pathlib import Path

import soundfile as sf

from signals import OUT_DIR
from yin import track

sys.stdout.reconfigure(encoding="utf-8")

REPO = Path(__file__).resolve().parents[2]
EXE = REPO / "build" / "03_pitch_to_midi" / "PitchToMidiCli_artefacts" / "Debug" / "PitchToMidiCli.exe"
CPP_OUT = OUT_DIR / "cpp"

SIGNALS = ["sine_220", "sine_880", "harmonic_110", "glide_110_440", "vibrato_c4", "silence", "noise"]

TOLERANCE_HZ = 0.01         # 주파수 차이 허용치
TOLERANCE_CONF = 0.001      # 확신도 차이 허용치


def _decode(raw: bytes) -> str:
    """빌드 출력은 줄마다 UTF-8이기도 하고 한국어 윈도우 인코딩(cp949)이기도 하다. 줄마다 맞는 쪽으로 읽는다."""
    lines = []
    for line in raw.split(b"\n"):
        try:
            lines.append(line.decode("utf-8"))
        except UnicodeDecodeError:
            lines.append(line.decode("cp949", errors="replace"))
    return "\n".join(lines)


def build() -> bool:
    """C++ 분석 도구를 빌드한다. 성공하면 True."""
    print("  C++ 빌드 중... (처음엔 1분쯤, 이후엔 몇 초)")
    result = subprocess.run(
        ["cmake", "--build", "build", "--target", "PitchToMidiCli", "--config", "Debug"],
        cwd=REPO, capture_output=True,
    )
    output = _decode(result.stdout) + "\n" + _decode(result.stderr)

    if result.returncode != 0:
        errors = []
        for line in output.splitlines():
            if ": error" not in line:
                continue
            # 끝의 " [...vcxproj]"를 먼저 떼고, 앞의 긴 폴더 경로를 떼서
            # "YinPitchDetector.cpp(58,20): error C2065: ..." 부분만 남긴다
            short = line.strip().split(" [")[0].split("\\")[-1]
            if short not in errors:
                errors.append(short)

        print("  ❌ 빌드 실패 — C++ 문법 에러가 있습니다. 첫 번째부터 보세요:")
        print("     (파일이름(줄,칸): error 번호: 내용)\n")
        for line in errors[:6]:
            print(f"     {line}")
        return False

    print("  빌드 완료\n")
    return True


def load_csv(path: Path) -> dict[int, tuple[float, float]]:
    """csv → {frame_start: (frequency_hz, confidence)}"""
    with open(path, encoding="utf-8") as f:
        return {int(r["frame_start"]): (float(r["frequency_hz"]), float(r["confidence"])) for r in csv.DictReader(f)}


def compare(python: dict, cpp: dict) -> tuple[bool, str]:
    """프레임마다 비교. (통과 여부, 설명) 을 돌려준다."""
    if python.keys() != cpp.keys():
        return False, f"프레임 수가 다름 — 파이썬 {len(python)}개, C++ {len(cpp)}개"

    max_diff = 0.0
    for frame_start in sorted(python):
        py_hz, py_conf = python[frame_start]
        cpp_hz, cpp_conf = cpp[frame_start]

        if (py_hz > 0) != (cpp_hz > 0):
            which = "파이썬만 음정 있음" if py_hz > 0 else "C++만 음정 있음"
            return False, f"프레임 {frame_start}: {which} (파이썬 {py_hz:.2f}Hz / C++ {cpp_hz:.2f}Hz)"

        diff = abs(py_hz - cpp_hz)
        if diff > TOLERANCE_HZ:
            return False, f"프레임 {frame_start}: 파이썬 {py_hz:.4f}Hz / C++ {cpp_hz:.4f}Hz (차이 {diff:.4f})"
        if abs(py_conf - cpp_conf) > TOLERANCE_CONF:
            return False, f"프레임 {frame_start}: 확신도 파이썬 {py_conf:.4f} / C++ {cpp_conf:.4f}"

        max_diff = max(max_diff, diff)

    return True, f"{len(python)}프레임 일치 (최대 차이 {max_diff:.5f} Hz)"


if __name__ == "__main__":
    skip_build = "--no-build" in sys.argv
    if not skip_build and not build():
        sys.exit(1)

    CPP_OUT.mkdir(exist_ok=True)
    passed = 0

    for name in SIGNALS:
        wav = OUT_DIR / f"{name}.wav"
        cpp_csv = CPP_OUT / f"{name}_cpp.csv"

        # C++
        run = subprocess.run([str(EXE), str(wav), str(cpp_csv)], capture_output=True)
        if run.returncode != 0:
            print(f"  ❌ {name:15s} C++ 도구 실행 실패")
            continue

        # 파이썬
        audio, sr = sf.read(wav, dtype="float32")
        python = {frame_start: (hz, conf) for frame_start, hz, conf in track(audio, sr)}

        ok, message = compare(python, load_csv(cpp_csv))
        print(f"  {'✅' if ok else '❌'} {name:15s} {message}")
        passed += ok

    print(f"\n  {passed} / {len(SIGNALS)} 일치", end="")
    print("   — 파이썬과 C++이 같은 답을 냅니다!" if passed == len(SIGNALS) else "")
