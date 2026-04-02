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
| WebSocket Streaming | ✅ (fixed pong response) |
| Plugin UI | ✅ |
| URL Display | ✅ (QR code removed) |
| Mobile Webpage | ✅ |
| Multi-device | ✅ (max 2 clients) |

---

## Project Structure

```
FAUNA/
+-- FAUNA.jucer
+-- Source/
�   +-- PluginProcessor.cpp/h
�   +-- PluginEditor.cpp/h
�   +-- AudioStreamer.cpp/h
�   +-- WebServer.cpp/h
+-- AGENTS.md
```

---

## How to Use

1. Load FAUNA in a DAW
2. Play audio in your DAW
3. Scan the QR code with your phone
4. Open the URL in your browser
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

---

## Build Status
**Last build:** April 2, 2026 - Successful

---

## Bug Fixes Applied

### WebSocket Error 1006 (April 2, 2026)

**Problem:** Browser disconnected immediately after WebSocket handshake with error 1006.

**Root Causes:**
1. Pong response incorrectly extracted ping payload
2. `sendWebSocketFrame` was setting mask bit (0x80) on server-to-client frames

**Solution:**
1. Fixed `handleWebSocketClient` ping handler to properly unmask and extract payload
2. Fixed `sendWebSocketFrame` to NOT set mask bit on outgoing frames (servers must never mask frames sent to clients per RFC 6455)

---

*Last Updated: April 2, 2026*
