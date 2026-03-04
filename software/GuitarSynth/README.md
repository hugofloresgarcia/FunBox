# GuitarSynth

A polyphonic guitar-to-synthesizer pedal for FunBox. Detects up to 6 simultaneous pitches from your guitar using STFT-based harmonic analysis, then generates clean synth oscillator tones at those pitches -- inspired by the EHX Synth9.

## Signal Flow

```
Guitar In --> STFT Pitch Detector (N=2048, hop=128) --> Voice Tracker (6 voices)
    |                                                          |
    |         +-- Osc 1 --+                                    |
    |         +-- Osc 2 --+                                    |
    |         +-- Osc 3 --+---> Sum --> SVF Filter --> Synth signal
    |         +-- Osc 4 --+         ^
    |         +-- Osc 5 --+         |
    |         +-- Osc 6 --+    Envelope Follower
    |                                    ^
    +------------------------------------+---> Dry/Wet Mix --> Soft Clip --> Output
```

## How It Works

Every 128 samples (~2.67 ms), the pedal:
1. Computes a 2048-point FFT of the guitar input
2. Builds a harmonic salience function (sums energy at f0, 2f0, 3f0, ... 10f0)
3. Iteratively picks the strongest fundamental, subtracts its harmonics, and repeats for up to 6 voices
4. Tracks voices across frames so oscillators transition smoothly
5. Drives 6 PolyBLEP oscillators at the detected pitches
6. Filters and mixes the synth output with the dry guitar signal

## Controls

### Knobs

| Knob | Parameter | Range |
|------|-----------|-------|
| 1 | Waveform | (reserved for future morph control) |
| 2 | Cutoff | 20 Hz - 18 kHz filter cutoff (log) |
| 3 | Resonance | 0 - 0.9 |
| 4 | Envelope Depth | How much picking dynamics sweep the filter |
| 5 | Synth Level | Output level of the synth voices |
| 6 | Mix | Dry/wet blend |

### 3-Way Switches

| Switch | Left | Center | Right |
|--------|------|--------|-------|
| 1 | Saw wave | Square wave | Sine wave |
| 2 | Lowpass | Bandpass | Highpass |
| 3 | -1 Octave | Unison | +1 Octave |

### DIP Switches

| DIP | Off | On |
|-----|-----|-----|
| 1 | Fast envelope (1ms attack) | Slow envelope (20ms attack) |
| 2 | Normal polarity (filter opens with pick) | Inverted (filter closes with pick) |
| 3 | No portamento | Portamento (~80ms glide between notes) |
| 4 | Clean filter | Extra filter drive (growlier) |

### Footswitches

| Footswitch | Tap | Hold |
|------------|-----|------|
| 1 (Left) | Toggle bypass | Momentary engage |
| 2 (Right) | Toggle freeze (holds detected pitches) | Momentary freeze |

## Build

```
cd software/GuitarSynth
make
make program-dfu
```

Requires libDaisy and DaisySP built in the expected locations. CMSIS-DSP transform functions are compiled as part of this project for FFT support.
