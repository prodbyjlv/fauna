# Project FAUNA - Audio Streaming Plugin

## Overview

**Project FAUNA** is a VST3 audio plugin that streams DAW audio to mobile devices over WiFi, with remote mute control via webpage.

---

## Features

- Stream audio from DAW to mobile device
- QR code display for easy URL access
- Mute/unmute control via mobile webpage
- Real-time audio streaming via WebSocket

---

## Technical Specifications

- **Format:** VST3 (Windows)
- **Audio:** Stereo, matches DAW sample rate
- **Networking:** HTTP + WebSocket on port 8080
- **Max Devices:** 2 simultaneous

---

## Implementation Status

| Phase | Status |
|-------|--------|
| Project Setup | ✅ |
| Audio Engine | ✅ |
| HTTP Server | ✅ |
| WebSocket Connection | ✅ (fixed SHA-1 + base64) |
| Plugin UI | ✅ |
| URL Display | ✅ (QR code removed) |
| Mobile Webpage (status) | ✅ (HTTP polling) |
| Mobile Webpage (audio) | ❌ (not implemented) |
| Audio Streaming | ❌ (not implemented) |

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
│   +-- index.html (mobile control page)
+-- AGENTS.md
```

---

## How to Use

1. Load FAUNA in a DAW
2. Play audio in your DAW
3. Open URL on your phone (http://IP:8080)
4. Audio streams to your phone!

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

## Build Status
**Last build:** April 2, 2026 - Successful

---

## Session History

### April 2, 2026 - WebSocket Connection Fixed

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

## Next Steps (Priority Order)

### 1. Implement Audio Streaming (HIGH PRIORITY)
- Connect AudioStreamer to WebServer
- Send audio data via WebSocket binary frames
- Handle multiple clients (max 2)

### 2. Update Mobile Webpage (HIGH PRIORITY)
- Add WebSocket connection for audio
- Add AudioContext / Web Audio API playback
- Add mute/unmute toggle with WebSocket messaging

### 3. Add Mute Control (MEDIUM PRIORITY)
- Implement setMuteState() in WebServer
- Send mute messages from webpage to plugin

### 4. Polish (LOW PRIORITY)
- Add level meter visualization
- Add connection status indicator
- Consider adding back QR code

---

## Files Modified

- `Source/WebServer.cpp` - Fixed SHA-1, base64, WebSocket handshake

---

*Last Updated: April 2, 2026*
