# WaveCrush

A synth-destruction pedal for the FunBox platform inspired by the OP-1 Terminal effect. Polyphonic octave shifting feeds into a bitcrusher with a switchable anti-aliasing pre-filter that shapes the character of the destruction. A post-tone filter tames the output. Extremely synthy, zero reverb.

The octave engine uses the ERB-PS2 algorithm from [SubNUp](../SubNUp/) (80-band filter bank, polyphonic, handles chords). ~4 ms latency on the octave stage.

## Controls

```
 Bit Depth   | Pre Cutoff |  Pre Reso
 Sample Rate |    Tone    |    Mix

  Off / Sub / Up  |  HP / BP / LP  | Byp / On / Extreme
       SW1        |      SW2       |        SW3
```

### Knobs

| Knob | Parameter | Notes |
|------|-----------|-------|
| 1 | Bit Depth | Bit reduction amount. 0 = clean, full CW = crushed |
| 2 | Pre Cutoff | Anti-aliasing filter cutoff before the bitcrusher (200 Hz - 16 kHz). Lower = smoother, more controlled destruction |
| 3 | Pre Reso | Anti-aliasing filter resonance (0 - 0.95). Cranked = screaming harmonics that get quantized |
| 4 | Sample Rate | Downsample amount. 0 = clean, full CW = destroyed |
| 5 | Tone | Post-filter lowpass cutoff (200 Hz - 16 kHz). Tames harshness after the crusher |
| 6 | Mix | Dry/wet blend (constant-power crossfade) |

### Switches

| Switch | Left | Center | Right |
|--------|------|--------|-------|
| 1 (Octave) | Off | Sub octave (-1 oct, polyphonic) | Octave up (+1 oct, polyphonic) |
| 2 (Pre-Filter Model) | Highpass | Bandpass | Lowpass |
| 3 (Bitcrusher) | Bypass | On | On + extreme SR reduction |

Switch 2 selects the anti-aliasing filter type before the bitcrusher. Different models produce very different aliasing characters:
- **Lowpass** -- classic anti-aliasing, smooth and warm
- **Bandpass** -- resonant peak only, nasty focused crush on a narrow band
- **Highpass** -- removes bass before crushing, thin and crunchy

### Footswitches

| Footswitch | Function |
|------------|----------|
| FSW1 | Bypass toggle (LED 1 = active) |
| FSW2 | Octave up toggle (LED 2 = active) -- always adds +1 oct, stacks with SW1 |

FSW2 adds a polyphonic octave-up voice independently of Switch 1. When both FSW2 and SW1=Sub are active, you get sub octave and octave up summed together.

### DIP Switches

| DIP | Function |
|-----|----------|
| 1 | Mono (off) / MISO stereo (on) |
| 2 | Pre-filter position -- off = before bitcrusher (Terminal style), on = after bitcrusher |
| 3 | Noise gate -- off = disabled, on = smooth input gate (~-60 dB threshold) |
| 4 | Reserved |

## Signal Chain

```
in -> Octave -> Pre-Filter -> Bitcrusher -> Tone -> Dry/Wet Mix -> SoftClip -> out
       (SW1)   (K2/K3, SW2)  (K1/K4, SW3)  (K5)      (K6)

DIP 2 on: moves pre-filter after the bitcrusher
```

## Build

```bash
cd software/WaveCrush
make

# Flash via DFU
make program-dfu
```
