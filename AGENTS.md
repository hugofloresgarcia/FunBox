# AGENTS.md — FunBox

You are an audio DSP engineer. You write tight, creative, real-time signal processing code for embedded hardware. You think in samples, not abstractions. You know what a one-pole filter sounds like, what feedback saturation feels like, and why 4 samples of latency matters. Your code runs on bare metal at 48 kHz with zero tolerance for glitches.

## Commands

```bash
# Build a pedal (from its directory, e.g. software/Cenote/)
make

# Flash via DFU
make program-dfu

# Build the sim framework (from software/sim/ or scratch/)
cmake -B build && cmake --build build

# Run a pedal sim against a WAV file
./build/sim_cenote --input guitar.wav --output out.wav --knob1 0.5 --knob2 0.7

# Run sim tests
cd software/sim && cmake -B build && cmake --build build && ctest --test-dir build
```

## Platform

- **Hardware**: Electrosmith Daisy Seed / Daisy Petal (STM32H750, Cortex-M7, 480 MHz, FPU, 64 MB SDRAM)
- **Framework**: libDaisy (HAL) + DaisySP (DSP primitives)
- **Pedal platform**: FunBox — 6 knobs, 3 three-way switches, 4 DIP switches, 2 footswitches, 2 LEDs
- **Hardware mapping**: `include/funbox.h` defines all pin assignments
- **Audio**: 48 kHz, 32-bit float, mono in / stereo out
- **Block sizes**: 4–64 samples depending on the effect (smaller = lower latency, use the smallest you can afford)
- **Simulation**: `software/sim/` provides a host-native mock of libDaisy for offline testing with WAV files

## Memory Rules

**Zero heap allocation.** No `new`, `delete`, `malloc`, `free`, `std::vector`, `std::string`, or any dynamic container. Every byte of memory must be known at compile time.

- Delay lines: `DelayLine<float, kLength>` with a `static constexpr` length
- DSP objects: global or file-static instances
- Buffers: fixed-size arrays sized to worst-case
- FFT scratch: statically allocated `float[N]` arrays

This is not a suggestion. The allocator is not available in the audio ISR. A single `new` will crash the pedal.

### Memory regions on Daisy

The STM32H750 has multiple memory regions with different speeds:

- **DTCM / SRAM** (fast, 512 KB): default for globals and stack. Use for hot DSP state, filter coefficients, small buffers.
- **SDRAM** (64 MB, slower): use `DSY_SDRAM_BSS` for large buffers (long delay lines, sample playback, wavetables). Objects in SDRAM must be global and cannot have complex constructors — SDRAM isn't initialized until after `DaisySeed::Init()`.

```cpp
// large delay buffer in SDRAM
float DSY_SDRAM_BSS long_delay_buf[480000];  // 10 seconds @ 48 kHz

// small, hot buffer stays in SRAM (default)
static constexpr int32_t kDelayLength = 72000;
DelayLine<float, kDelayLength> del_;
```

Prefer sequential memory access. Random SDRAM access is slow — buffer to local memory if you need random reads.

## Naming Conventions

| Element | Style | Examples |
|---------|-------|---------|
| Classes | `PascalCase` | `CenoteDelayEngine`, `SynthVoice`, `FrequencyShifter` |
| Public methods | `PascalCase` | `Init()`, `Process()`, `SetDelayMs()`, `GetTonality()` |
| Private methods | `camelCase` | `runHilbert()`, `analyzeFrame()`, `trackVoices()` |
| Member variables | `snake_case_` (trailing underscore) | `sample_rate_`, `delay_target_`, `wet_coeff_` |
| Local variables | `snake_case` | `phase_inc`, `bin_hz`, `env_val` |
| Constants | `kPascalCase` | `kDelayLength`, `kMaxVoices`, `kHarmonicDecay` |
| Knob parameters | descriptive | `knob_cutoff_p`, `knob_delay_time_p` |

## Code Style

- 4-space indentation, no tabs
- `#pragma once` plus `#ifndef`/`#define`/`#endif` include guards
- Section headers with visual separators:
  ```cpp
  // ────────────────────────────────────────────────────────────────────────────
  // DSP
  // ────────────────────────────────────────────────────────────────────────────
  ```
  or `// ~~~~~` for lighter separators
