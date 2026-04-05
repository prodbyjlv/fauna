# Project FAUNA - Audio Streaming Plugin

## Overview

**FAUNA** is a VST3 audio plugin that streams DAW audio to mobile devices over WiFi, with remote mute control via webpage.

---

## Features

- Stream audio from DAW to mobile device
- Real-time audio streaming via WebSocket
- Mute/unmute control via mobile webpage
- Level meter visualization on mobile
- Support for up to 2 simultaneous devices
- iPhone and Android support
- Sample rate resampling (44100Hz → 48000Hz)
- Ring buffer for smooth audio playback
- Prebuffering to prevent crackling
- QR code display for easy URL access

---

## Technical Specifications

- **Format:** VST3 (Windows)
- **Audio:** Stereo, matches DAW sample rate
- **Networking:** HTTP + WebSocket on port 8080
- **Max Devices:** 2 simultaneous

---

## Version History

### v1.2 - UI Redesign (April 5, 2026)

**Status:** ✅ WORKING

**Major changes:**
- Complete plugin UI redesign with new color scheme (warm brown background, cream/coral/green accents)
- New 340x400px window size
- QR code now generates dynamically from URL
- Diagonal watermark background pattern
- Updated meta row layout (URL / Port / SR)
- Green status dot for connected devices
- Complete mobile browser redesign matching plugin aesthetic
- Mobile info cards (Server, Audio, Sample Rate, Buffer)
- START AUDIO button for browser audio policy compliance
- Level meter visualization

**Files modified:**
- PluginEditor.h - Complete UI redesign
- PluginEditor.cpp - New paint methods, dynamic QR generation
- WebServer.h - Added getSampleRate()
- WebServer.cpp - New mobile HTML with matching design

### v1.1.5 - iPhone + Android Support (April 4, 2026)

**Status:** ✅ WORKING

**Major improvements:**
- Fixed iPhone audio routing (Audio Session API: `navigator.audioSession.type = 'playback'`)
- Android audio pitch correction (sample rate resampling)
- Ring buffer for smooth audio playback (192000 floats = 2 seconds)
- Prebuffering to prevent crackling (4096 frames before playback)
- ScriptProcessorNode for real-time buffer filling
- Valid WAV data handling for iOS

### v1.0.0 - Initial Release (April 2, 2026)

**Status:** ✅ WORKING

First functional version with audio streaming capability.

---

## Implementation Status

| Phase | Status |
|-------|--------|
| Project Setup | ✅ |
| Audio Engine | ✅ |
| HTTP Server | ✅ |
| WebSocket Connection | ✅ |
| Plugin UI | ✅ |
| URL Display | ✅ |
| Mobile Webpage | ✅ |
| Audio Streaming | ✅ |
| Mute Control | ✅ |
| Level Meter | ✅ |
| QR Code | ✅ |
| Sample Rate Resampling | ✅ |
| Ring Buffer | ✅ |
| Prebuffering | ✅ |

---

## Project Structure

```
FAUNA/
+-- FAUNA.jucer
+-- Source/
│   +-- PluginProcessor.cpp/h
│   +-- PluginEditor.cpp/h
│   +-- AudioStreamer.cpp/h
│   +-- WebServer.cpp/h
│   +-- QrCode.cpp/h
+-- web/
│   +-- index.html
+-- AGENTS.md
```

---

## How to Use

1. Load FAUNA in a DAW (FL Studio, etc.)
2. Play audio in your DAW
3. Open URL on your phone: http://YOUR_IP:8080
4. Click "START AUDIO" button
5. Audio streams to your phone!

---

## Developer

- **Name:** Joshua
- **Email:** prodbyjlv@gmail.com
- **GitHub:** https://github.com/prodbyjlv/fauna

### Environment
- Windows 10/11
- Visual Studio Community
- JUCE 8.0.12
- JUCE path: `C:\JLV JUCE\juce-8.0.12-windows\JUCE`
- Project path: `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA`

---

## Session History

### April 2, 2026 - Session 1: WebSocket Connection Fixed

**Problem:** Browser disconnected immediately after WebSocket handshake (error 1006).

**Debug Logs:**
```
FAUNA: recv returned 0, WSA error 0
```
Browser showed "CLOSE code:1006"

**Root Causes Identified:**
1. SHA-1 implementation had incorrect padding calculation
2. base64Encode had incorrect padding logic
3. JUCE string lifetime issues in generateWebSocketKey

