"""pesto_detect.py 빈칸 채점기.

    cd 03_pitch_to_midi\\prototype
    .venv\\Scripts\\python.exe practice\\check_pesto.py
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
sys.stdout.reconfigure(encoding="utf-8")

try:
    import pesto_detect
except SyntaxError as e:
    print(f"  ⚠️ pesto_detect.py {e.lineno}번 줄: 파이썬 문법이 아닙니다 (파일 전체를 읽지 못해 채점을 못 함)")
    print(f"        {e.text.strip() if e.text else ''}")
    sys.exit(1)

import soundfile as sf

from signals import OUT_DIR


class NotFilled(Exception):
    pass


def _call(fn, *args):
    try:
        return fn(*args)
    except NameError as e:
        if "___" in str(e):
            raise NotFilled from None
        raise


def check_blank1():
    got = _call(pesto_detect.step_size_ms, 48000)
    assert abs(got - 512 / 48000 * 1000) < 1e-9, f"step_size_ms(48000)이 10.666...이어야 하는데 {got!r}"
    got = _call(pesto_detect.step_size_ms, 16000)
    assert abs(got - 32.0) < 1e-9, f"step_size_ms(16000)이 32.0이어야 하는데 {got!r}"


def check_blank2():
    got = _call(pesto_detect.frame_offset)
    assert got == 2, f"frame_offset()이 2여야 하는데 {got!r}"
    assert isinstance(got, int), f"정수여야 하는데 {type(got).__name__}. // (정수 나눗셈)을 썼나요?"

    # 창 크기를 바꿔도 맞게 나오는지 — 숫자 2를 직접 썼으면 여기서 걸린다
    original = pesto_detect.WINDOW_SIZE
    try:
        pesto_detect.WINDOW_SIZE = 4096
        got = _call(pesto_detect.frame_offset)
        assert got == 4, f"창이 4096이면 가운데가 2048 → 2048 ÷ 512 = 4여야 하는데 {got!r}. 숫자 2를 직접 썼나요?"
    finally:
        pesto_detect.WINDOW_SIZE = original


def check_blank3():
    for confidence, want in [(0.9, True), (0.1, False), (0.5, True), (0.49, False)]:
        got = _call(pesto_detect.is_voiced, confidence)
        assert got == want, f"is_voiced({confidence})가 {want}여야 하는데 {got!r}"


def check_all_together():
    audio, sr = sf.read(OUT_DIR / "sine_220.wav", dtype="float32")
    frames = _call(pesto_detect.track, audio, sr)
    expected = (len(audio) - 2048) // 512 + 1
    assert len(frames) == expected, f"프레임 수가 YIN과 같은 {expected}개여야 하는데 {len(frames)}개"
    voiced = [hz for _, hz, _ in frames if hz > 0]
    assert len(voiced) == len(frames), f"220Hz 사인은 전부 음정 있음이어야 하는데 {len(frames) - len(voiced)}프레임이 0"

    audio, sr = sf.read(OUT_DIR / "silence.wav", dtype="float32")
    frames = _call(pesto_detect.track, audio, sr)
    loud = [hz for _, hz, _ in frames if hz > 0]
    assert not loud, f"무음은 전부 0(음정 없음)이어야 하는데 {len(loud)}프레임이 음정 있음"


BLANKS = [
    ("빈칸 1", "프레임 간격 샘플 → ms",       check_blank1),
    ("빈칸 2", "YIN과 시각 맞추기",           check_blank2),
    ("빈칸 3", "확신도로 음정 있음/없음",     check_blank3),
    ("종합",   "테스트 신호로 한 번에 확인",   check_all_together),
]

if __name__ == "__main__":
    done, next_blank = 0, None
    for key, title, check in BLANKS:
        try:
            check()
            print(f"  ✅ {key}  {title}")
            done += 1
        except NotFilled:
            print(f"  ⬜ {key}  {title}")
            next_blank = next_blank or key
        except AssertionError as e:
            print(f"  ❌ {key}  {title}\n        → {e}")
            next_blank = next_blank or key
        except Exception as e:
            print(f"  ❌ {key}  {title}\n        → 에러: {type(e).__name__}: {e}")
            next_blank = next_blank or key

    print(f"\n  {done} / {len(BLANKS)} 완료", end="")
    print(f"   — 다음: {next_blank}" if next_blank else "   — 전부 완료! 이제  .venv\\Scripts\\python.exe compare_detectors.py")