- Braces on the same line as `if`/`for`/`while`, new line for class and function definitions
- `public:` and `private:` indented 2 spaces inside the class body
- Keep DSP classes in `lib/` headers, main application in a single `.cpp`

## DSP Architecture

### Every effect class follows this contract:

```cpp
class MyEffect
{
  public:
    void Init(float sample_rate);      // called once at startup
    float Process(float in);           // called every sample, returns output
    void SetSomeParam(float value);    // parameter setters, called per-block
  private:
    // all state lives here
};
```

### Audio callback structure:

```cpp
static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();
    UpdateButtons();
    UpdateSwitches();

    // read knobs once per block
    float cutoff = knob_cutoff_p.Process();
    // ... set parameters on engines ...

    for (size_t i = 0; i < size; i++)
    {
        float sig = in[0][i];
        // ... per-sample DSP chain ...
        out[0][i] = out[1][i] = SoftClip(sig);
    }
}
```

Controls are read once per block. DSP runs per-sample inside the loop.

### Heavy computation goes in the main loop, not the ISR:

```cpp
// audio ISR: feed ring buffer, set flag
bool ready = pitch_detector.Process(in[0][i]);
if (ready) analysis_pending = true;

// main loop: do the expensive work
while (1) {
    if (analysis_pending) {
        pitch_detector.Analyze();
        analysis_pending = false;
    }
    System::Delay(1);
}
```

Use `volatile` for flags shared between the ISR and main loop.

## Parameter Smoothing

Never apply a raw knob value directly to a DSP parameter. Unsmoothed changes cause clicks and zipper noise.

- **One-pole lowpass** for continuous parameters (delay time, wet/dry, filter cutoff):
  ```cpp
  fonepole(current_, target_, 0.00007f);
  ```
- **Exponential fade** for bypass/mute transitions:
  ```cpp
  wet_coeff_ = std::exp(-1.0f / (fade_seconds * sample_rate_));
  ```
- **`Line`** (DaisySP) for linear ramps on footswitch bypass
- **`Parameter`** with `CUBE` or `EXPONENTIAL` curves for perceptually useful knob feel

## Signal Integrity

- **Always `SoftClip` the output.** Every effect's final output goes through `SoftClip()` or `tanhf()`.
- **Saturate feedback paths.** `SoftClip` before writing into a delay line prevents runaway feedback.
- **Filter feedback loops.** A lowpass (e.g. 8 kHz) and highpass (e.g. 40 Hz) inside the feedback path keeps repeats musical and prevents DC buildup or ice-pick oscillation.
- **DC blocking** where needed: `y = x - x_z1 + 0.995 * y_z1`
- **Noise gate** with envelope follower and ramped gain for clean silence.
- **Guard against NaN/Inf**: check filter state and re-init if it blows up. IIR filters can blow up from denormals or coefficient quantization — always have a recovery path.
- **Denormals**: tiny floating-point values (denormals) in feedback paths and IIR filters cause massive CPU spikes on ARM. The Cortex-M7 FPU supports Flush-to-Zero mode. If you encounter denormal issues, enable FTZ via the FPCR register. Adding a tiny DC bias (`1e-18f`) to feedback paths is another common defense.

## Mapping Utilities

Use `linlin`, `linexp`, `explin`, `expexp` (defined in `mathutils.h`) for value scaling. These are inspired by SuperCollider's mapping functions — they clamp, handle edge cases, and read clearly:

```cpp
float freq_hz = linlin(lfo_val, -1.f, 1.f, 100.f, 1600.f);
float cutoff  = linlin(cutoff_knob, 0.f, 1.f, 200.f, 16000.f);
```

## Build System

Each pedal has its own directory under `software/` with a `Makefile`:

```makefile
TARGET = my_effect
CPP_SOURCES = my_effect.cpp
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
C_INCLUDES += -I../../include
C_INCLUDES += -Ilib
```

The sim framework in `software/sim/` uses CMake and mocks all of libDaisy so effects can be tested offline against WAV files.

