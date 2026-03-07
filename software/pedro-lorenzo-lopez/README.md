# pedro-lorenzo-lopez (PLL)

Monophonic PLL synthesizer pedal for the FunBox platform. Inspired by the EQD Data Corrupter -- a three-voice guitar synth built around pitch tracking, interval multiplication, and subharmonic division. "PLL" stands for Phase-Locked Loop, but also for pedro-lorenzo-lopez.

The input signal is pitch-tracked via zero-crossings and used to drive three voices: a square-wave fuzz (hard-clipped input), a master oscillator (tuned to a selectable interval above the root), and a subharmonic oscillator (tuned to a selectable interval below). All three are mixed together with independent level controls.

## Improvements Over the Data Corrupter

- **Selectable waveforms** -- Square, saw, triangle, or sine (vs. square-only analog)
- **Just-intonation intervals** -- More musical than integer PLL divisions
- **Envelope follower** -- Synth voices can respond to pick dynamics
- **Freeze/drone mode** -- Hold a pitch and play over it
- **Clean blend** -- Mix dry guitar signal with the synth voices
- **Smoother glide** -- Software portamento is more controllable than analog loop filter

## Controls

### Knobs

| Knob | Parameter | Notes |
|------|-----------|-------|
| 1 | Mod Rate | Glide speed (Switch 2 left) or vibrato rate (Switch 2 right) |
| 2 | Master Interval | Quantized to 8 intervals -- turn to select |
| 3 | Sub Interval | Quantized to 8 intervals -- turn to select |
| 4 | Square Level | Volume of the square-wave fuzz voice |
| 5 | Master Level | Volume of the master oscillator voice |
| 6 | Sub Level | Volume of the subharmonic voice |

### Switches

| Switch | Left | Center | Right |
|--------|------|--------|-------|
| 1 | Root -2 oct | Root Unison | Root -1 oct |
| 2 | Glide | Mod Off | Vibrato |
| 3 | Sub from Unison | Sub Off | Sub from Oscillator |

Switch 1 shifts the root frequency fed into the PLL. Lower roots help tracking on higher frets and extend the range of the master oscillator downward.

Switch 3 selects where the subharmonic voice gets its root:
- **Unison** -- divides from the detected input frequency (more stable)
- **Oscillator** -- divides from the master oscillator output (wilder, frequency modulator applies to sub too)

### Footswitches

| Footswitch | Function |
|------------|----------|
| FSW1 | Bypass (tap = toggle, hold > 300ms = momentary) |
| FSW2 | Freeze (tap = toggle drone, hold = momentary drone) |

When freeze is active, the synth voices hold the last detected pitch regardless of what you play. LED 2 lights up during freeze.

### DIP Switches

| DIP | Function |
|-----|----------|
| 1-2 | Waveform select: 00 = Square, 01 = Saw, 10 = Triangle, 11 = Sine |
| 3 | Envelope follower on/off (synth voices track pick dynamics) |
| 4 | Clean blend on/off (mixes dry guitar into output) |

## Interval Tables

**Master Oscillator (8 intervals from root, going up):**

| Position | Interval | Ratio |
|----------|----------|-------|
| 0 | Unison | x1 |
| 1 | Minor 3rd | x6/5 |
| 2 | Major 3rd | x5/4 |
| 3 | Perfect 4th | x4/3 |
| 4 | Perfect 5th | x3/2 |
| 5 | Octave | x2 |
| 6 | Octave + 5th | x3 |
| 7 | 2 Octaves | x4 |

**Subharmonic (8 intervals, going down):**

| Position | Interval | Ratio |
|----------|----------|-------|
| 0 | -1 Octave | x1/2 |
| 1 | -1 Oct + 5th | x1/3 |
| 2 | -2 Octaves | x1/4 |
| 3 | -2 Oct + Maj 3rd | x1/5 |
| 4 | -2 Oct + 5th | x1/6 |
| 5 | -3 Octaves | x1/8 |
| 6 | -3 Oct + Maj 3rd | x1/10 |
| 7 | -3 Oct + 5th | x1/12 |

## How It Works

1. **Pitch detection** -- The input is hard-clipped and zero-crossings are measured. A median-of-3 filter rejects octave glitches. Valid range: 30 Hz to 1500 Hz.
2. **Root selection** -- The detected frequency is optionally shifted down 1 or 2 octaves (Switch 1) to bring it into range or access lower harmonics.
3. **Master oscillator** -- A polyBLEP oscillator locked to `root * interval_ratio`. Frequency modulation (glide or vibrato) is applied based on Switch 2.
4. **Subharmonic** -- A second polyBLEP oscillator at `source * sub_ratio`, where source is either the input frequency (Unison) or the master oscillator frequency (Oscillator mode).
5. **Square fuzz** -- The hard-clipped input signal itself, used directly as a voice.
6. **Mixing** -- All three voices are summed with individual level knobs, optionally modulated by an envelope follower, DC-blocked, and soft-clipped.

## Signal Chain

```
in → hard clip → zero-crossing pitch tracker → detected_freq
                       |
                       → root_freq (× root_multiplier from Switch 1)
                       |
                       ├→ × master_interval → [glide/vibrato] → Master Osc (polyBLEP)
                       |                                              |
                       ├→ × sub_interval ──────────────────────→ Sub Osc (polyBLEP)
                       |   (from root or master, per Switch 3)
                       |
     hard clip output → Square Fuzz voice
                       |
                       ├→ [envelope follower?] → voice levels → mixer → DC block → SoftClip → out
                       |
     dry input ────────┘ (clean blend, DIP 4)
```

## Build

```bash
cd software/pedro-lorenzo-lopez
make

# Flash via DFU
make program-dfu
```

## Tracking Tips

- Place early in signal chain, before delay/reverb/modulation
- Neck pickup tracks best
- Single notes with clean picking track quickly; chords produce chaos (by design)
- Use Root switch to bring high notes into tracking range
- Freeze mode lets you set a drone pitch and then play freely over it
