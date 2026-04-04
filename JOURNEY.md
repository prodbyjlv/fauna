# FAUNA - Journey from Concept to v1.1

## Overview

FAUNA is a VST3 audio plugin that streams DAW audio to mobile devices over WiFi.

---

## Version History

### master - Initial Working Version
- Basic VST3 plugin with audio streaming
- WebSocket server for audio transmission
- Mobile web UI for audio control

### v1.1 - iPhone + Android Support
**Major improvements:**
- Fixed audio streaming to work on Android devices
- Fixed iPhone audio routing issue (speakers not working)
- Implemented ring buffer for smooth audio playback
- Added sample rate resampling (FL Studio 44100Hz → Device 48000Hz)
- Implemented prebuffering to prevent crackling

---

## Technical Challenges Solved

### 1. WebSocket Connection
**Problem:** Browser disconnected immediately after connection (error 1006)

**Solution:** 
- Fixed SHA-1 implementation with correct padding
- Fixed base64 encoding
- Used proper string handling

### 2. Android Audio Pitch Shift
**Problem:** Audio played at wrong pitch/speed (8.8% faster)

**Cause:** Sample rate mismatch (FL Studio 44100Hz vs Android 48000Hz)

**Solution:** Linear interpolation resampler

### 3. Audio Crackling
**Problem:** Audio had crackling/popping sounds

**Solution:** 
- Ring buffer (192000 floats = 2 seconds)
- Prebuffering (4096 frames before playback)

### 4. iPhone Audio Not Playing Through Speakers
**Problem:** Audio streamed but no sound from speakers (only Bluetooth worked)

**Root Cause:** iOS WebAudio defaults to "ambient" audio session (respects mute switch, routes to earpiece)

**Solution:** 
```javascript
navigator.audioSession.type = 'playback';
```
This tells iOS to treat the web app as a media app, routing audio to speakers.

---

## Files Structure

```
Source/
├── PluginProcessor.cpp/h   - Main audio processing
├── PluginEditor.cpp/h      - Plugin UI
├── WebServer.cpp/h         - HTTP/WebSocket server + mobile HTML
├── AudioStreamer.cpp/h    - Audio streaming logic
└── QrCode.cpp/h           - QR code generation

web/
└── index.html             - Standalone mobile UI
```

---

## Key Technologies

- **VST3** - Audio plugin format
- **WebSocket** - Real-time audio streaming
- **Web Audio API** - Browser audio playback
- **JUCE** - C++ audio framework
- **Audio Session API** - iOS audio routing control

---

## Credits

Built by Joshua | prodbyjlv@gmail.com
