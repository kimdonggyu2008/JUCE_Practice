"""to_midi.py 빈칸 채점기.

    cd 03_pitch_to_midi\\prototype
    .venv\\Scripts\\python.exe practice\\check_to_midi.py

빈칸 1부터 순서대로. ⬜ = 아직 안 채움, ❌ = 채웠는데 틀림(무엇을 기대했는지 알려줌), ✅ = 통과.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))   # prototype 폴더의 to_midi를 불러오기 위해
sys.stdout.reconfigure(encoding="utf-8")

import numpy as np

try:
    import to_midi
    from to_midi import Note
except SyntaxError as e:
    # 문법 에러가 하나라도 있으면 파일 전체를 못 읽는다 — 채점 전에 그것부터 알려준다.
    print(f"  ⚠️ to_midi.py {e.lineno}번 줄: 파이썬 문법이 아닙니다 (파일 전체를 읽지 못해 채점을 못 함)")
    print(f"        {e.text.strip() if e.text else ''}")
    print(f"        {' ' * max((e.offset or 1) - 1 - (len(e.text) - len(e.text.lstrip()) if e.text else 0), 0)}^")
    sys.exit(1)


class NotFilled(Exception):
    pass


def _call(fn, *args, **kwargs):
    """빈칸(___)을 아직 안 채웠으면 NotFilled로 바꿔서 구분한다."""
    try:
        return fn(*args, **kwargs)
    except NameError as e:
        if "___" in str(e):
            raise NotFilled from None
        raise


def _frames(midi_values):
    """노트 번호(소수) 목록 → yin.track()과 같은 모양의 프레임 목록. None은 음정 없음."""
    return [(i * 512, 0.0 if m is None else to_midi.midi_to_hz(m), 1.0) for i, m in enumerate(midi_values)]


def _describe(notes):
    return "[" + ", ".join(f"{n.number}번 {n.start_frame}~{n.end_frame}" for n in notes) + "]"


def check_blank1():
    for hz, want in [(440, 69), (880, 81), (220, 57)]:
        got = _call(to_midi.hz_to_midi, hz)
        assert abs(got - want) < 1e-9, f"hz_to_midi({hz})이 {want}이어야 하는데 {got}"
    got = _call(to_midi.hz_to_midi, 261.63)
    assert abs(got - 60) < 0.01, f"hz_to_midi(261.63)이 약 60(C4)이어야 하는데 {got:.3f}"


def check_blank2():
    frames = _frames([57.08, 56.95, None, 60.02, 59.9])
    got = _call(to_midi.naive_notes, frames)
    want = [Note(0, 2, 57), Note(3, 5, 60)]
    assert got == want, (
        f"프레임 노트 번호 [57.08, 56.95, 없음, 60.02, 59.9]를 반올림하면 [57, 57, 없음, 60, 60] → "
        f"묶으면 {_describe(want)}이어야 하는데 {_describe(got)}.  "
        f"per_frame.append(...)에 정수 노트 번호가 들어가나요? (round로 반올림, int로 정수)"
    )


def check_blank3():
    # 60번 근처에서 ±0.6반음 흔들림 — limit 0.8 안이라 전부 60이어야 함
    wobble = [60 + 0.6 * np.sin(k) for k in range(12)]
    got = _call(to_midi.hysteresis_per_frame, _frames(wobble), 0.8)
    assert got == [60] * 12, (
        f"60번에서 ±0.6만 흔들리면(limit 0.8) 계속 60이어야 하는데 {got}.  "
        f"멀어졌을 때만 True가 되는 조건인가요?"
    )
    # 60 → 62로 실제로 옮겨감 — 1.4 > 0.8이라 바뀌어야 함
    got = _call(to_midi.hysteresis_per_frame, _frames([60, 60, 62, 62]), 0.8)
    assert got == [60, 60, 62, 62], (
        f"60에서 62로 옮겨가면(차이 2 > 0.8) 바뀌어야 하는데 {got}.  "
        f"조건이 항상 False인가요?"
    )


def check_blank4():
    # 60번 노트 한가운데 옥타브 오류(72) 한 프레임 — 버리고 이어 붙여 60 하나가 되어야 함
    frames = _frames([60] * 10 + [72] + [60] * 10)
    got = _call(to_midi.stable_notes, frames, min_frames=3)
    assert got == [Note(0, 21, 60)], (
        f"60 ×10, 72 ×1, 60 ×10 → 72(1프레임)는 버리고 60 하나(0~21)로 이어져야 하는데 {_describe(got)}.  "
        f"min_frames 이상인 것만 남기는 조건인가요?"
    )


BLANKS = [
    ("빈칸 1", "Hz → 노트 번호 공식",        check_blank1),
    ("빈칸 2", "프레임마다 반올림",          check_blank2),
    ("빈칸 3", "히스테리시스 조건",          check_blank3),
    ("빈칸 4", "짧은 노트 걸러내기",          check_blank4),
]

if __name__ == "__main__":
    done = 0
    next_blank = None
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
    print(f"   — 다음: {next_blank}" if next_blank else "   — 전부 완료! 이제  .venv\\Scripts\\python.exe demo_to_midi.py")
