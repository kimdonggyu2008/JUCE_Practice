"""음정 곡선(프레임별 Hz) → MIDI 노트.

YIN은 프레임마다 Hz를 준다. MIDI는 "몇 번 노트가 언제 시작해서 언제 끝났나"다.
그 사이에 변환이 두 번 있다:
  ① Hz → 노트 번호        반음 칸으로 반올림. 공식 하나.
  ② 프레임 → 노트 구간    같은 노트가 이어지는 프레임을 하나로 묶기. 여기가 어렵다.

②가 어려운 이유: 목소리는 흔들리고(비브라토), 검출기는 가끔 튄다(옥타브 오류).
매 프레임 그냥 반올림하면 반올림 경계 근처에서 노트가 왔다 갔다 하며 잘게 쪼개진다.

────────────────────────────────────────────────────────────────────────────
실습: 이 파일에 빈칸(___)이 4개 있다. 빈칸 1부터 순서대로 채운다.
  - `___` 세 글자만 지우고 그 자리에 채운다. 줄의 나머지는 건드리지 않는다.
  - 채울 때마다 채점:  .venv\\Scripts\\python.exe practice\\check_to_midi.py
  - 4개 다 채우면:    .venv\\Scripts\\python.exe demo_to_midi.py   (실제 신호로 돌려보기)
────────────────────────────────────────────────────────────────────────────
"""

from dataclasses import dataclass

import numpy as np

from yin import HOP_SIZE, WINDOW_SIZE

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


# ════════════════════════════════════════════════════════════════════════════
# ① Hz → 노트 번호
# ════════════════════════════════════════════════════════════════════════════

def hz_to_midi(hz: float) -> float:
    """주파수 → MIDI 노트 번호(소수 그대로).

    규칙 두 개:
      - 440Hz가 69번(A4)이다.
      - 주파수가 2배가 되면(한 옥타브 위) 번호가 12 올라간다. (반음 12개 = 한 옥타브)

      Hz     440Hz의 몇 배?   log2(몇 배)   ×12    +69    결과
      ────   ─────────────   ──────────   ────   ────   ────
      440         1              0          0     69     69  (A4)
      880         2              1         12     69     81  (A5)
      220        0.5            -1        -12     69     57  (A3)

    그래서 공식은  69 + 12 × log2(hz ÷ 440)
    """
    # ┌─ 빈칸 1 ─────────────────────────────────────────────────────────────┐
    # │ "440Hz의 몇 배인가"를 계산하는 식을 넣는다. 위 표의 두 번째 열.     │
    # │ 예: hz가 880이면 이 자리는 2가 되어야 한다.                         │
    # └───────────────────────────────────────────────────────────────────────┘
    
    hz = hz / 440
    return 69 + 12 * np.log2(hz)


def midi_to_hz(note: float) -> float:
    """hz_to_midi의 반대. 69 → 440.0"""
    return 440.0 * 2 ** ((note - 69) / 12)


def note_name(note: int) -> str:
    """60 → C4, 69 → A4.  12로 나눈 나머지가 음 이름, 몫이 옥타브."""
    return f"{NOTE_NAMES[note % 12]}{note // 12 - 1}"


# ════════════════════════════════════════════════════════════════════════════
# ② 프레임 → 노트 구간
# ════════════════════════════════════════════════════════════════════════════

@dataclass
class Note:
    start_frame: int   # 이 노트의 첫 프레임
    end_frame: int     # 이 노트가 끝난 "다음" 프레임 (파이썬 슬라이스처럼 끝은 포함 안 함)
    number: int        # MIDI 노트 번호

    @property
    def num_frames(self) -> int:
        return self.end_frame - self.start_frame


def frame_time(frame_index: int, sample_rate: float) -> float:
    """프레임 번호 → 시각(초). 정답 규칙과 같게 창 가운데를 그 프레임의 시각으로 본다."""
    return (frame_index * HOP_SIZE + WINDOW_SIZE / 2) / sample_rate


