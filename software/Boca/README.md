# Boca

Envelope filter / auto-wah with LFO modulation and a formant filter mode for the FunBox platform. Inspired by the Electro-Harmonix Q-Tron. "Boca" means "mouth" in Spanish.

## Modes

**Standard mode** (Switch 2: left or center) — Classic envelope filter. A state-variable filter (LP/BP/HP) whose cutoff frequency is driven by picking dynamics and/or an LFO. Switch 2 selects sweep direction: left = down, center = up.

**Formant mode** (Switch 2: right) — Three parallel bandpass filters tuned to vowel formant frequencies (F1/F2/F3). The modulation signal morphs between two vowel shapes, making the guitar "speak." Switch 1 selects the vowel pair.

## Modulation

The envelope follower and LFO are summed additively:

- **Sensitivity > 0, LFO off** — Pure envelope filter (Q-Tron style)
- **Sensitivity = 0, LFO on** — Rhythmic cyclic auto-wah
- **Both active** — Envelope opens the filter on pick attack, LFO adds a wobble on top

## Controls

```
 Sensitivity |   Range    | Resonance
  LFO Rate   | LFO Depth  |    Mix

  LP / BP / HP   | Down / Up / Fmt  | Fast / Med / Slow
      SW1        |       SW2        |       SW3
```

### Knobs

| Knob | Parameter | Notes |
|------|-----------|-------|
| 1 | Sensitivity | Envelope follower gain. 0 = no envelope contribution |
| 2 | Range | Base frequency and sweep extent |
| 3 | Resonance | Filter Q (standard) or formant bandwidth (formant) |
| 4 | LFO Rate | 0 = off, clockwise to ~15 Hz |
| 5 | LFO Depth | How much LFO modulates the filter |
| 6 | Mix | Dry/wet blend (asymmetric crossfade) |

### Switches

| Switch | Left | Center | Right |
|--------|------|--------|-------|
| 1 | LP / ah-ee | BP / oh-ee | HP / oo-ah |
| 2 | Env Down | Env Up | Formant |
| 3 | Fast | Medium | Slow |

Switch 1 meaning depends on Switch 2: in standard mode it selects filter type (LP/BP/HP), in formant mode it selects vowel pair.

Switch 3 selects envelope response speed:
- **Fast** — 1ms attack, 30ms decay (snappy funk)
- **Medium** — 5ms attack, 100ms decay (classic Q-Tron feel)
- **Slow** — 20ms attack, 300ms decay (gentle swells)

### Footswitches

| Footswitch | Function |
|------------|----------|
| FSW1 | Bypass (toggle + momentary hold) |
| FSW2 | Freeze (holds filter at current position) |

### DIP Switches

| DIP | Function |
|-----|----------|
| 1 | Drive — soft saturation before the filter |
| 2 | Bass recovery — mixes lowpassed dry signal back in |
| 3 | LFO waveform — off = sine, on = triangle |
| 4 | Reserved |

## Formant Vowel Pairs

| Switch 1 | Start | End | Character |
|----------|-------|-----|-----------|
| Left | /a/ (ah) | /i/ (ee) | Open to bright — "yah" |
| Center | /o/ (oh) | /i/ (ee) | Round to bright — "woy" |
| Right | /u/ (oo) | /a/ (ah) | Dark to open — "wah" |

## Build

```bash
# Hardware
cd software/Boca
make

# Flash via DFU
make program-dfu

# Sim (from scratch/ or software/sim/)
cmake -B build && cmake --build build
./build/sim_boca --duration 2.0 --freq 220 --knob1 0.7 --knob3 0.6 --knob6 1.0 --fsw1 --output boca_out.wav
```

## Signal Chain

```
in → [drive?] → envelope follower ─┐
                                    ├→ mod signal (0-1) → filter cutoff / formant morph
                    LFO ────────────┘
     [drive?] → SVF or 3x formant SVFs → [bass recovery?] → dry/wet mix → SoftClip → out
```
