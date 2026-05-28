#!/usr/bin/env python3
"""
MIDI → ChimePlayer Song Converter

Converts a standard MIDI file (.mid) into a C++ header file (.h) that can be
included in the InnoX Boat firmware and played via ChimePlayer.

Usage:
    python tools/midi_to_chime.py input.mid                # writes to src/song.h
    python tools/midi_to_chime.py input.mid --tempo 140
    python tools/midi_to_chime.py input.mid --output my_song.h
"""

import argparse
import math
import struct
import sys

# 这个脚本不在 ESP32 上运行，而是在电脑上运行：
# 它读取 MIDI 文件，把音符事件转换成 ChimePlayer 能加载的 C++ song.h。

# ===========================================================================
# MIDI File Parsing
# ===========================================================================

class MidiFile:
    """Minimal MIDI file parser — handles format 0 and 1."""

    def __init__(self, path):
        # tracks 保存每条轨道的音符事件：(tick, 是否note_on, MIDI音高, 力度)。
        self.tracks = []        # list of lists of (tick, is_note_on, note, velocity)
        # tick_rate 是每个四分音符包含多少 tick；MIDI 文件头会覆盖默认值。
        self.tick_rate = 480    # ticks per quarter note
        # us_per_qn 是一个四分音符多少微秒；默认 120 BPM。
        self.us_per_qn = 500000  # default 120 BPM

        with open(path, "rb") as f:
            # 用二进制方式解析 MIDI 块结构。
            self._parse(f)

    def _read_varlen(self, f):
        # MIDI 的可变长度整数每个字节低 7 位存数据，最高位表示是否还有后续字节。
        value = 0
        while True:
            b = f.read(1)[0]
            value = (value << 7) | (b & 0x7F)
            if not (b & 0x80):
                return value

    def _parse(self, f):
        # --- Header ---
        # 标准 MIDI 文件必须以 MThd 头块开始。
        magic = f.read(4)
        if magic != b"MThd":
            raise ValueError("Not a valid MIDI file (no MThd)")

        chunk_len = struct.unpack(">I", f.read(4))[0]
        data = f.read(chunk_len)
        if len(data) < 6:
            raise ValueError("MIDI header too short")

        format_type = struct.unpack(">H", data[0:2])[0]
        num_tracks = struct.unpack(">H", data[2:4])[0]
        division = struct.unpack(">H", data[4:6])[0]

        if division & 0x8000:
            # 高位为 1 时表示 SMPTE 时间格式。
            fps = 256 - ((division >> 8) & 0xFF)
            subframes = division & 0xFF
            self.tick_rate = fps * subframes
        else:
            # 常见情况：division 直接表示每个四分音符的 tick 数。
            self.tick_rate = division

        print(f"  Format: {format_type}, Tracks: {num_tracks}, "
              f"Tick rate: {self.tick_rate}", file=sys.stderr)

        # --- Tracks ---
        for t in range(num_tracks):
            chunk_header = f.read(8)
            if len(chunk_header) < 8:
                break
            if chunk_header[:4] != b"MTrk":
                skip = struct.unpack(">I", chunk_header[4:8])[0]
                f.read(skip)
                continue

            track_len = struct.unpack(">I", chunk_header[4:8])[0]
            track_data = f.read(track_len)
            events = self._parse_track(track_data)

            if events:
                self.tracks.append(events)
                note_count = sum(1 for e in events if e[1])
                print(f"  Track {t}: {len(events)} events, "
                      f"{note_count} note_on", file=sys.stderr)
            else:
                print(f"  Track {t}: 0 events (skipped)", file=sys.stderr)

    def _parse_track(self, data):
        """Parse track events. Returns list of (tick, is_note_on, note, velocity)."""
        # events 只保留本项目关心的 Note On / Note Off 事件。
        events = []
        tick = 0
        i = 0
        running_status = 0x00

        while i < len(data):
            # --- Delta time ---
            delta = 0
            while i < len(data):
                b = data[i]
                delta = (delta << 7) | (b & 0x7F)
                i += 1
                if not (b & 0x80):
                    break
            tick += delta
            if i >= len(data):
                break

            # --- Status byte or running status ---
            status = data[i]
            if status & 0x80:
                running_status = status
                i += 1
            else:
                status = running_status

            if status == 0xFF:
                # Meta event
                # Meta event 包括速度、拍号、轨道名等；这里主要读取 tempo。
                if i >= len(data):
                    break
                meta_type = data[i]
                i += 1
                meta_len = 0
                while i < len(data):
                    b = data[i]
                    meta_len = (meta_len << 7) | (b & 0x7F)
                    i += 1
                    if not (b & 0x80):
                        break
                # Set tempo if it's a set tempo meta event
                if meta_type == 0x51 and meta_len >= 3:
                    if i + 2 < len(data):
                        self.us_per_qn = (data[i] << 16) | (data[i+1] << 8) | data[i+2]
                i += meta_len
                continue

            command = status >> 4

            if command == 0x9 or command == 0x8:  # Note On / Note Off
                if i + 1 >= len(data):
                    break
                note = data[i]
                velocity = data[i + 1]
                i += 2
                is_on = (command == 0x9 and velocity > 0)
                events.append((tick, is_on, note, velocity))

            elif command == 0xC or command == 0xD:  # Program Change / Ch Pressure
                i += 1
            elif command == 0xB or command == 0xE or command == 0xA:  # 2-byte events
                i += 2
            else:
                i += 1  # skip unknown

        return events