def _group(per_frame: list) -> list[Note]:
    """프레임별 노트 번호 목록 → 같은 번호가 이어지는 구간을 Note로 묶는다.

    [57, 57, 57, None, 60, 60]  →  [Note(0, 3, 57), Note(4, 6, 60)]
    None(음정 없음)은 노트가 아니라 쉼이라 Note를 만들지 않는다.
    """
    notes = []
    current, start = None, 0
    for i, number in enumerate(per_frame):
        if number != current:                       # 번호가 바뀌는 순간
            if current is not None:
                notes.append(Note(start, i, current))   # 지금까지 이어진 구간을 하나의 노트로
            current, start = number, i
    if current is not None:
        notes.append(Note(start, len(per_frame), current))
    return notes


def naive_notes(frames: list[tuple[int, float, float]]) -> list[Note]:
    """가장 단순한 방법: 매 프레임을 가장 가까운 노트로 반올림하고, 이어지는 것끼리 묶는다.

    frames는 yin.track()의 결과: [(frame_start, hz, confidence), ...]
    """
    per_frame = []
    for _, hz, _ in frames:
        if hz > 0:
            # ┌─ 빈칸 2 ─────────────────────────────────────────────────────┐
            # │ hz를 노트 번호로 바꾸고, 가장 가까운 정수로 만든다.          │
            # │ 예: 221Hz → hz_to_midi → 57.08 → 57                         │
            # │ 쓸 것: hz_to_midi(...)  /  round(...) 로 반올림  /  int(...) │
            # └───────────────────────────────────────────────────────────────┘
            hz = hz_to_midi(hz)
            hz = round (hz)
            hz = int(hz)
            per_frame.append(hz)
        else:
            per_frame.append(None)                  # 음정 없음 → 쉼
    return _group(per_frame)


def hysteresis_per_frame(frames: list[tuple[int, float, float]], limit: float) -> list:
    """히스테리시스: "지금 노트에서 충분히 멀어졌을 때만" 노트를 바꾼다.

    반올림은 ±50 cents(0.5반음)가 경계다. 목소리가 60번 노트에서 ±60 cents 흔들리면
    단순 반올림은 59 → 60 → 61 → 60 → 59 ... 로 계속 바뀌어 노트가 잘게 쪼개진다.
    그래서 경계에 여유를 더 준다. limit은 그 경계(반음 단위).

      지금 노트 60, limit 0.8  →  60.8을 넘거나 59.2 아래로 가야 바꾼다
      61.3 → |61.3 - 60| = 1.3 > 0.8  → 바꾼다 (61)
      60.6 → |60.6 - 60| = 0.6 ≤ 0.8  → 그대로 60

    결과는 naive_notes 안의 per_frame과 같은 모양 — 프레임마다 노트 번호(없으면 None).
    """
    per_frame = []
    current = None                                  # 지금 울리고 있는 노트 번호 (없으면 None)
    for _, hz, _ in frames:
        if hz <= 0:
            current = None                          # 음정 없음 → 노트 끊김
        else:
            m = hz_to_midi(hz)                      # 소수 그대로 (예: 60.6)
            # ┌─ 빈칸 3 ─────────────────────────────────────────────────────┐
            # │ "지금 노트(current)에서 limit보다 멀어졌나?"를 쓴다.         │
            # │ 멀다 = 두 값의 차이의 크기(부호 무시)가 limit보다 큼          │
            # │ 예: m=61.3, current=60, limit=0.8 → True                    │
            # │     m=60.6, current=60, limit=0.8 → False                   │
            # │ 쓸 것: abs(...) 는 부호를 뗀 크기.  abs(-1.3) → 1.3          │
            # └───────────────────────────────────────────────────────────────

                
            if current is None or abs(m-current) > limit:
                current = int(round(m))             # 충분히 멀어졌을 때만 새 노트로
        per_frame.append(current)
    return per_frame


