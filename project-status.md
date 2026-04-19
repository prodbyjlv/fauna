# FAUNA CMake Conversion - Project Status

## Goal

Build a macOS version (Audio Unit) of the FAUNA VST3 plugin using CMake, enabling GitHub Actions CI/CD for automated cross-platform builds.

## Why CMake Instead of Projucer?

- **Projucer limitation:** Cannot build macOS plugins on GitHub Actions CI (no Mac runner, Projucer not available on CI)
- **CMake advantage:** Works with CI, generates Xcode/VS projects, cross-platform compatible
- **Current status:** Windows builds work via Projucer. macOS needs CMake for CI automation.

## Project Paths

| Item | Path |
|------|------|
| FAUNA project root | `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA` |
| JUCE installation | `C:\JLV JUCE\juce-8.0.12-windows\JUCE` |
| Source files | `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA\Source` |
| CMakeLists.txt | `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA\CMakeLists.txt` |
| FAUNA.jucer (backup) | `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA\FAUNA.jucer` |
| CMake build folder | `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA\build` |
| CMake output (VST3) | `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA\build\FAUNA_artefacts\Release\VST3\FAUNA.vst3\Contents\x86_64-win\FAUNA.vst3` |

## What Was Changed (Phase 1-2 → Reverted)

### Original (Windows-only)
- WebServer.cpp/h used Windows sockets (winsock2.h)
- FAUNA.jucer had `pluginFormats="buildVST3"`

### Phase 1-2 Attempt (Cross-platform)
- Modified WebServer for cross-platform (Windows + macOS)
- Added buildAU to FAUNA.jucer
- Then reverted because it complicated the plan

### Current State
- WebServer.cpp/h is back to **Windows-only** (original)
- FAUNA.jucer is back to **`pluginFormats="buildVST3"`** (original)

## Phase 1: CMake Conversion - COMPLETED ✅

### Step 1.1 to 1.6 - Completed
Created `CMakeLists.txt` with:
- cmake_minimum_required(VERSION 3.27)
- project(FAUNA VERSION 1.2)
- find_package(JUCE CONFIG REQUIRED)
- juce_add_plugin(FAUNA FORMATS VST3 AU ...)
- All source files linked
- All JUCE modules linked
- juce_generate_juce_header(FAUNA) added
- JUCE_VST3_CAN_REPLACE_VST2=0 added to fix build error

### Step 2.1 - Completed
CMake generates Visual Studio project successfully.

### Step 2.2 - Completed ✅
Build succeeded - FAUNA.vst3 created.

### Step 2.3 - Not Started
Test in DAW - needs to be done manually

### Step 2.4 - Not Started
Verify VST3 works in FL Studio or similar

## Build Output

**CMake Build:** SUCCESS ✅
```
C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA\build\FAUNA_artefacts\Release\VST3\FAUNA.vst3\Contents\x86_64-win\FAUNA.vst3
```

## Next Steps to Complete

### Step 3.1 - Create GitHub Actions workflow
Create `.github/workflows/build.yml` for:
- Windows VST3 build (on push to master/mv1)
- macOS Audio Unit build (on push to master/mv1)

### Step 3.2 - Test CI workflow
Verify CI runs on GitHub Actions

### Step 3.3 - Set up artifact upload
Upload VST3 and AU builds as downloadable artifacts

## Files Created During This Session

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Main CMake build config |
| `cmake-plan.md` | Plan document |
| `macversion-plan-1.md` | Mac version plan |
| `project-status.md` | This document - current status |

## Files That Exist (Not Modified)

| File | Purpose |
|------|---------|
| `FAUNA.jucer` | Original Projucer config (backup) |
| `Source/WebServer.cpp` | Windows-only (reverted) |
| `Source/WebServer.h` | Windows-only (reverted) |
| `Source/PluginProcessor.cpp/h` | Unchanged |
| `Source/PluginEditor.cpp/h` | Unchanged |
| `Source/AudioStreamer.cpp/h` | Unchanged |
| `Source/QrCode.cpp` | Unchanged |

## Backup Locations

| Backup | Location |
|--------|----------|
| Full project backup | `prodbyjlv/fauna-vst3-plugin-full-backup` (GitHub private) |
| mv1 branch (mac plan) | `prodbyjlv/fauna` (GitHub public) |

## Current CMakeLists.txt Contents

```cmake
cmake_minimum_required(VERSION 3.27)

project(FAUNA VERSION 1.2)

find_package(JUCE CONFIG REQUIRED HINTS "C:/JLV JUCE/juce-8.0.12-windows/JUCE" NO_DEFAULT_PATH)

juce_add_plugin(FAUNA
    FORMATS VST3 AU
    COMPANY_NAME "JLVX"
    COMPANY_EMAIL "prodbyjlv@gmail.com"
    PLUGIN_NAME "FAUNA"
    PRODUCT_NAME "FAUNA"
    PLUGIN_MANUFACTURER_CODE "JLVX"
    PLUGIN_CODE "Funa"
)

target_sources(FAUNA PRIVATE
    Source/AudioStreamer.cpp
    Source/AudioStreamer.h
    Source/WebServer.cpp
    Source/WebServer.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/QrCode.cpp
)

target_link_libraries(FAUNA PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_processors_headless
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_osc
)

target_compile_features(FAUNA PRIVATE cxx_std_17)
target_compile_definitions(FAUNA PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

juce_generate_juce_header(FAUNA)
```

## Commands Used

```bash
# Configure (done)
cmake -S . -B build -G "Visual Studio 18 2026"

# Build (done)
cmake --build build --config Release

# Output location:
# build/FAUNA_artefacts/Release/VST3/FAUNA.vst3/Contents/x86_64-win/FAUNA.vst3
```

## Next: GitHub Actions CI Setup

The next phase is creating `.github/workflows/build.yml` to automate:
1. Windows VST3 build via CMake
2. macOS AU build via CMake (on macOS runner)

This will enable automated builds without needing a Mac locally.

## Status Summary

| Phase | Status |
|-------|--------|
| Phase 1: CMake setup | ✅ COMPLETE |
| Phase 2: Local build | ✅ COMPLETE |
| Phase 3: CI workflow | ⏳ PENDING |
| Phase 4: Documentation | ⏳ PENDING |