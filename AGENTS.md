# Project FAUNA - Audio Streaming Plugin

## Overview

**Project FAUNA** is a VST3 audio plugin that allows users to stream DAW audio playback to mobile devices over a local WiFi network, with remote mute/unmute control via a webpage.

---

## Plugin Concept

### What It Does
- Streams audio from a DAW to any mobile device on the same WiFi network
- Users scan a QR code in the plugin to access a webpage
- Mobile webpage allows mute/unmute of audio playback on the device only
- Supports testing audio on up to 2 mobile devices simultaneously

### Use Case
- Hear how your mix/master sounds on a mobile device (phone/tablet)
- Remote monitoring without cables
- Quick A/B testing between studio monitors and mobile playback

### Supported DAWs
- Ableton Live, FL Studio, Studio One, Pro Tools (VST3)

### Plugin Format
- VST3 (Windows primary)

---

## Technical Specifications

### Audio
- **Sample Rate:** 44.1kHz (matches DAW)
- **Bit Depth:** 32-bit float (16-bit output)
- **Channels:** Stereo
- **Format for Streaming:** Raw PCM via WebSocket

### Networking
- **Protocol:** HTTP + WebSocket (custom implementation)
- **Port:** 8080
- **URL:** `http://192.168.x.x:8080`
- **Max Devices:** 2 simultaneous

---

## Implementation Status

### Phase 1: Project Setup ?
- [x] JUCE project initialized
- [x] VST3 build configured
- [x] GitHub repository created

### Phase 2: Audio Engine ?
- [x] AudioStreamer class
- [x] Capture audio via processBlock()
- [x] Circular buffer
- [x] Level meter

### Phase 3: HTTP Server ?
- [x] HTTPServer with Winsock
- [x] TCP socket listening
- [x] IP detection
- [x] Status endpoint
- [x] HTML webpage

### Phase 4: Plugin UI ?
- [x] Title display
- [x] IP address
- [x] URL display
- [x] Connection status

### Phase 5: WebSocket Audio Streaming ?
- [x] WebSocket handshake
- [x] Custom SHA1/base64 (no OpenSSL)
- [x] Audio broadcast to clients
- [x] Mobile webpage with WebSocket client
- [x] Audio playback in browser

### Phase 6: Mobile Controls ?
- [x] Mute/unmute button
- [x] Audio gain control
- [x] WebSocket reconnection
- [x] Visual feedback

### Phase 7: QR Code
- [ ] Generate QR code in plugin UI

---

## Project Structure

```
FAUNA/
+-- FAUNA.jucer
+-- Source/
¦   +-- PluginProcessor.cpp/h
¦   +-- PluginEditor.cpp/h
¦   +-- AudioStreamer.cpp/h
¦   +-- WebServer.cpp/h (HTTP + WebSocket)
+-- AGENTS.md
```

---

## Design
- **UI:** Dark blue gradient (#1a1a2e, #16213e)
- **Accent:** Cyan (#00d4ff)
- **Mute Button:** Green (unmuted) / Red (muted)

---

## Developer Notes
- **Developer:** Joshua (prodbyjlv)
- **Email:** prodbyjlv@gmail.com
- **GitHub:** https://github.com/prodbyjlv/fauna

### Development Environment
- Windows 10/11
- Visual Studio Community
- JUCE 8.0.12
- JUCE path: `C:\JLV JUCE\juce-8.0.12-windows\JUCE`

### Next Steps
1. Add QR code generation
2. Test audio streaming
3. Add real level meter from audio
4. Multi-device support

---

## Build Status
**Last successful build:** April 2, 2026

---

## Conversation History

### Session 1
- Set up JUCE project, configured VST3, GitHub

### Session 2
- Added AudioStreamer, WebServer, mobile webpage

### Session 3
- HTTP server with Winsock working
- Plugin UI with status display

### Session 4
- WebSocket audio streaming implemented
- Custom SHA1/base64 (no OpenSSL dependency)
- Mobile webpage plays audio via WebSocket

---

*Last Updated: April 2, 2026*
