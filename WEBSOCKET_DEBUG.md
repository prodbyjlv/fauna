# FAUNA WebSocket Debug Session Log

## Date: April 2, 2026
## Last Commit: 9e16e64 "Add WebSocket debug summary and minimal 101 response fix"

---

## Problem Description

Browser disconnects immediately after server sends 101 Switching Protocols response. Error 1006 (abnormal closure) in browser. Server sees `recv returned 0, WSA error 0` which indicates graceful close from browser side.

---

## DebugView Data Provided by User (Since Last Push)

### Test 1 - Minimal 101 Response (no Sec-WebSocket-Protocol header)
```
FAUNA: Client connected
FAUNA: Full HTTP request:
GET / HTTP/1.1 
Host: 192.168.0.250:8080 
Connection: Upgrade 
Pragma: no-cache 
Cache-Control: no-cache 
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/146.0.0.0 Safari/537.36 
Upgrade: websocket 
Origin: http://192.168.0.250:8080 
Sec-WebSocket-Version: 13 
Accept-Encoding: gzip, deflate 
Accept-Language: en-US,en;q=0.9 
Sec-WebSocket-Key: 6i7E0fW8TerAnPTK8MfgjA== 
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits 

FAUNA: HTTP request received, checking for WebSocket...
FAUNA: Detected WebSocket upgrade request
FAUNA: Parsed WebSocket key: 6i7E0fW8TerAnPTK8MfgjA==
FAUNA: Generated accept key: PjYnDn75TOmOtF3nW42TGIuqe2A=
FAUNA: 101 Switching Protocols response ready to send
FAUNA: Full response:
HTTP/1.1 101 Switching Protocols 
Upgrade: websocket 
Connection: Upgrade 
Sec-WebSocket-Accept: PjYnDn75TOmOtF3nW42TGIuqe2A= 

FAUNA: Sending 101 Switching Protocols response...
FAUNA: 101 Response sent, bytes: 129
FAUNA: Client IP: 192.168.0.250
FAUNA: Client added to list
FAUNA: recv returned 0, WSA error 0
```

### Test 2 - With shutdown(SD_SEND) added
Same result - recv returned 0 immediately after handshake.

### Test 3 - With handleWebSocketClient() called immediately
```
FAUNA: Client added to list
FAUNA: About to call handleWebSocketClient
FAUNA: recv returned 0, WSA error 0
FAUNA: handleWebSocketClient returned
FAUNA: recv returned 0, WSA error 0
```

### Test 4 - Without shutdown call, with handleWebSocketClient
```
FAUNA: Client added to list
FAUNA: About to call handleWebSocketClient
FAUNA: recv returned 0, WSA error 0
FAUNA: handleWebSocketClient returned
FAUNA: recv returned 0, WSA error 0
```

---

## Changes Made Since Last Commit (9e16e64)

### Code Changes to WebServer.cpp

1. **Removed `Sec-WebSocket-Protocol` header** - Simplified to minimal RFC-compliant response:
   ```cpp
   juce::String response = "HTTP/1.1 101 Switching Protocols\r\n";
   response += "Upgrade: websocket\r\n";
   response += "Connection: Upgrade\r\n";
   response += "Sec-WebSocket-Accept: " + acceptKey + "\r\n";
   response += "\r\n";
   ```

2. **Added `shutdown(clientSocket, SD_SEND)`** after sending 101 response - Later removed - didn't help

3. **Added immediate `handleWebSocketClient()` call** after adding client to list - To test if browser sends data immediately - Didn't help

---

## Research Findings

### Stack Overflow Reference
From Chrome WebSocket connection closes immediately (Stack Overflow Question 71342595):
- The issue was server not accepting a protocol when the client sends one
- Solution was to use `request.accept('json', request.origin)` instead of `request.accept(null, request.origin)`
- Chrome gives unhelpful error messages compared to other browsers

### Key Insight
Browser request includes `Sec-WebSocket-Extensions: permessage-deflate` but no `Sec-WebSocket-Protocol`. Our server doesn't send any protocol header in the 101 response. This might be causing Chrome to reject the connection.

---

## Proposed Fixes To Test Next

### Option 1: Add Subprotocol Header
Add `Sec-WebSocket-Protocol: binary` or `Sec-WebSocket-Protocol: base64` header to 101 response.

### Option 2: Verify Accept Key Generation
The Sec-WebSocket-Accept key might be incorrectly generated. Need to verify the SHA-1 + Base64 process.

### Option 3: Check Network Traffic
Use a network capture tool (Wireshark) to see exactly what Chrome is receiving/rejecting.

---

## Current Status

- 101 response is RFC-compliant and minimal
- All extra headers have been removed
- `shutdown()` call was removed (didn't help)
- `handleWebSocketClient()` is called immediately (didn't help)
- Connection still closes immediately after handshake
- Need to investigate subprotocol requirement or network-level issue

---

## Notes

- The "bmRequest" errors in DebugView are unrelated Windows/USB errors, not from our code
- Browser shows error 1006 (abnormal closure)
- Server sends 129 bytes successfully
- No subsequent WebSocket frames received
