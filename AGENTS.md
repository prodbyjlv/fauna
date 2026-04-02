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
- **Format for Streaming:** Raw PCM (simplified approach, no Opus encoding yet)

### Networking
- **Protocol:** Custom WebSocket implementation for real-time audio streaming
- **Port:** 8080 (default)
- **URL Format:** `http://192.168.x.x:8080` (auto-detected local IP)
- **Max Devices:** 2 simultaneous connections

### Web Interface
- **Mute Control:** Per-device mute state
- **Level Meter:** Visual audio level indicator
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

### Phase 3: Web Server ? (Simplified)
- [x] Implement HTTP server using JUCE StreamingSocket
- [x] Serve mobile webpage
- [x] Basic WebSocket handling
- [x] REST API for mute control

### Phase 4: Plugin UI
- [ ] Display local IP address
- [ ] Generate QR code
- [ ] Show connection status
- [ ] Basic controls

### Phase 5: Mobile Webpage ?
- [x] HTML5 audio playback (embedded in WebServer)
- [x] Mute/unmute buttons
- [x] Level meter visualization
- [x] Responsive mobile design

### Phase 6: Audio Streaming
- [ ] Implement actual audio streaming via WebSocket
- [ ] Raw PCM streaming (simplified approach)
- [ ] Client-side audio playback

### Phase 7: Multi-Device Support
- [ ] Handle 2 simultaneous WebSocket connections
- [ ] Per-device mute state management

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
¦   +-- WebServer.cpp        # HTTP/WebSocket server
¦   +-- WebServer.h
+-- Builds/
¦   +-- VisualStudio2026/    # Visual Studio project
+-- web/                     # (integrated into WebServer)
+-- AGENTS.md               # This file
```

---

## Third-Party Libraries

| Library | Purpose | Status |
|---------|---------|--------|
| JUCE 8.0.12 | Audio plugin framework | ? Used |
| OpenSSL | WebSocket handshake | ? Used |
| Custom HTTP Server | Built into WebServer class | ? Implemented |

---

## Design Preferences

- **Initial UI:** Basic/minimal
- **Level Meter:** Yes, implemented
- **Mobile Page:** Simple mute/unmute with level meter (blue/green/red theme)
- **Colors:** Dark gradient background, cyan accent color

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
1. Build and test the current code
2. Add QR code generation to plugin UI
3. Implement actual WebSocket audio streaming
4. Add audio playback on client side

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
- Integrated WebServer class (HTTP server with mobile webpage)
- Updated PluginProcessor to use new components
- Mobile webpage with mute/unmute and level meter implemented
- Simplified approach: using JUCE networking instead of libwebsockets
- Webpage embedded directly in WebServer.cpp

---

*Last Updated: April 2, 2026*
