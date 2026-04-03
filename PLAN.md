# FAUNA v1.1 Development Plan

## Overview

This branch contains the v1.1 development work for FAUNA audio streaming plugin.

## Features

### 1. Sample Rate Handshake
- **Problem:** Browser's AudioContext may use different sample rate than DAW, causing audio playback issues
- **Solution:** Send DAW sample rate to browser via WebSocket after connection
- **Implementation:**
  - After WebSocket handshake, server sends JSON message with sample rate to browser
  - Browser creates AudioContext at the correct DAW sample rate before starting audio playback
- **Files to modify:** `PluginProcessor.cpp`, `WebServer.cpp`, embedded HTML

### 2. QR Code Generation
- **Problem:** Users need to manually type the URL to connect their mobile device
- **Solution:** Generate QR code in plugin UI that encodes the connection URL
- **Implementation:**
  - Add QR code generation library
  - Display QR code in plugin UI alongside the URL
  - Scan with phone camera → auto-opens connection page
- **Files to modify:** `PluginEditor.cpp`, `PluginEditor.h`, possibly add QR library

## Workflow

1. Implement Feature 1 (Sample Rate Handshake)
2. Test thoroughly in FL Studio
3. Implement Feature 2 (QR Code)
4. Test thoroughly in FL Studio
5. Push to GitHub when both features are verified working

## Testing Checklist

- [ ] Sample rate: Plugin starts, browser connects, audio streams correctly
- [ ] Sample rate: Verify browser uses DAW sample rate (not default 44100)
- [ ] QR Code: QR code displays correctly in plugin UI
- [ ] QR Code: Scanning QR code opens correct URL on mobile
- [ ] QR Code: Mobile connects and streams audio after scan
- [ ] No crashes when deleting plugin from FL Studio

## Notes

- Design/UI improvements are deferred to a future version
- Focus is on making features work flawlessly before polish