def stable_notes(frames: list[tuple[int, float, float]],
                 hysteresis_cents: float = 30.0,
                 min_frames: int = 3) -> list[Note]:
    """흔들림과 튀는 값에 강한 방법. 장치 두 개를 쓴다.

    ① 히스테리시스 — 위 hysteresis_per_frame. 경계 근처에서 왔다 갔다 하는 걸 막는다.
    ② 최소 길이   — min_frames보다 짧은 노트는 튄 값(옥타브 오류 등)으로 보고 버린다.
    """
    limit = (50 + hysteresis_cents) / 100           # 반음 단위. 30이면 0.8
    per_frame = hysteresis_per_frame(frames, limit)

    # ┌─ 빈칸 4 ─────────────────────────────────────────────────────────────┐
    # │ 묶은 노트들 중 "min_frames 프레임 이상인 것만" 남긴다.              │
    # │ n.num_frames 가 그 노트의 길이(프레임 수)다.                        │
    # │ 이 줄은 "목록에서 조건에 맞는 것만 골라내기" 문법이다:               │
    # │     [x for x in 목록 if 조건]                                        │
    # │ 예: [x for x in [1, 5, 2, 8] if x >= 3]  →  [5, 8]                  │
    # └───────────────────────────────────────────────────────────────────────┘
    notes = [n for n in _group(per_frame) if n.num_frames >= min_frames]

    # 버린 자리 양옆이 같은 노트면 하나로 이어 붙인다.
    # (60 노트 한가운데 옥타브 오류 한 프레임이 끼면, 그걸 버린 뒤 60 두 개를 다시 합친다)
    merged = []
    for note in notes:
        previous = merged[-1] if merged else None
        if previous and previous.number == note.number and note.start_frame - previous.end_frame < min_frames:
            previous.end_frame = note.end_frame
        else:
            merged.append(note)
    return merged


# ════════════════════════════════════════════════════════════════════════════
# 결과 내보내기 — 실습 대상 아님
# ════════════════════════════════════════════════════════════════════════════

def write_midi(notes: list[Note], sample_rate: float, path, velocity: int = 90) -> None:
    """Note 목록을 표준 MIDI 파일(.mid)로 쓴다. DAW에 끌어다 놓으면 피아노롤로 보인다."""
    import mido

    ticks_per_beat = 480
    tempo = mido.bpm2tempo(120)                     # 120 BPM → 1박 = 0.5초

    def to_ticks(seconds: float) -> int:
        return int(round(mido.second2tick(seconds, ticks_per_beat, tempo)))

    events = []
    for note in notes:
        events.append((frame_time(note.start_frame, sample_rate), "note_on", note.number))
        events.append((frame_time(note.end_frame, sample_rate), "note_off", note.number))
    events.sort(key=lambda e: (e[0], e[1] == "note_on"))   # 같은 시각이면 note_off 먼저

    track = mido.MidiTrack()
    track.append(mido.MetaMessage("set_tempo", tempo=tempo, time=0))
    last_tick = 0
    for seconds, kind, number in events:
        tick = to_ticks(seconds)
        track.append(mido.Message(kind, note=number, velocity=velocity if kind == "note_on" else 0,
                                  time=tick - last_tick))  # MIDI 파일의 시간은 "직전 이벤트로부터 몇 틱"
        last_tick = tick

    midi = mido.MidiFile(ticks_per_beat=ticks_per_beat)
    midi.tracks.append(track)
    midi.save(path)


def render(notes: list[Note], sample_rate: float, num_samples: int) -> np.ndarray:
    """Note 목록을 사인파로 다시 소리 내기 — 원본과 나란히 들어보며 변환이 맞았는지 귀로 확인한다."""
    out = np.zeros(num_samples)
    fade = int(0.005 * sample_rate)                 # 5ms 페이드 — 없으면 노트 경계에서 '틱' 소리
    for note in notes:
        start = int(frame_time(note.start_frame, sample_rate) * sample_rate)
        end = min(int(frame_time(note.end_frame, sample_rate) * sample_rate), num_samples)
        if end - start <= 2 * fade:
            continue
        t = np.arange(end - start) / sample_rate
        tone = 0.3 * np.sin(2 * np.pi * midi_to_hz(note.number) * t)
        tone[:fade] *= np.linspace(0, 1, fade)
        tone[-fade:] *= np.linspace(1, 0, fade)
        out[start:end] += tone
    return out
