# FAUNA v1.1 Development Plan

## Overview

This branch contains the v1.1 development work for FAUNA audio streaming plugin.

## Features Completed

### 1. Sample Rate Handshake ✅
- **Problem:** Browser's AudioContext may use different sample rate than DAW, causing audio playback issues
- **Solution:** Send DAW sample rate to browser via WebSocket after connection
- **Implementation:**
  - After WebSocket handshake, server sends JSON message with sample rate to browser
  - Browser creates AudioContext at the correct DAW sample rate before starting audio playback
- **Files modified:** `PluginProcessor.cpp`, `WebServer.cpp`

### 2. QR Code Generation ✅
- **Problem:** Users need to manually type the URL to connect their mobile device
- **Solution:** Generate QR code in plugin UI that encodes the connection URL
- **Implementation:**
  - Uses Nayuki QR Code library (QrCode.cpp/hpp)
  - QR code displays in plugin UI with connection URL
  - Scan with phone camera → auto-opens connection page
  - QR only regenerates when URL changes (performance optimization)
- **Files modified:** `PluginEditor.cpp`, `PluginEditor.h`

### 3. Pre-Buffer for Audio Quality ✅
- **Problem:** Android audio had crackling due to WiFi jitter
- **Solution:** Skip first 2 audio callbacks to pre-buffer, start playback on 3rd callback
- **Result:** Smooth audio on Android, no crackling

### 4. Performance & Stability Improvements ✅
- **Thread-safe client counting:** Using `std::atomic<int>` for connected client count
- **Better socket management:** Extract socket handles before releasing lock, cleaner separation
- **Level meter during pre-buffer:** Shows activity before audio starts playing
- **Pre-buffer counter reset:** Properly resets on disconnect/reconnect
- **Files modified:** `WebServer.cpp`, `WebServer.h`, `PluginEditor.cpp`

## Testing Checklist

- [x] Sample rate: Plugin starts, browser connects, audio streams correctly
- [x] Sample rate: Verify browser uses DAW sample rate (not default 44100)
- [x] QR Code: QR code displays correctly in plugin UI
- [x] QR Code: Scanning QR code opens correct URL on mobile
- [x] QR Code: Mobile connects and streams audio after scan
- [x] No crashes when deleting plugin from FL Studio
- [x] Audio quality: Pre-buffer reduces crackling on Android
- [x] Improvements: QR optimization, atomic client count, socket management tested

---

## Known Issues

### iPhone Audio Playback - NOT WORKING ❌

**Issue:** Audio streams to iPhone browser but no sound plays through speakers.

**Symptoms:**
- WebSocket connection succeeds
- Sample rate handshake shows correct value (44100 Hz)
- "Playing!" message appears
- Level meter moves (data IS reaching browser)
- No audio plays through iPhone speakers

**Tested Browsers:**
- Safari - No sound
- Chrome on iOS - No sound (uses WebKit engine like Safari)

**What Works:**
- Android (Chrome, Samsung Internet) - Audio plays correctly
- Level meter proves audio data flows correctly
- Sample rate handshake works
- QR code scanning works

**Attempts to Fix:**
1. `audioCtx.resume()` - Did not fix iPhone audio
2. `src.start(0)` immediate playback - Did not fix iPhone, but BROKE Android audio quality
3. Reverted to scheduled playback - Android audio restored

**Hypothesis:**
- The issue is iOS WebKit-specific
- Audio data reaches the browser JavaScript (level meter proves this)
- iOS may be handling AudioContext output differently
- Possible causes:
  - iOS audio routing (output to wrong destination)
  - AudioContext state management
  - iOS WebKit audio policy restrictions

**Recommended Next Steps:**
1. Try adding `audioCtx.resume()` AND scheduled playback together
2. Investigate iOS audio routing API
3. Add browser console logging to diagnose
4. Research iOS WebKit audio restrictions
5. Consider using Web Audio API worklet for better iOS compatibility

---

## Notes

- All improvements successfully tested and working
- Android audio is solid with current implementation
- iPhone issue documented, needs further investigation
- Design/UI improvements deferred to future version
