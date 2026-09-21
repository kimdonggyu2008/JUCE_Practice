"""to_midi.py의 빈칸을 다 채운 뒤, 실제 테스트 신호로 돌려보는 스크립트.

    .venv\\Scripts\\python.exe demo_to_midi.py

하는 일:
  1. 신호마다 YIN → 단순 반올림 / 안정화 두 방법으로 노트를 만들어 개수를 비교한다.
  2. out/ 에 결과를 쓴다:
       xxx_naive.mid, xxx_stable.mid      — DAW에 끌어다 놓으면 피아노롤로 보인다
       xxx_naive_render.wav, xxx_stable_render.wav — 노트를 사인파로 다시 소리 낸 것
     원본 xxx.wav와 번갈아 들어보면 변환이 어땠는지 귀로 알 수 있다.
"""

import sys

import soundfile as sf

from signals import OUT_DIR
from to_midi import naive_notes, note_name, render, stable_notes, write_midi
from yin import track

sys.stdout.reconfigure(encoding="utf-8")


def summary(notes, limit=8):
    names = ", ".join(note_name(n.number) for n in notes[:limit])
    return f"{len(notes):3d}개  [{names}{' ...' if len(notes) > limit else ''}]"


if __name__ == "__main__":
    for name in ["sine_220", "harmonic_110", "vibrato_c4", "glide_110_440", "silence", "noise"]:
        audio, sr = sf.read(OUT_DIR / f"{name}.wav", dtype="float32")
        frames = track(audio, sr)

        naive = naive_notes(frames)
        stable = stable_notes(frames)

        print(f"■ {name}")
        print(f"    단순 반올림  {summary(naive)}")
        print(f"    안정화       {summary(stable)}")

        for label, notes in [("naive", naive), ("stable", stable)]:
            write_midi(notes, sr, OUT_DIR / f"{name}_{label}.mid")
            sf.write(OUT_DIR / f"{name}_{label}_render.wav", render(notes, sr, len(audio)), sr, subtype="FLOAT")

    print(f"\n결과 파일: {OUT_DIR}")