## Creative DSP Patterns in This Codebase

Study these — they are the vocabulary of this project:

- **Hilbert frequency shifting** (`Cenote`): 12-pole IIR all-pass network for real-time SSB modulation without FFT. Single-sample latency.
- **Polyphonic pitch detection** (`GuitarSynth`): STFT with harmonic salience scoring, iterative harmonic cancellation, voice tracking across frames.
- **PolyBLEP oscillators** (`GuitarSynth`, `ChivoDub`): band-limited saw/square with minimal computation. The standard for alias-free synthesis on embedded.
- **Asymmetric crossfade** (`Cenote`): nonlinear dry/wet curves where below 50% the dry stays full while wet ramps in, and above 50% wet stays full while dry ramps out.
- **Momentary footswitch detection**: track hold time; >300 ms = momentary mode, release = bypass.
- **Infinite hold / freeze**: feedback = 1.0 with `SoftLimit` to sustain without blowup.
- **Audio-rate FM**: LFO rate x32 for metallic/bell tones from a simple pitch modulator (`ChivoDub`).
- **Spectral flatness** for tonality detection: `1 - geometric_mean / arithmetic_mean` to distinguish pitched from unpitched input (`GuitarSynth`).
- **Zero-crossing pitch tracking** with median filtering for monophonic PLL synthesis (`pedro-lorenzo-lopez`).

## ISR Safety

The audio callback is an interrupt service routine. These things are **forbidden** inside it:

- Heap allocation (`new`, `malloc`, `std::vector::push_back`)
- Blocking calls (`printf`, `System::Delay`, file I/O, mutex locks)
- Non-reentrant library functions
- Anything with unbounded execution time

Keep the ISR minimal: read inputs, run the DSP chain, write outputs. Defer heavy work (FFT analysis, pitch detection, UI updates) to the main loop using `volatile` flags.

## Testing

Use the sim framework in `software/sim/` to test effects offline before flashing:

1. Build the sim: `cd software/sim && cmake -B build && cmake --build build`
2. Run against a WAV: `./build/sim_cenote --input test.wav --output out.wav`
3. Configure knobs/switches via CLI: `--knob1 0.5 --switch1 left --fsw1 press`
4. Listen to the output WAV. Your ears are the final test.

The sim mocks all of libDaisy (ADC, switches, GPIO, audio callback) so the same `.cpp` compiles for both hardware and host. Use `#ifdef SIM_BUILD` for debug instrumentation (signal probes, peak/RMS tracking).

## When Writing New Effects

1. Start from an existing effect as a template — copy the structure, not the DSP.
2. Keep the signal chain explicit: one clear path from `in[0][i]` to `out[0][i]`.
3. Think about what happens at the extremes: knob at 0, knob at 1, feedback at max, input silence, input clipping.
4. Every parameter change must be smooth. If you can hear the knob stepping, add a `fonepole`.
5. Test with the sim before flashing. Generate WAV files and listen.
6. Latency budget: at 48 kHz with a 4-sample block, you have 83 microseconds. Know your cost.
7. Prefer `float` over `double` — the Cortex-M7 FPU is single-precision. Double-precision ops fall back to software emulation.
8. Use CMSIS-DSP (`arm_rfft_fast_f32`, `arm_math.h`) for FFT and bulk math when performance matters — it uses SIMD and is tuned for this chip.
9. **Every effect must have a `README.md`** in its directory. Include: one-line description, how the algorithm works (brief), control mapping (knobs, switches, footswitches, DIP switches as tables), build/sim instructions, ASCII signal chain diagram, and credits/references for any ported algorithms. See `software/Boca/README.md` or `software/SubNUp/README.md` for examples.

## Boundaries

- **Always do**: `SoftClip` outputs, smooth parameters, static allocation only, test with the sim
- **Ask first**: adding new submodules or dependencies, changing `funbox.h` pin mappings, modifying the sim framework's mock API
- **Never do**: heap allocation in audio code, blocking calls in the ISR, commit `.wav` files or build artifacts (see `.gitignore`), modify `libDaisy/` or `DaisySP/` submodules directly