**Solution Applied:**
1. Rewrote SHA-1 with correct padding: `totalLen = ((len + 1 + 8 + 63) / 64) * 64`
2. Fixed base64Encode with proper `remaining` checks
3. Used std::string to avoid JUCE pointer lifetime issues
4. Removed direct handleWebSocketClient() call after handshake

**Result:** 
- ✅ WebSocket connection succeeds
- ✅ Browser shows "Connected!"
- ✅ Plugin shows device connected
- ✅ Proper disconnect on plugin close

---

### April 2, 2026 - Session 2: Audio Streaming Implemented

**Problem:** No audio was being streamed to mobile device - broadcastAudio() was empty.

**Issues Found:**
1. PluginProcessor didn't send audio to WebServer
2. WebServer.broadcastAudio() was empty - no implementation
3. No audio retrieval mechanism from AudioStreamer

**Solution Applied:**

1. **PluginProcessor.cpp** - Added audio sending after processBlock:
   - Interleaves stereo audio [L0,R0, L1,R1, ...]
   - Clamps values to [-1, 1] to prevent WebSocket corruption
   - Calls httpServer.writeAudioData()

2. **WebServer.cpp** - Implemented broadcastAudio():
   - Sends binary WebSocket frames (opcode 0x02) to all connected clients
   - Checks muted state - sends silence if muted
   - Uses non-blocking sockets for audio streaming
   - Large send buffer (256KB) for smooth streaming

3. **Embedded HTML** - getHTMLPage() now serves full mobile page:
   - WebSocket connection for audio
   - AudioContext with scheduled buffer playback
- Level meter visualization (2.5x magnification for better visual feedback)
   - Mute/unmute toggle button
   - Server status display

**Testing:**
- ✅ Audio streams from FL Studio to phone
- ✅ WebSocket connection works
- ✅ Level meter responds to audio
- ✅ Mute control functional

---

### April 3-4, 2026 - iPhone + Android Support

**Problems:**
1. Android audio played at wrong pitch/speed (8.8% faster)
2. iPhone audio didn't play through speakers (only Bluetooth)
3. Audio had crackling/popping on mobile devices

**Root Causes Identified:**
1. Sample rate mismatch: FL Studio 44100Hz vs Android 48000Hz
2. iOS WebAudio defaults to "ambient" audio session (routes to earpiece)
3. No buffering mechanism for smooth playback

**Solutions Applied:**

1. **Sample Rate Resampling** - Linear interpolation resampler:
   - Converts FL Studio 44100Hz to device 48000Hz
   - Audio plays at correct pitch/speed on Android

2. **iOS Audio Session** - Added `navigator.audioSession.type = 'playback'`:
   - Tells iOS to treat web app as media app
   - Audio routes to speakers instead of earpiece

3. **Ring Buffer** - 192000 floats (2 seconds):
   - Smooths out timing variations
   - Prevents underrun/overrun

4. **Prebuffering** - 4096 frames before playback starts:
   - Ensures buffer is full before audio begins
   - Eliminates crackling at start

**Testing:**
- ✅ Android: Audio plays at correct pitch
- ✅ iPhone: Audio plays through speakers
- ✅ Android: No crackling/popping
- ✅ iPhone: No crackling/popping
- ✅ Bluetooth headphones work on both platforms

---

## Files Modified

### Source/WebServer.cpp
- Fixed SHA-1 implementation with correct padding
- Fixed base64Encode with proper padding
- Implemented broadcastAudio() for sending audio to clients
- Added full embedded HTML page with audio streaming UI
- Non-blocking sockets for audio streaming
- Client limit (max 2)
- Sample rate resampling for Android compatibility

### Source/PluginProcessor.cpp
- Added audio interleaving for WebSocket transmission
- Added value clamping to prevent corruption
- Added call to httpServer.writeAudioData()

### Source/QrCode.cpp/h
- QR code generation for easy URL access
- Displayed in plugin UI

### web/index.html
- Full mobile control page with WebSocket audio
- ScriptProcessorNode for real-time buffer filling
- Ring buffer (192000 floats) for smooth playback
- Level meter visualization
- Mute/unmute toggle
- iOS Audio Session API: `navigator.audioSession.type = 'playback'`

---

## Build Instructions

1. Open FAUNA.jucer in Projucer
2. Select Visual Studio 2022 (or your preferred IDE)
3. Build in Release mode for better performance
4. Copy FAUNA.vst3 to `C:\Program Files\Common Files\VST3\`

---

*Last Updated: April 4, 2026*
