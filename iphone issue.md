# iPhone Audio Issue - April 4, 2026

## Problem Summary

Audio streams to iPhone browser but no sound plays through speakers.
- Level meter moves (data IS reaching browser)
- Audio plays through Bluetooth headphones
- Audio does NOT play through iPhone built-in speakers
- Android works perfectly

---

## Consultant's Technical Analysis

**Consultant identified two specific issues:**

### Issue 1: Corrupt WAV File (Mute Switch Bypass)

**Problem:** The iOS Web Audio API (AudioContext) is tied to the iPhone's "Ringer" session by default. If the physical switch on the side of the iPhone is set to "Silent" (showing orange), iOS will mute the JavaScript audio stream.

**Solution:** Playing an HTML5 `<audio>` element forces iOS to switch from "Ringer" session to "Media" session (which ignores the mute switch).

**Flaw Found:** The base64 WAV string was only 26 bytes. A valid WAV file header requires a minimum of 44 bytes. Because the file was corrupt, Safari's media player silently rejected it, the `play()` promise failed, and the session never switched to "Media".

### Issue 2: WebKit ScriptProcessor Optimization Bug

**Problem:** In `startAudio()`, the ScriptProcessor was initialized with 0 input channels:
```javascript
scriptNode = audioCtx.createScriptProcessor(4096, 0, 2);
```

**Flaw Found:** iOS Safari aggressively optimizes its audio processing graph to save battery. If it sees 0 inputs, WebKit considers the node a "dead end" and completely omits it from the audio graph, meaning `onaudioprocess` never fires.

---

## Code Changes Made

### File: Source/WebServer.cpp

#### Change 1: Valid 44-byte WAV + Loop
```javascript
// BEFORE (corrupt 26-byte WAV, no loop):
function unlockIOSAudio(ctx){
    var b=ctx.createBuffer(1,1,22050);
    var s=ctx.createBufferSource();
    s.buffer=b;s.connect(ctx.destination);s.start(0);
    var a=document.getElementById('iosAudio');
    a.src='data:audio/wav;base64,UklGRiQAAABXQVZFZm10IBAAAAABAAEA';
    a.volume=0.001;a.play().catch(function(){});
}

// AFTER (valid 44-byte WAV, loop enabled):
function unlockIOSAudio(ctx){
    var a=document.getElementById('iosAudio');
    a.src='data:audio/wav;base64,UklGRigAAABXQVZFZm10IBIAAAABAAEARKwAAIhYAQACABAAAABkYXRhAgAAAAEA';
    a.loop=true;a.volume=0.001;a.play().catch(function(e){console.log('Unlock failed',e);});
}
```

#### Change 2: 1 Dummy Input Channel
```javascript
// BEFORE:
scriptNode=audioCtx.createScriptProcessor(4096,0,2);

// AFTER:
scriptNode=audioCtx.createScriptProcessor(4096,1,2);
```

#### Change 3: Proper AudioContext Resume
```javascript
// BEFORE:
audioCtx.resume().then(function(){unlockIOSAudio(audioCtx);})
             .catch(function(){unlockIOSAudio(audioCtx);});

// AFTER:
if(audioCtx.state==='suspended'){audioCtx.resume().then(function(){unlockIOSAudio(audioCtx);})}
else{unlockIOSAudio(audioCtx);}
```

---

## Why This Should Have Worked

1. **Valid 44-byte WAV** - Forces iOS to switch from "Ringer" to "Media" session, bypassing the mute switch
2. **1 dummy input channel** - Prevents WebKit from optimizing out the ScriptProcessor node
3. **Loop enabled** - Keeps the audio element playing continuously, maintaining the Media session
4. **Proper resume** - Ensures AudioContext is in a running state before playback

This is described as "industry-standard solution" for iOS Web Audio API issues.

---

## Result

**The fix was implemented and tested - it did NOT solve the iPhone speaker issue.**

### What Still Happens:
- Audio streams to iPhone (level meter works)
- Audio plays through Bluetooth headphones
- Audio does NOT play through iPhone built-in speakers

### Conclusion:
The consultant's hypothesis about the mute switch and WebKit optimization was reasonable and well-reasoned, but did not resolve the core issue. The problem appears to be deeper in iOS audio routing for Web Audio API.

---

## Next Steps to Investigate

1. Test with physical iPhone mute switch in different positions
2. Check if iOS audio session category needs explicit configuration
3. Investigate `navigator.mediaDevices` or AudioOutput endpoint selection
4. Consider if audio output destination can be forced to speakers
5. Test on multiple iPhone models and iOS versions
6. Compare with working web audio apps to understand the difference

---

## Second Fix Attempt: "Prime the Pump" - April 4, 2026

### Consultant's New Analysis

After the first fix attempt failed, the consultant identified a deeper issue:

#### The "Empty Pipe" Optimization

**Problem:** While having 1 input channel prevents the node from being immediately deleted, Safari's CoreAudio engine goes one step further to save battery. It looks at the Web Audio Graph and says: "Okay, this node has an input pipe. But is anything actually flowing into it?"

Because no source was connected to the scriptNode, Safari sees an **empty pipe**. It assumes that if there is zero input, the output must also be zero, so it completely **suspends the onaudioprocess loop** to save CPU.

