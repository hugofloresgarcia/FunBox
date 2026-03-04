# FunBox Simulation Environment

Host-native simulation of FunBox pedal firmware. Runs the real pedal DSP code on your
computer using mock implementations of the Daisy hardware layer, so you can test effects
with WAV files without flashing hardware.

## Prerequisites

- CMake 3.14+
- A C++17 compiler (clang, gcc, MSVC)
- The FunBox repo with DaisySP submodule initialized

## Build

```bash
cd software/sim
mkdir build && cd build
cmake ..
make            # builds everything
# or build a specific target:
make sim_guitarsynth
make sim_cenote
```

## Usage

Each `sim_<pedal>` executable processes audio through the pedal's DSP with configurable
controls.

```bash
# Process a WAV file through GuitarSynth
./sim_guitarsynth --input guitar.wav --output synth.wav \
    --fsw1 \
    --knob1 0.5 --knob2 0.8 --knob3 0.3 \
    --knob4 0.6 --knob5 0.7 --knob6 0.5

# Use a generated sine tone (no input file needed)
./sim_cenote --fsw1 --freq 440 --duration 2.0 --output delay.wav \
    --knob2 0.6 --knob3 0.5 --knob5 0.3

# See all options
./sim_guitarsynth --help
```

### Options

| Option | Description |
|--------|-------------|
| `--input <file.wav>` | Input WAV file (mono or stereo, any sample rate) |
| `--output <file.wav>` | Output WAV file (default: `output.wav`) |
| `--freq <hz>` | Frequency for generated sine input (default: 440) |
| `--duration <sec>` | Duration of generated input (default: 1.0) |
| `--knob1` .. `--knob6` | Knob positions, 0.0 to 1.0 (default: 0.5) |
| `--switch1` .. `--switch3` | 3-way switch: `left`, `center`, or `right` |
| `--fsw1`, `--fsw2` | Enable footswitch 1 or 2 (effect on) |
| `--dip1` .. `--dip4` | Enable DIP switches |

### Knob mapping (per pedal)

Refer to each pedal's README for what each knob controls. The knob numbering
matches the FunBox hardware layout (knob1 = top-left, knob6 = bottom-right).

## Adding a new pedal

1. Add a CMake target in `CMakeLists.txt`:

```cmake
add_pedal_sim(mypedal
    ${SOFTWARE_DIR}/MyPedal/mypedal.cpp
)
target_include_directories(sim_mypedal PRIVATE
    ${SOFTWARE_DIR}/MyPedal/lib
)
```

2. Build: `make sim_mypedal`

The `add_pedal_sim` function handles the `main()` rename and links the mock
Daisy library + DaisySP automatically. If your pedal uses additional libraries
(RTNeural, CloudSeed, etc.), add them as extra sources or link targets.

## Architecture

```
sim/
  include/        Mock libDaisy headers (DaisyPetal, Switch, Led, etc.)
  src/            Mock implementations
  harness/        Sim runner (CLI, WAV I/O, audio loop driver)
  tests/          Milestone tests (compile, controls, DaisySP, harness, WAV)
  CMakeLists.txt  Build system
```

The mock layer shadows real libDaisy headers so pedal code compiles unmodified.
DaisySP compiles natively (it's pure DSP math with no hardware dependencies).

## Running tests

```bash
cd build
make test_compile && ./test_compile
make test_controls && ./test_controls
make test_daisysp && ./test_daisysp
make test_harness && ./test_harness
make test_wav_io && ./test_wav_io
```

## Currently supported pedals

- **GuitarSynth** - Polyphonic pitch-to-synth (FFT pitch detection, synth voices, SVF filter)
- **Cenote** - Vibrato + delay + frequency shift + crossfade