# ===========================================================================
# MIDI → Steps — quantized to NOTE_* constants from chime.h
# ===========================================================================

# NOTE_* constants defined in chime.h, indexed by MIDI note number.
# These cover MIDI notes 24-96 (C1 through B7).
# Notes outside that range are clamped to the nearest defined note.
_NOTE_NAMES = [None] * 128
_NOTE_FREQS = [0] * 128

# Octave 1 (MIDI 24-35)
_oct1 = ["C1","CS1","D1","DS1","E1","F1","FS1","G1","GS1","A1","AS1","B1"]
_oct1f = [33,35,37,39,41,44,46,49,52,55,58,62]
# Octave 2 (MIDI 36-47)
_oct2 = ["C2","CS2","D2","DS2","E2","F2","FS2","G2","GS2","A2","AS2","B2"]
_oct2f = [65,69,73,78,82,87,93,98,104,110,117,123]
# Octave 3 (MIDI 48-59)
_oct3 = ["C3","CS3","D3","DS3","E3","F3","FS3","G3","GS3","A3","AS3","B3"]
_oct3f = [131,139,147,156,165,175,185,196,208,220,233,247]
# Octave 4 (MIDI 60-71)
_oct4 = ["C4","CS4","D4","DS4","E4","F4","FS4","G4","GS4","A4","AS4","B4"]
_oct4f = [262,277,294,311,330,349,370,392,415,440,466,494]
# Octave 5 (MIDI 72-83)
_oct5 = ["C5","CS5","D5","DS5","E5","F5","FS5","G5","GS5","A5","AS5","B5"]
_oct5f = [523,554,587,622,659,698,740,784,831,880,932,988]
# Octave 6 (MIDI 84-95)
_oct6 = ["C6","CS6","D6","DS6","E6","F6","FS6","G6","GS6","A6","AS6","B6"]
_oct6f = [1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976]
# Octave 7 (MIDI 96-107)
_oct7 = ["C7","CS7","D7","DS7","E7","F7","FS7","G7","GS7","A7","AS7","B7"]
_oct7f = [2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951]

_all_octaves = list(zip(
    range(24, 108, 12),
    [_oct1, _oct2, _oct3, _oct4, _oct5, _oct6, _oct7],
    [_oct1f, _oct2f, _oct3f, _oct4f, _oct5f, _oct6f, _oct7f]
))

for start, names, freqs in _all_octaves:
    for i in range(12):
        idx = start + i
        _NOTE_NAMES[idx] = f"NOTE_{names[i]}"
        _NOTE_FREQS[idx] = freqs[i]


def midi_to_note_name(midi_note):
    """Quantize a MIDI note to the nearest defined NOTE_* constant name.
    Clamps to C1 (24) or B7 (107) if outside range.
    """
    # 项目只定义 C1-B7，超出范围时夹到最近可用音符。
    idx = max(24, min(107, midi_note))
    return _NOTE_NAMES[idx]


def midi_to_note_freq(midi_note):
    """Get the frequency of the nearest quantized NOTE_* constant."""
    idx = max(24, min(107, midi_note))
    return _NOTE_FREQS[idx]


def velocity_to_duty(velocity, max_duty=100):
    # MIDI velocity 是 1-127，这里映射到 PWM duty，默认最高只到 100，避免电机太猛。
    return max(1, min(max_duty, int(round(velocity / 127.0 * max_duty))))


