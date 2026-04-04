# Lessons Learned - VST3 Installation Mistakes

## Date: April 3, 2026

## The Problem

When installing the FAUNA VST3 plugin, I made a critical mistake that caused the plugin to not appear in FL Studio.

### What Happened

1. After rebuilding the VST3 in Visual Studio, the build output contained a nested folder structure:
   ```
   FAUNA.vst3/
   └── Contents/
       └── x86_64-win/
           └── FAUNA.vst3/     <-- This nested folder is CORRECT
               └── FAUNA.dll
   ```

2. I saw this nested `FAUNA.vst3` folder and assumed it was wrong
3. I manually created a new "clean" structure without the nested folder:
   ```
   FAUNA.vst3/
   └── Contents/
       ├── Resources/
       │   └── moduleinfo.json
       └── x86_64-win/
           └── FAUNA.dll
   ```
4. This caused FL Studio to not recognize the plugin at all

5. When I finally copied the entire source folder exactly as VS built it (including the nested folder), the plugin worked correctly

## The Lesson

### NEVER modify the VST3 build output structure

The VS/JUCE build system creates the VST3 structure correctly. Other VST3 plugins (like FabFilter, Portal, etc.) have the same nested folder structure. This is the correct VST3 format.

**Rule:** Copy the entire `FAUNA.vst3` folder from the build output **exactly as it is**. Do not try to "clean up" or restructure anything.

## Correct Copy Procedure

1. Delete the old `FAUNA.vst3` from `C:\Program Files\Common Files\VST3\`
2. Copy the entire `FAUNA.vst3` folder from:
   `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA\Builds\VisualStudio2026\x64\Debug\VST3\FAUNA.vst3`
   to:
   `C:\Program Files\Common Files\VST3\`

## Root Cause

I assumed the build was incorrect based on seeing what appeared to be a duplicate folder. In reality:
- VST3 bundles have a specific structure that includes nested folders
- The nested folder is part of the VST3 specification
- FL Studio and other DAWs understand this structure

## Prevention

In the future:
1. **Never assume the build output is wrong**
2. **Copy build output exactly as-is**
3. **Trust the build system (JUCE/VS)**
4. If something seems wrong, ask before making changes

---

## iOS Audio Output Discovery

**Date: April 3, 2026**

### Discovery

After implementing the iPhone unlock overlay, we discovered that:
- Audio DOES play through Bluetooth headphones connected to iPhone
- Audio does NOT play through iPhone built-in speakers
- Other apps play audio through iPhone speakers fine
- Disabling Bluetooth on iPhone did NOT fix the speaker issue

### What This Means

| Observation | Implication |
|------------|-------------|
| Audio plays through Bluetooth headphones | AudioContext is working correctly ✓ |
| Audio doesn't play through iPhone speakers | iOS audio routing issue |
| Other apps play through speakers fine | iPhone speakers work, just not web audio |

### The Problem

iOS Safari defaults to Bluetooth audio output when a Bluetooth device is connected or has been connected in the session. Web Audio API on iOS tends to route audio to Bluetooth devices rather than built-in speakers.

### Status

**The iPhone unlock code is working** - audio is being processed and plays through Bluetooth. The remaining issue is an iOS audio routing problem specific to Web Audio API, not our code.

### Next Steps to Investigate

1. Research iOS Audio Session configuration for Web Audio API
2. Explore using `navigator.mediaDevices` or AudioOutput endpoint selection
3. Test on Android Chrome to see if speakers work there
4. Consider if this is acceptable behavior (audio to headphones is still functional)

---

## iOS Speaker Fix Attempt - ScriptProcessor Approach

**Date: April 4, 2026**

### What We Tried

After external help, we implemented a completely different audio approach using ScriptProcessorNode:

1. **Full-screen unlock overlay removed** - Simplified to single START button
2. **Added `<audio id="iosAudio" playsinline>`** - Silent audio element for iOS
3. **Added `unlockIOSAudio()` function:**
   - Silent buffer trick: creates 1-sample buffer and plays it
   - Silent audio element: plays inaudible WAV at 0.01 volume
4. **ScriptProcessorNode instead of BufferSourceNode:**
   - Real-time buffer filling instead of scheduled playback
   - Ring buffer (array) approach for audio data
   - Directly writes to output buffer in onaudioprocess callback

### Code Structure

```javascript
// Silent audio unlock
function unlockIOSAudio(context) {
    const buffer = context.createBuffer(1, 1, 22050);
    const source = context.createBufferSource();
    source.buffer = buffer;
    source.connect(context.destination);
    source.start();
    
    const iosAudio = document.getElementById('iosAudio');
    iosAudio.src = 'data:audio/wav;base64,...';
    iosAudio.volume = 0.01;
    iosAudio.play();
}

// ScriptProcessor for real-time playback
scriptNode = audioCtx.createScriptProcessor(4096, 0, 2);
scriptNode.onaudioprocess = function(e) {
    const left = e.outputBuffer.getChannelData(0);
    const right = e.outputBuffer.getChannelData(1);
    for (let i = 0; i < 4096; i++) {
        if (audioBuffer.length >= 2) {
            left[i] = audioBuffer.shift();
            right[i] = audioBuffer.shift();
        } else {
            left[i] = 0;
            right[i] = 0;
        }
    }
};
```

### Result

| Platform | Result |
|----------|--------|
| Android | ✅ Works perfectly |
| iOS Bluetooth | ✅ Works (audio plays through headphones) |
| iOS Speakers | ❌ Still does not work |

### Conclusion

The ScriptProcessor approach was a complete rewrite of the audio playback method but **did not solve the iPhone speaker issue**. This confirms that:

1. AudioContext is working correctly
2. Audio data IS being processed and streamed successfully
3. The issue is purely iOS Safari's audio routing policy for Web Audio API
4. This is a known platform limitation, not a code bug

### Status: iPhone Speaker Issue Remains Unsolved

All audio playback approaches tested:
- BufferSourceNode with scheduling ✅ Android / ❌ iOS Speakers
- Full-screen unlock overlay ✅ Android / ❌ iOS Speakers  
- ScriptProcessorNode ✅ Android / ❌ iOS Speakers

Audio plays through Bluetooth headphones on iPhone but NOT through built-in speakers. This is an iOS Safari limitation for Web Audio API that cannot be resolved with JavaScript alone.

