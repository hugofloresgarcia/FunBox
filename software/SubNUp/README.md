# SubNUp

Polyphonic octave pedal for the FunBox platform. Clone of the TC Electronic Sub'N'Up using the ERB-PS2 algorithm from [terrarium-poly-octave](https://github.com/schult/terrarium-poly-octave) by Steve Schulteis (MIT license). The algorithm is based on Etienne Thuillier's thesis "Real-Time Polyphonic Octave Doubling for the Guitar" (Aalto University, 2016, done in cooperation with TC Electronic).

Three octave voices -- one octave up, one octave down, two octaves down -- with independent level controls, tone shaping, and sub character switching. ~4 ms latency, truly polyphonic (handles chords).

## How It Works

80 complex bandpass filters isolate individual frequency bands from the input (decimated to 8 kHz). Each filter output's phase is scaled to shift the frequency by an octave ratio -- doubling the phase rate for octave up, halving it for octave down. All filter outputs are summed per shift direction and interpolated back to 48 kHz.

No FFT, no delay buffers. All state fits in ~8 KB of SRAM.

## Controls

### Knobs

| Knob | Parameter | Notes |
|------|-----------|-------|
| 1 | Dry | Dry signal level |
| 2 | Up | Octave up (+1) level |
| 3 | Sub | Octave down (-1) level |
| 4 | Sub2 | Two octaves down (-2) level |
| 5 | Tone | Lowpass on octave signals (500 Hz - 8 kHz) |
| 6 | Output | Master output level (slight boost available) |

### Switches

| Switch | Left | Center | Right |
|--------|------|--------|-------|
| 1 | All voices | Up + Sub | Sub + Sub2 |
| 2 | Deep (low shelf boost) | Normal | Tight (HP 80 Hz on subs) |
| 3 | Reserved | Reserved | Reserved |

Switch 1 provides quick voice presets by muting disabled voices. All voices are still controlled by their individual knobs when enabled.

Switch 2 shapes the sub-octave character:
- **Deep** -- low shelf boost on the sub signals for a fatter tone
- **Normal** -- flat, no additional filtering
- **Tight** -- highpass at 80 Hz on sub signals to reduce mud

### Footswitches

| Footswitch | Function |
|------------|----------|
| FSW1 | Bypass (toggle, smooth 20 ms fade) |
| FSW2 | Momentary sub boost (hold > 300 ms: max sub/sub2, kill dry) |

### DIP Switches

| DIP | Function |
|-----|----------|
| 1 | Noise gate on/off |
| 2 | Trails on bypass (wet signal continues after bypass) |
| 3 | Reserved |
| 4 | Reserved |

## Build

```bash
# Hardware
cd software/SubNUp
make

# Flash via DFU
make program-dfu

# Sim (from software/sim/)
cmake -B build && cmake --build build
./build/sim_subnup --input guitar.wav --output subnup_out.wav \
    --knob1 0.5 --knob2 0.7 --knob3 0.7 --knob4 0.5 --knob5 0.8 --knob6 0.7 --fsw1
```

## Signal Chain

```
in (48 kHz) → [noise gate?] → decimate (48k → 8k)
                                    |
                                    → 80 complex bandpass filters (ERB-PS2)
                                    |   ├── up1  × up_level
                                    |   ├── down1 × sub_level  → [sub character filter]
                                    |   └── down2 × sub2_level → [sub character filter]
                                    |
                                    → interpolate (8k → 48k) → shelving EQ → tone LP
                                    |
dry × dry_level ────────────────────┘
                                    |
                                    → sum → output gain → SoftClip → stereo out
```

## Algorithm Details

The ERB-PS2 algorithm works by:

1. **Decimation** -- 2-stage polyphase FIR (48 kHz → 16 kHz → 8 kHz). Only frequencies below ~2 kHz are processed, matching the real Sub'N'Up.
2. **Filter bank** -- 80 complex bandpass filters spaced on an exponential curve: `f(n) = 480 * 2^(0.027n) - 420` Hz. Each produces a complex analytic signal.
3. **Phase scaling** -- Per-band octave shifting without FFT:
   - Octave up: square the unit phasor (double the phase rate)
   - Octave down: square root of unit phasor with zero-crossing sign tracking
   - Two octaves down: apply octave-down to the octave-down result
4. **Summation** -- All 80 band outputs are summed per shift direction.
5. **Interpolation** -- 2-stage polyphase FIR (8 kHz → 16 kHz → 48 kHz).
6. **EQ compensation** -- High shelf -11 dB @ 140 Hz + low shelf +5 dB @ 160 Hz to flatten the filter bank response.

## Credits

- Algorithm: ERB-PS2 from Etienne Thuillier's thesis (Aalto University / TC Electronic, 2016)
- Implementation reference: [terrarium-poly-octave](https://github.com/schult/terrarium-poly-octave) by Steve Schulteis (MIT license)
- Ported to FunBox with all external dependencies removed (no cycfi::q, gcem, or infra)
