# FAUNA WebSocket Debug Summary

## Problem
Browser disconnects immediately after server sends 101 Switching Protocols response. Error 1006 (abnormal closure) in browser.

## Debug Information

### HTTP Request from Browser
```
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
Sec-WebSocket-Key: [key] 
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits 
```

### Server Response (101 Switching Protocols)
```
HTTP/1.1 101 Switching Protocols 
Upgrade: websocket 
Connection: Upgrade 
Sec-WebSocket-Accept: [accept key]
```

### Behavior Observed
- Server sends 156-188 bytes successfully
- Client added to list
- `recv returned 0, WSA error 0` - graceful close from browser side
- No subsequent WebSocket frames received to server

## Fixes Attempted

| # | Fix Attempted | Reasoning | Result |
|---|---------------|-----------|--------|
| 1 | Added `Sec-WebSocket-Protocol: binary` header | Browser might expect subprotocol | Failed |
| 2 | Changed to `Sec-WebSocket-Protocol: base64` header | Different subprotocol | Failed |
| 3 | Removed Sec-WebSocket-Version header | Minimal RFC-compliant response | Failed |
| 4 | Removed all extra headers | Minimal response | Testing in progress |

## Previous Fixes (from earlier sessions)

1. **Pong response fix** - Ping handler incorrectly extracted payload from wrong offset
2. **Mask bit fix** - Server was setting mask bit (0x80) on outgoing frames - servers must NOT mask per RFC 6455

## Current Status
Testing minimal 101 response with only required headers (Upgrade, Connection, Sec-WebSocket-Accept).

## Notes
- The 101 response is being sent correctly according to RFC 6455
- The browser accepts the response (no HTTP error codes)
- Browser immediately closes connection after receiving 101
- This is unusual - typically if the accept key was wrong, browser would return HTTP 400 error
- The issue may be related to the WebSocket frame format or timing after handshake
