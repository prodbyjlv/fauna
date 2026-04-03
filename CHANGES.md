# FAUNA - Current State Changelog

## Branch: current-state
Last updated: April 3, 2026

---

## Session: April 3, 2026 - Debugging UI Not Showing + No Audio Streaming

### Issues Identified
1. **Plugin crashed on deletion** - `processBlock` could access server during teardown
2. **No audio streaming to mobile** - WebSocket connected but no audio received
3. **Sample rate mismatch** - DAW sample rate not communicated to browser
4. **UI labels overlapping** - status/devices labels drawn on top of QR code box
5. **UI not showing** - `isShuttingDown` flag was set in `releaseResources()` but never reset in `prepareToPlay()`, causing timer callback to return early

### Changes Made

#### Source/PluginProcessor.cpp
- Added `isShuttingDown = false` reset in `prepareToPlay()` - fixes UI not showing
- Added `isShuttingDown` guard at top of `processBlock` - prevents crash on deletion
- Added `isShuttingDown = true` in `releaseResources()` - safe shutdown before destructor
- Removed excessive `OutputDebugString` from `processBlock` (was blocking audio thread)

#### Source/WebServer.h
- Added `sampleRateSent` flag to `AudioClient` struct
- Added `setSampleRate(double sr)` method
- Added `sendSampleRateMessage(AudioClient&)` declaration
- Added `sampleRate` member variable (default 44100.0)
- Added `audioFrameCount` counter for periodic debug logging
- Changed `int64_t` to `__int64` for Windows compatibility
- Changed `JUCE_LEAK_DETECTOR` to `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`

#### Source/WebServer.cpp
- Added `sendSampleRateMessage()` - sends `{"type":"init","sampleRate":xxx}` as text frame
- Modified `broadcastAudio()` - sends init message first, skips audio that callback, sends audio next callback
- Added `SO_RCVTIMEO` (5s) to client socket in `handleClient` - prevents blocking server thread
- Added debug logging for `recv()` failures in `handleClient`
- Removed excessive `OutputDebugString` from `broadcastAudio` and `sendWebSocketFrame` (was blocking audio thread)
- Updated embedded HTML to parse sample rate init message and create AudioContext at correct rate
- Added "Sample Rate" display field to HTML

#### Source/PluginEditor.cpp
- Moved `statusLabel` and `devicesLabel` below QR code box (y=310, y=330)
- Moved `urlLabel` to y=350
- Moved `sampleRateLabel`, `bufferSizeLabel`, `portLabel` to y=380
- Increased window height from 420 to 440
- Added debug logging to constructor, `paint()`, and `timerCallback()`
- Added `OutputDebugString` to trace UI lifecycle

### Current Status
- **Crash fix**: Implemented (isShuttingDown guard in processBlock + releaseResources)
- **UI visibility**: Debugging (timer callback may be hitting early return due to isShuttingDown)
- **Audio streaming**: Debugging (need to verify WebSocket handshake completes on mobile)
- **Next steps**: Test with DebugView to trace timer callback behavior

---

## Previous Sessions (on master branch)

### April 2, 2026 - Session 1: WebSocket Connection Fixed
- Fixed SHA-1 implementation padding
- Fixed base64Encode padding logic
- Fixed JUCE string lifetime issues in generateWebSocketKey

### April 2, 2026 - Session 2: Audio Streaming Implemented
- Added audio interleaving in PluginProcessor
- Implemented broadcastAudio() in WebServer
- Added embedded HTML page with audio streaming UI
