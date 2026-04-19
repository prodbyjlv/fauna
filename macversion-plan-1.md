# FAUNA Mac Version Plan

## Overview

This document outlines the plan to create a macOS version of the FAUNA audio streaming plugin, using Audio Unit (AU) format.

---

## Phase 1: Cross-Platform Code Changes

The WebServer currently uses Windows-only networking and threading APIs. This must be rewritten to work on both Windows and macOS.

### Step 1.1 - Modify WebServer.h for cross-platform

Replace Windows headers with conditional compilation:

```
Current (Windows only):
#include <winsock2.h>
#include <ws2tcpip.h>

New approach:
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    // macOS: Use POSIX sockets
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif
```

Create platform-agnostic type definitions:
- Define INVALID_SOCKET as -1 on non-Windows
- Define SOCKET as int on non-Windows

### Step 1.2 - Modify WebServer.cpp for cross-platform

Replace Windows-specific function calls:

| Windows Function | macOS Replacement |
|---|---|
| WSAStartup(MAKEWORD(2, 2), &wsaData) | Remove (not needed) |
| WSACleanup() | Remove (not needed) |
| CreateThread(NULL, 0, func, this, 0, NULL) | new juce::Thread() or std::thread |
| Sleep(milliseconds) | juce::Thread::sleep() or usleep() |
| closesocket(sock) | close(sock) |
| INVALID_SOCKET | -1 (define if not exists) |
| SOCKET | int |
| SOCKADDR_IN | struct sockaddr_in |

### Step 1.3 - Add conditional compilation

Wrap platform-specific code:

```cpp
#ifdef _WIN32
    // Windows socket code
    SOCKET serverSocket = createServerSocket(port);
#else
    // POSIX/macOS socket code
    int serverSocket = createServerSocket(port);
#endif
```

### Step 1.4 - Test Windows still works

After changes, verify Windows VST3 build still compiles and runs correctly.

---

## Phase 2: Project Config Changes

### Step 2.1 - Update FAUNA.jucer

Find the pluginFormats line and add Audio Unit:

```
Current:
pluginFormats="buildVST3"

New:
pluginFormats="buildVST3 buildAU"
```

### Step 2.2 - Add macOS exporter target

Ensure the jucer file has a macOS exporter configured:
- Target: macOS (AU, VST3)
- Architecture: Universal (arm64, x86_64)

### Step 2.3 - Verify build settings

Check:
- Bundle identifier is set correctly
- Plugin name matches Windows version
- Audio Unit component type and subtype are correct

---

## Phase 3: Build Automation

### Step 3.1 - Create GitHub Actions workflow

Create new file: `.github/workflows/build-macos.yml`

This workflow will:
- Trigger on push to mv1 branch
- Run on macOS latest
- Checkout code
- Build using JUCE command line or Projucer
- Produce FAUNA.component (Audio Unit)
- Upload as artifact

### Step 3.2 - Configure build matrix

Build multiple formats:
- Audio Unit (.component)
- Optional: VST3 for macOS

### Step 3.3 - Test workflow

Verify the workflow runs successfully on GitHub Actions.

### Step 3.4 - Set up release artifact

Configure workflow to upload build as release asset when a tag is pushed.

---

## Phase 4: Documentation

### Step 4.1 - Update README.md

Add macOS section with:

**macOS Installation**

1. Download FAUNA-macOS.zip
2. Extract the FAUNA.component file
3. Copy to ~/Library/Audio/Plug-Ins/Components/
   - Note: You may need to create this folder if it doesn't exist
4. Open your DAW (Logic Pro, Ableton Live, etc.)
5. Rescan for plugins
6. FAUNA should appear in your plugin list

### Step 4.2 - Create macOS firewall PDF

Create a PDF document covering:

**macOS Firewall and Network Access**

- The FAUNA plugin runs inside your DAW process
- macOS Firewall works on a per-application basis
- Your DAW (Logic, Ableton, etc.) needs permission if prompted
- The plugin itself does NOT need firewall exceptions

**How to check:**

1. Go to System Settings → Security & Privacy → Firewall
2. Click Firewall Options
3. Check if your DAW is allowed to accept incoming connections
4. If not, add it to the list

**Note:** Most DAWs already have this permission by default.

### Step 4.3 - Update Windows firewall PDF (if needed)

Review existing Windows firewall PDF:
- Ensure instructions are still accurate
- Verify port 8080 reference is correct

### Step 4.4 - Create macOS zip package

Include in FAUNA-macOS.zip:
- FAUNA.component (Audio Unit file)
- README-macOS.txt (quick start)
- Firewall-instructions.pdf

### Step 4.5 - Create Windows zip updates (if needed)

If any changes to Windows version, rebuild Windows zip with updated docs.

---

## Order of Execution

1. Phase 1: Rewrite WebServer for cross-platform (Steps 1.1-1.4)
2. Phase 2: Update FAUNA.jucer (Steps 2.1-2.3)
3. Phase 3: Create GitHub Actions workflow (Steps 3.1-3.4)
4. Phase 4: Update documentation (Steps 4.1-4.5)

---

## Testing Checklist

- [ ] Windows VST3 still builds and functions
- [ ] macOS AU builds successfully
- [ ] AU installs correctly on macOS
- [ ] Audio streams to browser on macOS
- [ ] Mobile web UI works on iOS Safari
- [ ] GitHub Actions workflow passes
- [ ] Documentation is clear and accurate