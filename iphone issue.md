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
