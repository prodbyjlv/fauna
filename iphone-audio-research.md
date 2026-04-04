# iPhone Audio Issue - Comprehensive Research Document

**Date:** April 4, 2026  
**Project:** FAUNA Audio Streaming Plugin  
**Issue:** Audio streams to iPhone but does not play through speakers (only Bluetooth)

---

## Table of Contents

1. [Problem Summary](#1-problem-summary)
2. [What We've Tried (Failed Attempts)](#2-what-weve-tried-failed-attempts)
3. [Root Cause Analysis](#3-root-cause-analysis)
4. [Solutions Found in Research](#4-solutions-found-in-research)
5. [Recommended Implementation Plan](#5-recommended-implementation-plan)
6. [Testing Protocol](#6-testing-protocol)
7. [Resources and References](#7-resources-and-references)

---

## 1. Problem Summary

### Symptoms
- Audio streams to iPhone browser successfully
- Level meter responds to audio (data IS reaching the browser)
- Audio plays through Bluetooth headphones
- Audio does NOT play through iPhone built-in speakers
- Android works perfectly

### What This Tells Us
- WebSocket connection works
- Audio data transmission works
- iOS Safari's audio routing is the issue
- AudioContext is processing data (proven by level meter)
- The problem is at the OUTPUT stage (audio routing)

### Environment
- **Tested Browsers:** Safari, Chrome on iOS (both use WebKit)
- **iOS Version:** Assumed iOS 17+ (latest)
- **Hardware:** iPhone built-in speakers vs Bluetooth headphones

---

## 2. What We've Tried (Failed Attempts)

### Attempt 1: Original ScriptProcessor Approach
**Date:** Earlier in development  
**What was done:**
- Basic ScriptProcessor implementation with ring buffer
- Real-time buffer filling via onaudioprocess callback

**Result:** ❌ Failed - No audio through iPhone speakers

---

### Attempt 2: Valid 44-byte WAV + 1 Input Channel
**Date:** April 4, 2026  
**Consultant Analysis:**
- The base64 WAV string was only 26 bytes (corrupt)
- Valid WAV header requires minimum 44 bytes
- iOS Safari silently rejected the corrupt file, play() failed
- ScriptProcessor with 0 input channels causes WebKit optimization

**What was changed:**

```javascript
// BEFORE (corrupt WAV, 0 input channels):
a.src='data:audio/wav;base64,UklGRiQAAABXQVZFZm10IBAAAAABAAEA'; // 26 bytes
scriptNode = audioCtx.createScriptProcessor(4096, 0, 2);

// AFTER (valid WAV, 1 input channel):
a.src='data:audio/wav;base64,UklGRigAAABXQVZFZm10IBIAAAABAAEARKwAAIhYAQACABAAAABkYXRhAgAAAAEA'; // 44 bytes
scriptNode = audioCtx.createScriptProcessor(4096, 1, 2);
a.loop = true;
```

**Result:** ❌ Failed - Still no audio through speakers

---

### Attempt 3: "Prime the Pump" (Dummy Oscillator)
**Date:** April 4, 2026  
**Consultant Analysis:**
- Having 1 input channel prevents immediate deletion
- But Safari sees an "empty pipe" with no data flowing
- Safari assumes "no input = zero output" and suspends onaudioprocess
- Dummy oscillator provides continuous data through the pipe

**What was changed:**

```javascript
scriptNode = audioCtx.createScriptProcessor(4096, 1, 2);

// THE FINAL IOS FIX: "PRIME THE PUMP"
// Create a silent dummy oscillator and plug it into the script node.
// This forces Safari's battery optimizer to keep the process loop awake!
var dummyOsc = audioCtx.createOscillator();
dummyOsc.connect(scriptNode);
dummyOsc.start(0);
```

**Result:** ❌ Failed - Still no audio through speakers

---

### Why Bluetooth Worked (When It Did)

When Bluetooth headphones connect, iOS rebuilds the CoreAudio graph at the hardware level. This reconstruction forces Safari to momentarily drop its power-saving optimizations, accidentally "waking up" the audio routing to the correct output device.

---

## 3. Root Cause Analysis

### The Core Issue

**iOS WebAudio is connected to the Ringer/Audio Session, not just volume control.**

By default, iOS uses an **"ambient"** audio session type, which:
- Respects the physical mute switch
- Treats audio like background/notification sounds
- Routes audio based on hardware state

### Key Discovery (October 2025 - Stack Overflow)

> *"Since iOS 17, you can set the audio session type to 'playback'. Add `navigator.audioSession.type = 'playback'` and audio will not be suspended. By default the type is 'ambient' and so audio will be muted if the phone is muted."*

**Source:** https://stackoverflow.com/a/79789727

### Why This Matters

1. **"ambient" session (default):** Respects mute switch, treats as non-essential audio
2. **"playback" session:** Ignores mute switch, treats as essential media app
3. iOS native apps use AVAudioSession with "playback" category
4. Web apps couldn't previously access this until Audio Session API was introduced

### Audio Session Types (W3C Standard)

| Type | Description | Mute Switch Respected? |
|------|-------------|------------------------|
| `"auto"` | Default, browser decides | Depends on context |
| `"playback"` | Media playback (music/video) | **No** |
| `"transient"` | Short sounds (notifications) | Yes |
| `"transient-solo"` | Exclusive audio (voice prompts) | Yes |
| `"ambient"` | Background/ambient sounds | **Yes** |
| `"play-and-record"` | Both play and record (calls) | No |

---

## 4. Solutions Found in Research

### Solution 1: Audio Session API (RECOMMENDED)

**What it is:** A W3C standard API that allows web apps to control audio session behavior on iOS.

**Why it should work:** Setting `type = "playback"` tells iOS this is a media app, which:
- Ignores the physical mute switch
- Treats audio like a native media player
- Routes audio to the correct output device (speakers)

**Implementation:**

```javascript
// Add this ONE LINE in startAudio() function, before creating AudioContext
navigator.audioSession.type = "playback";
```

**Where to add in current code:**

```javascript
function startAudio(){
    if(started)return;
    started=true;
    
    // ADD THIS LINE FIRST
    if(navigator.audioSession){
        navigator.audioSession.type = "playback";
    }
    
    // ... rest of function
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    // ...
}
```

**Requirements:**
- iOS 17+ Safari (API support)
- Works on Chrome iOS (uses WebKit engine)
- Single line of code

**Success Rate:** Unknown (not yet tested - this is the new approach)

**Sources:**
- W3C Specification: https://w3c.github.io/audio-session/
- MDN Documentation: https://developer.mozilla.org/en-US/docs/Web/API/Audio_Session_API
- Stack Overflow: https://stackoverflow.com/a/79789727

---

### Solution 2: Dual Unlock (WebAudio + HTML5 Audio)

**What it is:** Playing BOTH a WebAudio buffer source AND an HTML5 `<audio>` element simultaneously.

**Why it should work:** iOS requires BOTH audio types to be playing to fully unlock audio and route it correctly to speakers.

**Implementation pattern:**

```javascript
function unlockAudio(ctx){
    // WebAudio unlock - play silent buffer
    var buffer = ctx.createBuffer(1, 1, 22050);
    var source = ctx.createBufferSource();
    source.buffer = buffer;
    source.connect(ctx.destination);
    source.start();
    
    // HTML5 audio unlock - play silent MP3
    var audio = document.getElementById('iosAudio');
    audio.src = 'data:audio/mp3;base64,//MkxAAHiAICWAB...'; // silent mp3
    audio.loop = true;
    audio.volume = 0.01;
    audio.play().catch(function(){});
}
```

**Why this works:**
- HTML5 `<audio>` forces Media session (ignores mute switch)
- WebAudio provides processing pipeline
- Both must be active to route correctly

**Success Rate:** High (documented in multiple sources)

**Source:** https://github.com/swevans/unmute

---

### Solution 3: Media Session API + ios-audio-routing Library

**What it is:** A library that forces iOS to treat web app like a native media app for audio routing purposes.

**Features:**
- Registers with Control Center
- Forces proper audio routing to speakers
- Handles background/foreground transitions
- Works on iOS 15+

**Implementation:**

```javascript
import { initIOSAudioSession, wakeUp, setPlaying } from 'ios-audio-routing';

// Initialize on first user interaction
button.addEventListener('click', async () => {
    await initIOSAudioSession({
        title: 'FAUNA',
        artist: 'Audio Streaming'
    });
});

// Before playing audio
await wakeUp();
setPlaying();
```

**Success Rate:** Unknown (newer approach)

**Library:** https://github.com/vinsidious/ios-audio-routing

---

### Solution 4: Reconnection Trick (WebRTC Workaround)

**What it is:** Disconnecting and reconnecting audio can force iOS to rebuild the CoreAudio graph.

**Implementation (complex):**
1. Start audio connection
2. Wait for audio track
3. Hang up/disconnect
4. Reconnect

**Why it works:** When iOS rebuilds the CoreAudio graph, it momentarily drops power-saving optimizations.

**Not recommended** - complex, unreliable, poor UX

**Source:** WebKit Bug #196539 (https://bugs.webkit.org/show_bug.cgi?id=196539)

---

## 5. Recommended Implementation Plan

### Phase 1: Audio Session API (Try First)
**Complexity:** Easy (1-2 lines of code)  
**Risk:** Low (non-breaking change)  
**iOS Version:** iOS 17+

**Steps:**
1. Add `navigator.audioSession.type = "playback"` before AudioContext creation
2. Rebuild plugin
3. Test on iPhone with mute switch in BOTH positions
4. If works, done! If not, proceed to Phase 2

---

### Phase 2: If Phase 1 Fails - Dual Unlock
**Complexity:** Medium (add HTML5 audio alongside WebAudio)  
**Risk:** Low (additive change)  
**iOS Version:** iOS 15+

**Steps:**
1. Keep Audio Session API code from Phase 1
2. Add HTML5 `<audio>` element with silent MP3
3. Play both WebAudio buffer AND HTML5 audio on unlock
4. Test on iPhone
5. If works, done! If not, proceed to Phase 3

---

### Phase 3: If Both Fail - Full Media Session Integration
**Complexity:** High (library integration)  
**Risk:** Medium (requires new dependencies)  
**iOS Version:** iOS 15+

**Steps:**
1. Integrate ios-audio-routing library
2. Add proper Media Session API handlers
3. Implement full unlock sequence
4. Test on iPhone

---

## 6. Testing Protocol

### Before Each Test

1. **Delete old plugin** from `C:\Program Files\Common Files\VST3\`
2. **Copy new build** from Visual Studio output
3. **Restart DAW** (FL Studio, etc.)
4. **Load FAUNA** on a track
5. **Play audio** in DAW

### Test Matrix

| Test # | Mute Switch Position | Output Device | Expected Result |
|--------|---------------------|---------------|----------------|
| 1 | Unmuted (switch up) | Speakers | Audio plays |
| 2 | Unmuted (switch up) | Bluetooth | Audio plays |
| 3 | Muted (switch down) | Speakers | Audio plays (with fixes) |
| 4 | Muted (switch down) | Bluetooth | Audio plays |

### What to Look For

**Success indicators:**
- Audio plays through iPhone speakers
- Audio plays regardless of mute switch position
- Level meter responds to audio
- Audio continues playing smoothly

**Failure indicators:**
- Level meter works but no sound from speakers
- Audio only plays through Bluetooth
- Audio stops when mute switch is flipped

### Debugging

Add these console.log statements to track what's happening:

```javascript
console.log('AudioSession supported:', !!navigator.audioSession);
if(navigator.audioSession){
    console.log('AudioSession type:', navigator.audioSession.type);
}
console.log('AudioContext state:', audioCtx.state);
console.log('Playing from speakers...');
```

---

## 7. Resources and References

### W3C Specifications
- **Audio Session API:** https://w3c.github.io/audio-session/
- **Audio Session Type Property:** https://developer.mozilla.org/en-US/docs/Web/API/AudioSession/type

### MDN Documentation
- **Audio Session API:** https://developer.mozilla.org/en-US/docs/Web/API/Audio_Session_API
- **AudioContext:** https://developer.mozilla.org/en-US/docs/Web/API/AudioContext
- **Navigator.audioSession:** https://developer.mozilla.org/en-US/docs/Web/API/Navigator/audioSession

### Stack Overflow Discussions
- **"No sound on iOS only (Web Audio API)":** https://stackoverflow.com/questions/76291413/no-sound-on-ios-only-web-audio-api
- **"IOS WebAudio only works on headphones":** https://stackoverflow.com/questions/21122418/ios-webaudio-only-works-on-headphones
- **Audio Session Answer (Oct 2025):** https://stackoverflow.com/a/79789727

### Open Source Solutions
- **ios-audio-routing (vinsidious):** https://github.com/vinsidious/ios-audio-routing
- **unmute (swevans):** https://github.com/swevans/unmute

### WebKit Bug Reports
- **Bug #196539 (iOS 12.2 speaker issue):** https://bugs.webkit.org/show_bug.cgi?id=196539
  - Status: **RESOLVED (2024)**
- **Bug #230902 (WebRTC volume issue):** https://bugs.webkit.org/show_bug.cgi?id=230902
  - Status: **RESOLVED**

### Apple Developer Documentation
- **AVAudioSession:** https://developer.apple.com/documentation/avfaudio/avaudiosession
- **Audio Session Categories:** https://developer.apple.com/documentation/audiotoolbox/audio-session-categories

---

## Appendix: Code Comparison

### Current Code Structure (startAudio function)

```javascript
function startAudio(){
    if(started)return;
    started=true;
    
    // ... UI updates ...
    
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    
    // Existing unlock (needs enhancement)
    unlockIOSAudio(audioCtx);
    
    scriptNode = audioCtx.createScriptProcessor(4096, 1, 2);
    
    // Dummy oscillator (Prime the Pump - still didn't work)
    var dummyOsc = audioCtx.createOscillator();
    dummyOsc.connect(scriptNode);
    dummyOsc.start(0);
    
    scriptNode.onaudioprocess = function(e){
        // ... ring buffer handling ...
    };
    
    scriptNode.connect(audioCtx.destination);
    
    // WebSocket connection...
}
```

### Proposed Enhancement (Audio Session API)

```javascript
function startAudio(){
    if(started)return;
    started=true;
    
    // ... UI updates ...
    
    // NEW: Set Audio Session to "playback" type
    if(navigator.audioSession){
        navigator.audioSession.type = "playback";
        console.log("Audio session set to playback");
    }
    
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    
    // Existing unlock (keep for now)
    unlockIOSAudio(audioCtx);
    
    scriptNode = audioCtx.createScriptProcessor(4096, 1, 2);
    
    // Dummy oscillator (keep for now)
    var dummyOsc = audioCtx.createOscillator();
    dummyOsc.connect(scriptNode);
    dummyOsc.start(0);
    
    scriptNode.onaudioprocess = function(e){
        // ... ring buffer handling ...
    };
    
    scriptNode.connect(audioCtx.destination);
    
    // WebSocket connection...
}
```

---

## Summary

| Attempt | Solution | Complexity | Status |
|---------|----------|------------|--------|
| 1 | ScriptProcessor alone | Easy | ❌ Failed |
| 2 | Valid WAV + 1 input | Easy | ❌ Failed |
| 3 | Prime the Pump | Easy | ❌ Failed |
| **4** | **Audio Session API** | **Easy** | **NOT YET TESTED** |
| 5 | Dual Unlock | Medium | Not yet tried |
| 6 | ios-audio-routing | High | Not yet tried |

---

**Document created:** April 4, 2026  
**Last updated:** April 4, 2026  
**Version:** 1.0
