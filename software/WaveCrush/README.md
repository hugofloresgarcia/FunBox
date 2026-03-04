# WaveCrush

A synth-destruction pedal for the FunBox platform. Three stages -- octave, ring modulator, and bitcrusher -- each independently switchable via the 3-way toggles. A resonant tone filter shapes the output. Extremely synthy, zero reverb.

## Controls

### Knobs

```
 Bit Depth  |  Cutoff  |  Resonance
Sample Rate | Ring Mod |    Mix
```

| Knob | Parameter | Notes |
|------|-----------|-------|
| 1 | Bit Depth | Bit reduction amount. 0 = clean, full CW = crushed |
| 2 | Cutoff | Tone filter cutoff (200 Hz - 16 kHz) |
| 3 | Resonance | Tone filter resonance (0 - 0.95). Cranked = screamy |
| 4 | Sample Rate | Downsample amount. 0 = clean, full CW = destroyed |
| 5 | Ring Mod Freq | Carrier oscillator frequency. Range set by DIP 3 |
| 6 | Mix | Dry/wet blend (constant-power crossfade) |

### Switches

| Switch | Left | Center | Right |
|--------|------|--------|-------|
| 1 (Octave) | Off | Sub octave (square, -1 oct) | Octave up (rectifier) |
| 2 (Ring Mod) | Bypass | Sine carrier | Square carrier |
| 3 (Bitcrusher) | Bypass | On | On + extreme SR reduction |

### Footswitches

| Footswitch | Function |
|------------|----------|
| FSW1 | Bypass toggle (LED 1 = active) |
| FSW2 | Momentary "chaos" -- maxes ring mod frequency while held (LED 2 = active) |

### DIP Switches

| DIP | Function |
|-----|----------|
| 1 | Mono (off) / MISO stereo (on) |
| 2 | Chain order -- off = Oct->RM->BC, on = BC->RM->Oct |
| 3 | Ring mod range -- off = low (20-1000 Hz), on = high (200-5000 Hz) |
| 4 | Tone position -- off = post (end of chain), on = pre (between stages) |

## Signal Chain

```
in -> Octave -> Ring Mod -> Bitcrusher -> Tone Filter -> Dry/Wet Mix -> SoftClip -> out
       (SW1)     (SW2)       (SW3)       (K2/K3)          (K6)

DIP 2 on: reverses stage order to BC -> RM -> Oct
DIP 4 on: moves tone filter between first and second stages
```

## Build

```bash
cd software/WaveCrush
make

# Flash via DFU
make program-dfu
```
