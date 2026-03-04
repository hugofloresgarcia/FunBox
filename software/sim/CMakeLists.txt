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
file(GLOB_RECURSE DAISYSP_SOURCES ${DAISYSP_DIR}/*.cpp)
add_library(daisysp STATIC ${DAISYSP_SOURCES})
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
)

# ---------- Milestone 1: compilation test ----------
add_executable(test_compile tests/test_compile.cpp)
target_link_libraries(test_compile PRIVATE daisy_mock)

# ---------- Milestone 2: control tests ----------
add_executable(test_controls tests/test_controls.cpp)
target_link_libraries(test_controls PRIVATE daisy_mock)

# ---------- Milestone 3: DaisySP native ----------
add_executable(test_daisysp tests/test_daisysp.cpp)
target_link_libraries(test_daisysp PRIVATE daisysp)
