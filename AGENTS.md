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
- **Format for Streaming:** Opus codec (low bandwidth, browser-native support)

### Networking
- **Protocol:** WebSocket for real-time audio streaming
- **Port:** 8080 (default, configurable)
- **URL Format:** `http://192.168.x.x:8080` (auto-detected local IP)
- **Max Devices:** 2 simultaneous connections

### Web Interface
- **Mute Control:** Per-device mute state
- **Level Meter:** Visual audio level indicator
- **Browser Support:** Chrome, Safari, Firefox (mobile)
- **Connection:** Local network only (no cloud)

---

## Implementation Status

### Phase 1: Project Setup ✅
- [x] JUCE project initialized
- [x] Visual Studio build configured
- [x] VST3 plugin format configured
- [x] GitHub repository created
- [x] Initial build successful

### Phase 2: Audio Engine
- [ ] Create AudioStreamer class
- [ ] Capture audio via processBlock()
- [ ] Implement circular buffer
- [ ] Set up audio format conversion

### Phase 3: Opus Encoding
- [ ] Integrate libopus
- [ ] Encode PCM to Opus in real-time
- [ ] Handle encoding thread

### Phase 4: Web Server
- [ ] Implement HTTP server (use libwebsockets)
- [ ] Serve mobile webpage
- [ ] WebSocket for audio streaming
- [ ] REST API for mute control

### Phase 5: Plugin UI
- [ ] Display local IP address
- [ ] Generate QR code
- [ ] Show connection status
- [ ] Basic controls

### Phase 6: Mobile Webpage
- [ ] HTML5 audio playback
- [ ] Mute/unmute buttons
- [ ] Level meter visualization
- [ ] Responsive mobile design

### Phase 7: Multi-Device Support
- [ ] Handle 2 simultaneous WebSocket connections
- [ ] Per-device mute state management

---

## Project Structure

```
FAUNA/
├── FAUNA.jucer              # JUCE project file
├── Source/
│   ├── PluginProcessor.cpp  # Audio engine
│   ├── PluginProcessor.h
│   ├── PluginEditor.cpp      # Plugin UI
│   └── PluginEditor.h
├── Builds/
│   └── VisualStudio2026/     # Visual Studio project
├── web/                      # Mobile webpage (to be created)
│   ├── index.html
│   ├── style.css
│   └── client.js
├── libraries/                # Third-party libraries (to be created)
│   └── libwebsockets/
└── AGENTS.md                # This file
```

---

## Third-Party Libraries

| Library | Purpose | License |
|---------|---------|---------|
| JUCE 8.0.12 | Audio plugin framework | GPL/Commercial |
| libwebsockets | WebSocket server | MIT |
| Opus codec | Audio encoding | BSD |
| libqrencode | QR code generation | LGPL |

---

## Design Preferences

- **Initial UI:** Basic/minimal
- **Level Meter:** Yes, to be implemented
- **Mobile Page:** Simple mute/unmute with level meter
- **Colors:** To be decided later

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
1. Integrate libwebsockets library
2. Build basic audio capture in processBlock()
3. Create simple HTTP server
4. Test audio streaming locally
5. Build mobile webpage

---

## Conversation History

### Session 1 (April 2, 2026)
- Set up JUCE project from scratch
- Configured VST3 build in Visual Studio
- Resolved JUCE module path issues
- Successfully built empty VST3 plugin
- Pushed project to GitHub
- Created this AGENTS.md file

---

*Last Updated: April 2, 2026*
