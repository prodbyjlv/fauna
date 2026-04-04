# FAUNA Project Workflow

## Overview
This document outlines the workflow for building and deploying the FAUNA VST3 audio plugin.

---

## Team Roles

| Task | Responsible |
|------|-------------|
| Making code changes | AI Assistant |
| Rebuilding in Visual Studio | Joshua (User) |
| Copying build to plugin folder | AI Assistant |
| Pushing to GitHub | AI Assistant (only when requested) |

---

## Build & Deploy Workflow

### Step 1: Code Changes
- AI Assistant makes changes to source files
- Files are modified in local project: `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA`

### Step 2: Rebuild
- User rebuilds the project in Visual Studio
- Build output location: `Builds\VisualStudio2026\x64\Debug\VST3\`

### Step 3: Copy to Plugin Folder
- AI Assistant deletes old plugin: `C:\Program Files\Common Files\VST3\FAUNA.vst3`
- AI Assistant copies new build **exactly as-is**

### Step 4: Test
- User tests plugin in FL Studio
- User reports back to AI Assistant

### Step 5: GitHub (when ready)
- User requests push to GitHub
- AI Assistant pushes to v1.1 branch

---

## CRITICAL: VST3 Folder Structure

**DO NOT MODIFY THE BUILD OUTPUT STRUCTURE**

The VST3 folder has a nested structure that MUST be preserved:

```
FAUNA.vst3/
└── Contents/
    ├── Resources/
    │   └── moduleinfo.json
    └── x86_64-win/
        └── FAUNA.dll
```

The `x86_64-win` folder inside `Contents` is CORRECT and required by the VST3 specification.

**NEVER** try to "flatten" or "clean up" this structure.

---

## Past Mistakes

### Mistake 1: Broken Mobile Page
- **Problem:** JavaScript functions weren't properly closed, causing buttons to not work
- **Lesson:** Every function in embedded HTML must have its own closing brace

### Mistake 2: Server Dying in FL Studio
- **Problem:** Server was started in `prepareToPlay()` which FL calls repeatedly
- **Lesson:** Start server in constructor, keep it running

### Mistake 3: Incorrect VST3 Structure
- **Problem:** User manually restructured the VST3 folder, breaking the plugin
- **Lesson:** Always copy the build output exactly as VS creates it

---

## File Locations

| File | Path |
|------|------|
| Project | `C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA` |
| JUCE | `C:\JLV JUCE\juce-8.0.12-windows\JUCE` |
| Plugin | `C:\Program Files\Common Files\VST3\FAUNA.vst3` |
| GitHub | `https://github.com/prodbyjlv/fauna` (branch: v1.1) |

---

## Quick Reference

To copy plugin after rebuild:
```powershell
# Delete old
Remove-Item -Recurse "C:\Program Files\Common Files\VST3\FAUNA.vst3"

# Copy new (exactly as-is)
Copy-Item -Recurse "Builds\VisualStudio2026\x64\Debug\VST3\FAUNA.vst3" "C:\Program Files\Common Files\VST3\"
```
