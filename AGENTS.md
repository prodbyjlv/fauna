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
| Project Setup | ? |
| Audio Engine | ? |
| HTTP Server | ? |
| WebSocket Streaming | ? |
| Plugin UI | ? |
| QR Code Display | ? |
| Mobile Webpage | ? |
| Multi-device | ?? |

---

## Project Structure

```
FAUNA/
+-- FAUNA.jucer
+-- Source/
¦   +-- PluginProcessor.cpp/h
¦   +-- PluginEditor.cpp/h
¦   +-- AudioStreamer.cpp/h
¦   +-- WebServer.cpp/h
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

*Last Updated: April 2, 2026*
