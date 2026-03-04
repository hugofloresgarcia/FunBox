# inspo.md — Make the Guitar Not Sound Like a Guitar

A collection of open-source repos, algorithms, references, and ideas for building weird, creative effects on the Daisy platform. Organized by vibe.

---

## Mutable Instruments (The Gold Standard)

Emilie Gillet's open-source Eurorack firmware is the single best resource for creative embedded DSP. All MIT licensed, all C++, all running on ARM Cortex-M (STM32F4). The code is beautifully written and runs on comparable hardware.

**Repo**: [pichenettes/eurorack](https://github.com/pichenettes/eurorack)

| Module | What It Does | Why It's Interesting |
|--------|-------------|---------------------|
| **Clouds** | Granular processor | The granular guitar pedal. Records audio into a buffer and replays overlapping, pitch-shifted, time-stretched grains. Has a freeze mode. Four quality modes including a spectral (FFT) granular mode. |
| **Beads** | Granular processor (Clouds successor) | Improved granular engine with better grain scheduling, reverb tail, and attenuation of clicking artifacts. Studies in how to make granular sound musical. |
| **Rings** | Resonator / physical modeling | Turns any input into sympathetic string resonance, modal synthesis, or FM. Three models: modal, sympathetic strings, inharmonic strings. Makes anything sound like a plucked or bowed instrument. |
| **Elements** | Full physical modeling voice | Exciter (bow, blow, strike) + resonator. Self-contained physical modeling synthesis. |
| **Warps** | Crossfader / wavefolder / vocoder | Analog-style ring mod, diode crossfading, digital wavefolder, bitcrusher, and a vocoder. All the ways to mangle two signals together. |
| **Plaits** | Macro oscillator (16 models) | Virtual analog, FM, formants, speech, additive, wavetable, granular noise, Karplus-Strong, modal, particle noise. A masterclass in fitting diverse synthesis into a microcontroller. |
| **Braids** | Macro oscillator (legacy) | Earlier oscillator with different synthesis models. Naive oversampling approach (contrast with Plaits' band-limited approach). |
| **Stages** | Envelope / LFO / sequencer | Segment generator that can be patched into complex modulation shapes. Interesting for generative control. |
| **Tides** | Tidal modulator | Looping/one-shot AD/ASR envelope with waveshaping. Generates complex modulation signals. |
| **Marbles** | Random sampler | Clocked random voltage generator with control over the distribution of randomness. For generative/aleatoric control of parameters. |

**Start here**: `clouds/dsp/granular_processor.cc`, `rings/dsp/resonator.cc`, `plaits/dsp/engine/`

---

## Daisy Platform Projects

These run on the exact same hardware (Daisy Seed / Petal) and use libDaisy + DaisySP.

### [amcerbu/StrobeSpectralGranular](https://github.com/amcerbu/StrobeSpectralGranular)
Spectral processing effects for the Daisy 125b pedal. Includes:
- **Phase vocoder** with ARM-optimized FFT (N=2048 or 4096)
- **Pitch shifter** (frequency domain)
- **Spectral freeze** (in progress)
- **Granular synthesizer** with real-time grain parameter control
- Real-time frequency domain visualization on display

This is the closest thing to Clouds on the Daisy platform.

### [willbearfruits/GlitchPedal](https://github.com/willbearfruits/GlitchPedal)
8 real-time audio effects with FFT-based spectral processing and OLED display for Daisy Seed. Glitch-focused.

### [hirnlego/wreath](https://github.com/hirnlego/wreath)
A multifaceted looper for Daisy. MIT licensed, uses DaisySP/libDaisy. Good reference for buffer management and looper state machines.

### [bkshepherd/DaisySeedProjects](https://github.com/bkshepherd/DaisySeedProjects)
Multi-effect guitar pedal platform for Daisy Seed. Includes granular synthesis, looper, delay, and other effects with a menu system and OLED display. Good reference for building a multi-effect architecture.

### [guitarml/DaisyEffects](https://github.com/guitarml/DaisyEffects)
Collection of effects for Daisy Seed / Terrarium pedals by GuitarML. Clean, simple implementations.

### [GuitarML/NeuralSeed](https://github.com/GuitarML/NeuralSeed)
Neural amp/pedal modeling on Daisy Seed using **RTNeural** (already a submodule in this repo). Runs GRU networks for real-time amp emulation. 11 built-in models, fits in 128 KB flash. Relevant because RTNeural is already available in this project.

### [GuitarML/Seed](https://github.com/GuitarML/Seed)
More Daisy Seed pedal effects from GuitarML including amp modeling and utility effects.

### [mtthw-meyer/daisy-looper](https://github.com/mtthw-meyer/daisy-looper)
A looper for Daisy Seed written in Rust. Interesting as a different-language reference for the same hardware.

---

## Other Open-Source Eurorack / Embedded DSP

### [Befaco/Oneiroi](https://github.com/Befaco/Oneiroi)
Experimental digital synth for the OWL platform. Features Ambience, ChaosNoise, Clock, Compressor, and DelayLine DSP modules. Live performance oriented.

### [gritwave/eurorack](https://github.com/gritwave/eurorack)
Open-source Eurorack firmware with CMake build system. MIT licensed.

---

## Algorithm References

### [musicdsp.org](https://www.musicdsp.org/en/latest/)
The classic community archive of DSP algorithms. Organized by category:
- [Effects](https://www.musicdsp.org/en/latest/Effects/) — distortion, waveshaping, fold-back, delay, reverb, dynamics, phaser, stereo
- [Filters](https://www.musicdsp.org/en/latest/Filters/) — Moog VCF, biquad, RBJ Audio-EQ-Cookbook, resonant LP, all-pass
- [Synthesis](https://www.musicdsp.org/en/latest/Synthesis/) — oscillators, formants, physical modeling

### [DAFX: Digital Audio Effects (book)](https://dafx.de/DAFX_Book_Page/)
The standard textbook. Key chapters for this project:
- **Ch. 7 — Time-segment processing**: SOLA/PSOLA pitch shifting, granulation, time shuffling
- **Ch. 8 — Time-frequency processing**: Phase vocoder (FFT/IFFT), time stretching, pitch shifting, robotization, whisperization
- **Ch. 11 — Time and frequency warping**: Inharmonizer effects

### [Dattorro Plate Reverb](https://github.com/el-visio/dattorro-verb)
Clean C implementation of Jon Dattorro's classic plate reverb algorithm from the CCRMA paper. The reference implementation for plate reverb.

### [LucaSpanedda/Digital_Reverberation_in_Faust](https://github.com/LucaSpanedda/Digital_Reverberation_in_Faust)
Experiments with digital reverb algorithms in Faust. Good for understanding FDN and Schroeder structures before porting to C++.

### [Faust Libraries](https://faustlibraries.grame.fr/)
Faust's standard libraries are a great algorithm reference even if you don't use the language. `filters.lib`, `delays.lib`, `reverbs.lib`, and `misceffects.lib` contain well-documented DSP functions.

### Parasit Studio 0415 Guitar Synth
[parasitstudio.se/guitarsynth](https://www.parasitstudio.se/guitarsynth.html) — Analog guitar synth using CMOS logic (CD4069UBE) and Schmitt triggers to transform guitar into square waves across four octaves. Interesting as a reference for the pure analog approach to guitar-to-synth.

---

## Effect Ideas & Algorithms

### Granular Processing
Record input into a circular buffer. Play back overlapping "grains" (10-500ms windows) with independent pitch, position, and envelope. Parameters: grain size, density, pitch scatter, position jitter, spray (randomness). The Clouds source code is the best reference. Key challenge on Daisy: buffer lives in SDRAM, grain scheduling must be deterministic.

### Spectral Freeze
FFT the input, capture a single frame's magnitude spectrum, continuously resynthesize it with randomized phases. The sound hangs in the air forever. Combine with slow pitch drift for evolving pads. See StrobeSpectralGranular for a Daisy implementation.

### Shimmer Reverb
Reverb with pitch-shifted feedback. Each time the signal circulates through the reverb tail, it gets shifted up an octave (or a fifth). Creates cathedral-like ascending harmonics. Implementation: reverb → pitch shifter → feed back into reverb input. The pitch shifter can be a simple delay-line modulation trick (two crossfading read heads).

### Reverse Delay
Write into a buffer forward, read it backward. Crossfade at chunk boundaries to avoid clicks. Red Panda Tensor does this with intelligent splice-point detection (analyzing phrase, note, and waveform levels). Simpler version: fixed-size reverse chunks with Hann window crossfades.

### Stutter / Glitch
Randomly or rhythmically slice the input buffer into segments and repeat, reverse, pitch-shift, or rearrange them. Key parameters: slice length, repeat count, probability of glitch, pitch shift range. Can be beat-synced or free-running.

### Phase Vocoder Robotization
FFT the input, zero all phases (or set to random), IFFT. Removes pitch information while preserving spectral envelope. The voice/guitar becomes a robotic monotone. Whisperization: multiply magnitudes by random noise before IFFT.

### Karplus-Strong Synthesis
Plucked string synthesis from a noise burst fed into a filtered delay line. The delay length sets the pitch, the filter controls the decay character. Can be excited by the guitar input (use envelope follower to trigger noise bursts). Already in DaisySP.

### Wavefolding
Drive the signal through a nonlinear waveshaper that folds the waveform back on itself when it exceeds a threshold. Adds dense, bell-like harmonics. The Serge wavefolder circuit is the classic reference. DaisySP has `Wavefolder`. Warps has a digital implementation.

### Ring Modulation with Envelope Following
Multiply the guitar signal by a carrier oscillator. Use an envelope follower on the guitar to modulate the carrier frequency — pick hard and the ring mod sweeps up, play soft and it stays low. Creates inharmonic, metallic textures that respond to dynamics.

### Resonator Bank (Rings-style)
Process the guitar through a bank of tuned resonators (comb filters or biquad bandpass filters). Tune them to a chord. The guitar excites the resonators — any input becomes pitched. Modal synthesis. See Rings' `resonator.cc`.

### Formant Filter
Bank of bandpass filters tuned to vowel formant frequencies. Sweep between vowel shapes with a knob or envelope follower. Makes the guitar "talk". Two-formant is enough for a basic vowel sound (F1 + F2).

### Tape Speed Emulation
Variable-rate playback of a delay buffer. Slow it down → pitch drops, warp. Speed it up → chipmunk. Smoothly varying the rate creates wow and flutter. Keith Fullerton Whitman's *Playthroughs* used multiple delay taps at slightly different speeds (2.0, 1.98, 1.96, 1.94 seconds) to create slowly shifting enharmonic clouds.

### Neural Amp Modeling
Use RTNeural (already a submodule) to run a trained GRU network that emulates analog amps/pedals. NeuralSeed fits a GRU-10 model in real-time on the Daisy. Train your own models with PyTorch on recordings of any analog gear.

### Chaos / Feedback Networks
Cross-coupled delay lines with nonlinear feedback (wavefold, clip, ring mod in the feedback path). Small parameter changes create dramatically different sonic landscapes. The Befaco Oneiroi `ChaosNoise` module is a reference.

### Frequency Shifting (already in Cenote)
Hilbert transform for SSB modulation. Small shifts (1-10 Hz) create phaser-like motion. Larger shifts (50-200 Hz) create metallic, inharmonic textures. Very different from pitch shifting — it shifts all frequencies by a constant Hz, so harmonics become inharmonic.

---

## Artists / Pedals for Sonic Reference

These aren't code, but they define the sound palette:

- **Red Panda Tensor** — reverse, time-stretch, stutter, pitch warp in real-time
- **Red Panda Particle** — granular delay/pitch shifter
- **Chase Bliss Mood** — micro-looper + reverb/delay, random slice playback
- **EHX Synth9** — guitar-to-synth (the commercial version of what GuitarSynth does)
- **Meris Hedra** — 3-voice pitch shifter with independent intervals and rhythmic delays
- **Cooper FX Generation Loss** — VHS/tape degradation emulation (wow, flutter, noise, dropout, filtering)
- **Montreal Assembly Count to Five** — 3 play heads with independent direction/speed, granular-ish
- **Hologram Microcosm** — granular looping + effects (glitch, stutter, mosaic, blur)
- **OBNE Screen Violence** — modulated reverb with pitch-shifted feedback (shimmer)
- **Keith Fullerton Whitman's *Playthroughs*** — MDA Tracker (pitch-to-sine) → Karlette (4-tap delay cloud) → GRM Shuffler → spring reverb. The whole album is improvised guitar turned into shimmering drones.
- **Bananana Effects ABRACADABRA** — 8 shimmer reverb modes including error delay (forward + double-speed reverse + cross-feedback)

---

## DaisySP Modules Worth Exploring

Already available in the DaisySP library (no need to write from scratch):

| Module | Category | Notes |
|--------|----------|-------|
| `GranularPlayer` | Sampling | Grain-based playback engine |
| `Looper` | Sampling | Normal, Onetime Dub, Replace, Frippertronics modes |
| `PitchShifter` | Effects | Delay-based pitch shifting |
| `Wavefolder` | Effects | Serge-style wavefolding |
| `Decimate` | Effects | Bitcrusher + sample rate reduction |
| `Overdrive` | Effects | Soft-clipping distortion |
| `Phaser` | Effects | All-pass filter chain |
| `Autowah` | Effects | Envelope-controlled filter |
| `ResonatorSvf` | Filters | Resonant state-variable filter |
| `KarplusString` | Synthesis | Plucked string physical modeling |
| `ModalVoice` | Synthesis | Modal resonator synthesis |
| `StringVoice` | Synthesis | Karplus-Strong with extended controls |
| `AnalogBassDrum` / `AnalogSnareDrum` / `HiHat` | Drums | Analog drum synthesis models |
| `FractalNoise` / `Dust` / `Particle` | Noise | Interesting noise textures for granular excitation |
| `FormantOscillator` | Synthesis | Formant/vowel synthesis |

---

## What to Build Next (Ideas)

1. **Granular freeze pedal** — Record into SDRAM buffer, play back grains with pitch/time control, freeze button captures a moment and holds it
2. **Spectral resonator** — FFT input, multiply spectrum by a resonator bank tuned to a chord, IFFT. Guitar excites the chord no matter what notes you play
3. **Shimmer reverb** — FDN reverb with pitch-shifted (+12 semitones) feedback path, possibly using the Hilbert shifter from Cenote for a detuned variant
4. **Tape degradation** — Variable-speed buffer playback with wow/flutter LFO, noise injection, dropout simulation, lowpass filtering that responds to "tape speed"
5. **Glitch delay** — Buffer slicer that randomly repeats, reverses, or pitch-shifts segments of the delay buffer, with probability controls
6. **Neural + granular hybrid** — RTNeural amp model into granular processing. Analog amp warmth → alien grain textures
7. **Formant pedal** — Envelope follower drives formant filter bank. Guitar "speaks" vowels based on picking dynamics
8. **Drone machine** — Infinite hold (from Cenote) + resonator bank (from Rings). Hit a chord, freeze it, it resonates forever as a tuned drone
