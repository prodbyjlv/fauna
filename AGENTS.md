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

---

## Technical Specifications

- **Format:** VST3 (Windows)
- **Audio:** Stereo, matches DAW sample rate
- **Networking:** HTTP + WebSocket on port 8080
- **Max Devices:** 2 simultaneous

---

## Version History

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
| QR Code | ❌ (not implemented) |

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
   - Level meter visualization
   - Mute/unmute toggle button
   - Server status display

**Testing:**
- ✅ Audio streams from FL Studio to phone
- ✅ WebSocket connection works
- ✅ Level meter responds to audio
- ✅ Mute control functional

---

## Files Modified

### Source/WebServer.cpp
- Fixed SHA-1 implementation with correct padding
- Fixed base64Encode with proper padding
- Implemented broadcastAudio() for sending audio to clients
- Added full embedded HTML page with audio streaming UI
- Non-blocking sockets for audio streaming
- Client limit (max 2)

### Source/PluginProcessor.cpp
- Added audio interleaving for WebSocket transmission
- Added value clamping to prevent corruption
- Added call to httpServer.writeAudioData()

### web/index.html
- Full mobile control page with WebSocket audio
- AudioContext playback with buffer scheduling
- Level meter visualization
- Mute/unmute toggle

---

## Known Issues / Future Improvements

1. **QR Code** - Not implemented yet - needed for easy distribution
2. **Buffer underrun handling** - Could be improved for smoother playback
3. **Latency adjustment** - Could add user-adjustable latency setting
4. **Multiple device sync** - Currently each device plays independently

---

## Build Instructions

1. Open FAUNA.jucer in Projucer
2. Select Visual Studio 2022 (or your preferred IDE)
3. Build in Release mode for better performance
4. Copy FAUNA.vst3 to `C:\Program Files\Common Files\VST3\`

---

*Last Updated: April 2, 2026*