#### Why Bluetooth Worked

When Bluetooth connects, iOS suddenly switches audio routes from the internal speaker to A2DP. When this happens, iOS completely **destroys and rebuilds the CoreAudio graph** at the hardware level. This hardware-level rebuild forces Safari to drop its power-saving optimizations momentarily, accidentally "waking up" the script node.

### The New Fix: "Prime the Pump"

To stop Safari from shutting down the ScriptProcessorNode on the internal speaker, we must give it **fake audio to process**. We need to create an invisible, silent OscillatorNode and plug it directly into the ScriptProcessorNode.

This forces continuous data through the pipe, tricking Safari into keeping the audio thread awake on the internal speaker.

### Code Change

Added three lines directly under where scriptNode is created:

```javascript
scriptNode=audioCtx.createScriptProcessor(4096,1,2);

// THE FINAL IOS FIX: "PRIME THE PUMP"
// Create a silent dummy oscillator and plug it into the script node.
// This forces Safari's battery optimizer to keep the process loop awake!
var dummyOsc=audioCtx.createOscillator();
dummyOsc.connect(scriptNode);dummyOsc.start(0);
```

### Logic and Reasoning

1. **Previous fix (1 input channel)** - Prevents immediate deletion of the node, but Safari still sees an empty pipe
2. **Dummy oscillator** - Provides actual audio data flowing through the pipe
3. **Continuous flow** - Forces Safari to keep the audio thread awake on the internal speaker
4. **Theoretical guarantee** - If data is flowing, Safari cannot assume output is zero

The consultant was confident this would work because:
- The symptoms (Bluetooth works, Speaker is dead) perfectly isolate the issue to Safari's internal power-state management
- The `<audio>` base64 trick bypasses the physical hardware mute switch
- This dummyOsc trick bypasses the software CPU optimization
- Once the valve is forced open, the WebSockets buffer should finally stream out of the bottom speaker

---

## Result

**The fix was implemented and tested - it did NOT solve the iPhone speaker issue.**

### What Still Happens:
- Audio streams to iPhone (level meter works)
- Audio plays through Bluetooth headphones
- Audio does NOT play through iPhone built-in speakers
- No change in behavior from the previous attempt

### Conclusion:
Despite the logical reasoning behind the "Prime the Pump" fix, the issue persists. The problem appears to be deeper in iOS audio routing for Web Audio API than initially understood. Both hardware-level (mute switch) and software-level (CPU optimization) workarounds have been attempted without success.

---

## Current Status (April 4, 2026)

All attempts so far:
| Attempt | Description | Result |
|---------|-------------|--------|
| 1 | Original ScriptProcessor approach | ❌ iPhone speakers don't work |
| 2 | Valid 44-byte WAV + 1 input channel | ❌ iPhone speakers don't work |
| 3 | "Prime the Pump" - dummy oscillator | ❌ iPhone speakers don't work |

**Audio works through Bluetooth on iPhone, but NOT through built-in speakers.**

---

## THE SOLUTION: Audio Session API - April 4, 2026

### The Fix That Worked!

After all previous attempts failed, additional research revealed the **root cause** of the iPhone audio issue:

### Root Cause

iOS WebAudio by default uses an **"ambient"** audio session type, which:
- Respects the physical mute switch
- Treats audio as background/notification sound
- Routes audio to the earpiece instead of speakers

Native apps like Spotify, YouTube use **"playback"** session type, which ignores the mute switch and routes to speakers.

### The Solution

**One line of code:**

```javascript
navigator.audioSession.type = 'playback';
```

This tells iOS to treat the web app as a media app, not a notification sound.

### Code Change

Added in the `startAudio()` function, before AudioContext creation:

```javascript
function startAudio(){
    if(started)return;
    started=true;
    
    // ADD THIS LINE FIRST
    if(navigator.audioSession){
        navigator.audioSession.type = 'playback';
    }
    
    // ... rest of function
}
```

### Why This Worked

1. **Directly addresses the root cause** - The audio session type was the fundamental issue
2. **Single line** - Simple, clean solution
3. **iOS 17+ support** - The Audio Session API is supported in iOS 17+ Safari
4. **No workaround needed** - Works without the WAV trick, dummy oscillator, etc.

### Sources

- W3C Audio Session API Specification: https://w3c.github.io/audio-session/
- MDN Documentation: https://developer.mozilla.org/en-US/docs/Web/API/Audio_Session_API
- Stack Overflow: https://stackoverflow.com/a/79789727

---

## Final Result

| Attempt | Solution | Result |
|---------|----------|--------|
| 1 | Original ScriptProcessor approach | ❌ Failed |
| 2 | Valid 44-byte WAV + 1 input channel | ❌ Failed |
| 3 | "Prime the Pump" - dummy oscillator | ❌ Failed |
| **4** | **Audio Session API** | **✅ SUCCESS!** |

### What Now Works:
- ✅ Audio plays through iPhone speakers
- ✅ Audio plays regardless of mute switch position
- ✅ Audio streams correctly
- ✅ Level meter responds
- ✅ Works on iOS 17+ Safari

**iPhone audio streaming is now fully functional! 🎉**