def midi_to_steps(midi, tempo_bpm=120):
    """Convert parsed MIDI into a list of steps.
    Each step: (duration_ms, [(track_idx, freq, duty), ...])
    """
    # 先根据 BPM 算出每个 tick 对应多少微秒。
    us_per_qn = 60_000_000 // tempo_bpm
    us_per_tick = us_per_qn / midi.tick_rate

    # Build note-on → note-off pairs
    active = {}
    all_notes = []

    for track_idx, events in enumerate(midi.tracks):
        # active 记录已经 note_on 但还没 note_off 的音。
        for tick, is_on, note, velocity in events:
            key = (track_idx, note)
            if is_on:
                active[key] = (tick, velocity)
            else:
                if key in active:
                    start_tick, vel = active.pop(key)
                    duration = tick - start_tick
                    if duration < 1:
                        duration = 1
                    all_notes.append((track_idx, start_tick, note, vel, duration))

    # Any notes that never got a note_off — append with a default duration
    for key, (start_tick, vel) in active.items():
        track_idx, note = key
        all_notes.append((track_idx, start_tick, note, vel, 1))

    all_notes.sort(key=lambda x: x[1])

    if not all_notes:
        raise ValueError("No note events found in MIDI file")

    # Group simultaneous notes into steps
    GRID_TICKS = 10
    steps = []
    i = 0
    n = len(all_notes)

    while i < n:
        # 将起始 tick 非常接近的音符合并成同一个 ChimeStep，实现和弦/同时播放。
        step_start = all_notes[i][1]
        step_notes = []
        max_duration = 0
        while i < n and abs(all_notes[i][1] - step_start) <= GRID_TICKS:
            track, start, note, vel, dur = all_notes[i]
            name = midi_to_note_name(note)
            freq = midi_to_note_freq(note)
            duty = velocity_to_duty(vel)
            step_notes.append((track, name, freq, duty))
            if dur > max_duration:
                max_duration = dur
            i += 1

        duration_ms = int(round(max_duration * us_per_tick / 1000))
        if duration_ms < 20:
            duration_ms = 20

        steps.append((duration_ms, step_notes))

    return steps


# ===========================================================================
# C++ Generation
# ===========================================================================

def steps_to_cpp(steps, track_count, tempo_bpm=120):
    """Convert steps to C++ header file."""
    # 生成的 C++ 文件包含 SongStep 数组和一个 SongData 描述对象。
    lines = []
    lines.append("// Auto-generated from MIDI by tools/midi_to_chime.py")
    lines.append(f"// Original tempo: {tempo_bpm} BPM")
    lines.append(f"// Tracks with notes: {track_count}")
    lines.append(f"// Steps: {len(steps)}")
    lines.append("#ifndef SONG_H")
    lines.append("#define SONG_H")
    lines.append("")
    lines.append('#include "chime.h"')
    lines.append("")

    lines.append("static const SongStep _song_steps[] PROGMEM = {")
    for dur_ms, notes in steps:
        note_list = ", ".join(
            f"{{{track}, {name}, {duty}}}"
            for track, name, freq, duty in notes
        )
        lines.append(f"  {{{dur_ms}, {len(notes)}, {{{note_list}}}}},")
    lines.append("};")
    lines.append("")

    lines.append("static const SongData songData PROGMEM = {")
    lines.append(f"  {len(steps)},")
    lines.append(f"  {tempo_bpm},")
    lines.append(f"  {track_count},")
    lines.append(f"  _song_steps")
    lines.append("};")
    lines.append("")

    lines.append("#endif // SONG_H")
    lines.append("")

    return "\n".join(lines)


# ===========================================================================
# Main
# ===========================================================================

def main():
    # 解析命令行参数：输入 MIDI、可选 tempo、可选输出文件路径。
    parser = argparse.ArgumentParser(
        description="Convert MIDI file to ChimePlayer song header")
    parser.add_argument("input", help="Input .mid file")
    parser.add_argument("--tempo", type=int, default=0,
                        help="Override tempo in BPM (default: use MIDI tempo)")
    parser.add_argument("--output", default="src/song.h",
                        help="Output path (default: src/song.h)")
    args = parser.parse_args()

    print(f"Parsing {args.input} ...", file=sys.stderr)
    midi = MidiFile(args.input)

    if not midi.tracks:
        # 没有任何音符轨道时无法生成歌曲。
        print("Error: No tracks with note events found", file=sys.stderr)
        sys.exit(1)

    tempo = args.tempo if args.tempo > 0 else round(60_000_000 / midi.us_per_qn)
    print(f"Tempo: {tempo} BPM (from {'argument' if args.tempo > 0 else 'MIDI'})",
          file=sys.stderr)

    steps = midi_to_steps(midi, tempo)

    print(f"Generated {len(steps)} steps from "
          f"{len(midi.tracks)} track(s) with notes", file=sys.stderr)
    print()

    cpp = steps_to_cpp(steps, len(midi.tracks), tempo)

    with open(args.output, "w") as f:
        # 把生成的 C++ 头文件写到磁盘；默认是 src/song.h。
        f.write(cpp)
    print(f"Written to {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()


if __name__ == "__main__":
    main()
