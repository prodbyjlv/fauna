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
- Ableton Live
- FL Studio
- Studio One
- Pro Tools (VST3 compatibility)

### Plugin Format
- VST3 (Windows primary)

---

## Technical Specifications

### Audio
- **Sample Rate:** 44.1kHz
- **Bit Depth:** 16-bit PCM
- **Channels:** Stereo
- **Format for Streaming:** Raw PCM (simplified approach)

### Networking
- **Protocol:** HTTP server using Winsock
- **Port:** 8080 (default)
- **URL Format:** `http://192.168.x.x:8080` (auto-detected local IP)
- **Max Devices:** 2 simultaneous connections (planned)

### Web Interface
- **Mute Control:** Per-device mute state (UI ready)
- **Level Meter:** Visual audio level indicator (UI ready)
- **Browser Support:** Chrome, Safari, Firefox (mobile)
- **Connection:** Local network only (no cloud)

---

## Implementation Status

### Phase 1: Project Setup ?
- [x] JUCE project initialized
- [x] Visual Studio build configured
- [x] VST3 plugin format configured
- [x] GitHub repository created
- [x] Initial build successful

### Phase 2: Audio Engine ?
- [x] Create AudioStreamer class
- [x] Capture audio via processBlock()
- [x] Implement circular buffer
- [x] Level meter functionality

### Phase 3: HTTP Server ?
- [x] HTTPServer class with Winsock
- [x] TCP socket listening
- [x] IP address detection
- [x] Status endpoint (/status)
- [x] HTML webpage served
- [x] Client connection tracking

### Phase 4: Plugin UI ?
- [x] Display title "FAUNA"
- [x] Display local IP address
- [x] Display URL (http://IP:8080)
- [x] Show connection status
- [x] Level meter display

### Phase 5: Mobile Webpage ?
- [x] HTML/CSS styling
- [x] Mute/unmute button
- [x] Level meter visualization
- [x] Responsive mobile design
- [x] Dark theme with cyan accent

### Phase 6: Audio Streaming
- [ ] WebSocket for real-time audio
- [ ] Client-side audio playback
- [ ] Actual audio streaming

### Phase 7: Multi-Device Support
- [ ] Handle 2 simultaneous connections
- [ ] Per-device mute state

### Phase 8: QR Code
- [ ] Generate QR code in plugin UI
- [ ] Display for easy scanning

---

## Project Structure

```
FAUNA/
+-- FAUNA.jucer              # JUCE project file
+-- Source/
¦   +-- PluginProcessor.cpp  # Main processor
¦   +-- PluginProcessor.h
¦   +-- PluginEditor.cpp     # Plugin UI
¦   +-- PluginEditor.h
¦   +-- AudioStreamer.cpp    # Audio capture and buffering
¦   +-- AudioStreamer.h
¦   +-- WebServer.cpp        # HTTP server with Winsock
¦   +-- WebServer.h
+-- Builds/
¦   +-- VisualStudio2026/    # Visual Studio project
+-- AGENTS.md               # This file
```

---

## Design Preferences

- **Initial UI:** Basic/minimal
- **Level Meter:** Yes, implemented in AudioStreamer
- **Mobile Page:** Dark gradient background (#1a1a2e to #16213e), cyan accent (#00d4ff)
- **Mute Button:** Green (unmuted) / Red (muted)
- **Font:** System fonts (Segoe UI, Roboto, sans-serif)

---

## Developer Notes

- Developer: Joshua (prodbyjlv)
- Email: prodbyjlv@gmail.com
- GitHub: https://github.com/prodbyjlv/fauna

### Development Environment
- Windows 10/11
- Visual Studio Community
- JUCE 8.0.12
- JUCE path: `C:\JLV JUCE\juce-8.0.12-windows\JUCE`

### Next Steps
1. Add WebSocket for real-time audio streaming
2. Implement actual audio playback on mobile webpage
3. Add QR code generation to plugin UI
4. Test with real mobile device

---

## Build Status

**Last successful build:** April 2, 2026
- VST3 plugin compiles successfully
- HTTP server starts on port 8080
- Mobile webpage accessible
- Audio capture working

---

## Conversation History

### Session 1 (April 2, 2026)
- Set up JUCE project from scratch
- Configured VST3 build in Visual Studio
- Resolved JUCE module path issues
- Successfully built empty VST3 plugin
- Pushed project to GitHub
- Created AGENTS.md file

### Session 2 (April 2, 2026)
- Integrated AudioStreamer class (audio capture and buffering)
- Integrated WebServer class (basic HTTP framework)
- Resolved multiple build issues (threading, OpenSSL, file encoding)
- VST3 plugin builds successfully
- Code pushed to GitHub

### Session 3 (April 2, 2026)
- Added HTTPServer with Winsock for HTTP server
- Created mobile webpage with mute/unmute and level meter
- Updated plugin UI with status, IP, URL display
- HTTP server successfully starts with plugin
- Code pushed to GitHub

---

*Last Updated: April 2, 2026*
