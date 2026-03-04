cmake_minimum_required(VERSION 3.14)
project(funbox_sim CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Repo root (two levels up from software/sim)
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(DAISYSP_DIR ${REPO_ROOT}/DaisySP/Source)

# Mock library: replaces libDaisy for host-native builds
add_library(daisy_mock STATIC
    src/mock_gpio.cpp
    src/mock_system.cpp
    src/mock_ctrl.cpp
    src/mock_switch.cpp
    src/mock_parameter.cpp
    src/mock_seed.cpp
    src/mock_petal.cpp
)

target_include_directories(daisy_mock PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# DaisySP: pure DSP library compiled natively
set(DAISYSP_LGPL_DIR ${REPO_ROOT}/DaisySP/DaisySP-LGPL/Source)

file(GLOB_RECURSE DAISYSP_SOURCES ${DAISYSP_DIR}/*.cpp)
file(GLOB_RECURSE DAISYSP_LGPL_SOURCES ${DAISYSP_LGPL_DIR}/*.cpp)
add_library(daisysp STATIC ${DAISYSP_SOURCES} ${DAISYSP_LGPL_SOURCES})
target_compile_definitions(daisysp PUBLIC USE_DAISYSP_LGPL=1)
target_include_directories(daisysp PUBLIC
    ${DAISYSP_DIR}
    ${DAISYSP_DIR}/Control
    ${DAISYSP_DIR}/Drums
    ${DAISYSP_DIR}/Dynamics
    ${DAISYSP_DIR}/Effects
    ${DAISYSP_DIR}/Filters
    ${DAISYSP_DIR}/Noise
    ${DAISYSP_DIR}/PhysicalModeling
    ${DAISYSP_DIR}/Sampling
    ${DAISYSP_DIR}/Synthesis
    ${DAISYSP_DIR}/Utility
    ${DAISYSP_LGPL_DIR}
    ${DAISYSP_LGPL_DIR}/Control
    ${DAISYSP_LGPL_DIR}/Dynamics
    ${DAISYSP_LGPL_DIR}/Effects
    ${DAISYSP_LGPL_DIR}/Filters
    ${DAISYSP_LGPL_DIR}/PhysicalModeling
    ${DAISYSP_LGPL_DIR}/Synthesis
    ${DAISYSP_LGPL_DIR}/Utility
)

# Common paths
set(FUNBOX_INCLUDE ${REPO_ROOT}/include)
set(SOFTWARE_DIR ${REPO_ROOT}/software)

# ---------- Milestone 1: compilation test ----------
add_executable(test_compile tests/test_compile.cpp)
target_link_libraries(test_compile PRIVATE daisy_mock)

# ---------- Milestone 2: control tests ----------
add_executable(test_controls tests/test_controls.cpp)
target_link_libraries(test_controls PRIVATE daisy_mock)

# ---------- Milestone 3: DaisySP native ----------
add_executable(test_daisysp tests/test_daisysp.cpp)
target_link_libraries(test_daisysp PRIVATE daisysp)

# Sim runner library
add_library(sim_runner STATIC harness/sim_runner.cpp)
target_include_directories(sim_runner PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/harness
)
target_link_libraries(sim_runner PUBLIC daisy_mock)

# ---------- Milestone 4: audio callback harness ----------
add_executable(test_harness tests/test_harness.cpp)
target_link_libraries(test_harness PRIVATE sim_runner daisy_mock)

# ---------- Milestone 5: WAV I/O ----------
add_executable(test_wav_io tests/test_wav_io.cpp)
target_include_directories(test_wav_io PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harness)

# ==========================================================
# Pedal simulation executables
# ==========================================================

# Helper function: build a pedal sim executable
function(add_pedal_sim name pedal_source)
    set_source_files_properties(${pedal_source} PROPERTIES
        COMPILE_DEFINITIONS "main=pedal_main"
    )
    add_executable(sim_${name}
        harness/sim_main.cpp
        ${pedal_source}
        ${ARGN}
    )
    target_link_libraries(sim_${name} PRIVATE sim_runner daisy_mock daisysp)
    target_include_directories(sim_${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/harness
        ${FUNBOX_INCLUDE}
    )
endfunction()

# ---------- Milestone 6: GuitarSynth ----------
add_pedal_sim(guitarsynth
    ${SOFTWARE_DIR}/GuitarSynth/guitarsynth.cpp
)
target_include_directories(sim_guitarsynth PRIVATE
    ${SOFTWARE_DIR}/GuitarSynth/lib
)
