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
