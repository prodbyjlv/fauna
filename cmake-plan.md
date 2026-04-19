# Option B: CMake Conversion Plan

## Overview

Convert the FAUNA project from JUCE Projucer (.jucer) format to CMake format. This enables cross-platform CI/CD on GitHub Actions.

---

## Phase 1: Create CMakeLists.txt

### Step 1.1 - Create base CMakeLists.txt

Create `CMakeLists.txt` in the FAUNA folder with:
- cmake_minimum_required(3.22)
- project name and version
- JUCE version 8.0.12

### Step 1.2 - Add JUCE framework

Point to existing JUCE installation:
```cmake
set(JUCE_VERSION 8.0.12)
add_subdirectory("C:/JLV JUCE/juce-8.0.12-windows/JUCE")
```

### Step 1.3 - Add plugin configuration

```cmake
juce_add_plugin(FAUNA
    FORMATS VST3 AU
    COMPANY_NAME "JLV"
    COMPANY_EMAIL "prodbyjlv@gmail.com"
    PLUGIN_NAME "FAUNA"
    VERSION "1.2"
)
```

### Step 1.4 - Add source files

List all .cpp and .h files in Source/ folder:
- AudioStreamer.cpp/h
- PluginEditor.cpp/h
- PluginProcessor.cpp/h
- WebServer.cpp/h
- QrCode.cpp

### Step 1.5 - Add required JUCE modules

From current FAUNA.jucer:
- juce_audio_basics
- juce_audio_devices
- juce_audio_formats
- juce_audio_plugin_client
- juce_audio_processors
- juce_audio_processors_headless
- juce_core
- juce_data_structures
- juce_events
- juce_graphics
- juce_gui_basics
- juce_gui_extra
- juce_osc

### Step 1.6 - Add compiler flags

- Set C++ standard to 17
- Add JUCE_STRICT_REFCOUNTEDPOINTER=1

---

## Phase 2: Test Local Build (Windows)

### Step 2.1 - Generate Visual Studio project

Run CMake to generate .sln file:
```bash
cmake -S . -B build -G "Visual Studio 17 2022"
```

### Step 2.2 - Build Release version

```bash
cmake --build build --config Release
```

### Step 2.3 - Verify FAUNA.vst3 is created

Check `build/Release/FAUNA.vst3` exists.

### Step 2.4 - Test in DAW

Manually verify the plugin works in FL Studio or similar.

---

## Phase 3: Create GitHub Actions CI

### Step 3.1 - Create .github/workflows folder

Create `.github/workflows/build.yml`

### Step 3.2 - Add Windows CI job

Runs on: windows-latest
Builds: VST3
Commands:
- cmake -S . -B build -G "Visual Studio 17 2022"
- cmake --build build --config Release

### Step 3.3 - Add macOS CI job

Runs on: macos-latest
Builds: AU
Commands:
- cmake -S . -B build -G Xcode
- cmake --build build --config Release

### Step 3.4 - Add Ubuntu CI job (optional)

Runs on: ubuntu-latest
Builds: VST3

### Step 3.5 - Add artifact upload

Upload build outputs from each job.

---

## Phase 4: Cleanup

### Step 4.1 - Remove old FAUNA.jucer

After CMake is verified working, delete FAUNA.jucer.

### Step 4.2 - Update AGENTS.md

Update project documentation to reflect CMake build system.

---

## Testing Checklist

- [ ] CMake generates VS project without errors
- [ ] Release build completes without errors
- [ ] FAUNA.vst3 is created and works
- [ ] GitHub Actions CI passes on Windows
- [ ] GitHub Actions CI passes on macOS
- [ ] GitHub Actions CI passes on Ubuntu
- [ ] Artifacts are uploaded correctly

---

## Files to Create/Modify

| File | Action |
|------|--------|
| CMakeLists.txt | Create |
| .github/workflows/build.yml | Create |
| AGENTS.md | Update |
| FAUNA.jucer | Delete (after verification) |

---

## Order of Execution

1. Phase 1: Create CMakeLists.txt
2. Phase 2: Test local build (Windows)
3. Phase 3: Create CI workflow
4. Phase 4: Cleanup