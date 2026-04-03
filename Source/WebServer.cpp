#include "WebServer.h"
#include <cstring>

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void sha1(const char* data, int len, unsigned char* out)
{
    unsigned int h0=0x67452301,h1=0xEFCDAB89,h2=0x98BADCFE,h3=0x10325476,h4=0xC3D2E1F0;
    int totalLen=len+1+8;
    if(totalLen%64!=0) totalLen=((totalLen/64)+1)*64;
    juce::HeapBlock<unsigned char> buf(totalLen,true);
    std::memcpy(buf.get(),data,len);
    buf[len]=0x80;
    unsigned long long bits=(unsigned long long)len*8;
    for(int i=7;i>=0;i--){buf[totalLen-8+i]=(unsigned char)(bits&0xFF);bits>>=8;}
    for(int i=0;i<totalLen;i+=64){
        unsigned int w[80];
        for(int j=0;j<16;j++) w[j]=((unsigned int)buf[i+j*4]<<24)|((unsigned int)buf[i+j*4+1]<<16)|((unsigned int)buf[i+j*4+2]<<8)|((unsigned int)buf[i+j*4+3]);
        for(int j=16;j<80;j++){unsigned int v=w[j-3]^w[j-8]^w[j-14]^w[j-16];w[j]=(v<<1)|(v>>31);}
        unsigned int a=h0,b=h1,c=h2,d=h3,e=h4;
        for(int j= 0;j<20;j++){unsigned int f=(b&c)|((~b)&d),t=((a<<5)|(a>>27))+f+e+0x5A827999+w[j];e=d;d=c;c=(b<<30)|(b>>2);b=a;a=t;}
        for(int j=20;j<40;j++){unsigned int f=b^c^d,        t=((a<<5)|(a>>27))+f+e+0x6ED9EBA1+w[j];e=d;d=c;c=(b<<30)|(b>>2);b=a;a=t;}
        for(int j=40;j<60;j++){unsigned int f=(b&c)|(b&d)|(c&d),t=((a<<5)|(a>>27))+f+e+0x8F1BBCDC+w[j];e=d;d=c;c=(b<<30)|(b>>2);b=a;a=t;}
        for(int j=60;j<80;j++){unsigned int f=b^c^d,        t=((a<<5)|(a>>27))+f+e+0xCA62C1D6+w[j];e=d;d=c;c=(b<<30)|(b>>2);b=a;a=t;}
        h0+=a;h1+=b;h2+=c;h3+=d;h4+=e;
    }
    out[0]=(h0>>24)&0xFF;out[1]=(h0>>16)&0xFF;out[2]=(h0>>8)&0xFF;out[3]=h0&0xFF;
    out[4]=(h1>>24)&0xFF;out[5]=(h1>>16)&0xFF;out[6]=(h1>>8)&0xFF;out[7]=h1&0xFF;
    out[8]=(h2>>24)&0xFF;out[9]=(h2>>16)&0xFF;out[10]=(h2>>8)&0xFF;out[11]=h2&0xFF;
    out[12]=(h3>>24)&0xFF;out[13]=(h3>>16)&0xFF;out[14]=(h3>>8)&0xFF;out[15]=h3&0xFF;
    out[16]=(h4>>24)&0xFF;out[17]=(h4>>16)&0xFF;out[18]=(h4>>8)&0xFF;out[19]=h4&0xFF;
}

static juce::String base64Encode(const unsigned char* data, int len)
{
    juce::String result; int i=0;
    while(i<len){
        int rem=len-i,a=data[i++],b=(rem>1)?data[i++]:0,c=(rem>2)?data[i++]:0;
        result+=base64_chars[a>>2];
        result+=base64_chars[((a&3)<<4)|(b>>4)];
        result+=(rem>1)?base64_chars[((b&15)<<2)|(c>>6)]:'=';
        result+=(rem>2)?base64_chars[c&63]:'=';
    }
    return result;
}

HTTPServer::HTTPServer()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2),&wsaData);
    localIP="127.0.0.1";
}

HTTPServer::~HTTPServer() { stop(); WSACleanup(); }

void HTTPServer::detectLocalIP()
{
    juce::Array<juce::IPAddress> addresses;
    juce::IPAddress::findAllAddresses(addresses);
    juce::String result="127.0.0.1";
    for(int i=0;i<addresses.size();i++){
        juce::String ip=addresses[i].toString();
        if(ip.startsWith("192.168.")||ip.startsWith("10.")||ip.startsWith("172.")){result=ip;break;}
    }
    localIP=result;
}

bool HTTPServer::start(int targetPort)
{
    if(running) return true;
    port=targetPort;
    serverSocket=createServerSocket(port);
    if(serverSocket==INVALID_SOCKET){OutputDebugString("FAUNA: Failed to create socket\n");return false;}
    running=true;
    detectLocalIP();
    serverThreadHandle=CreateThread(NULL,0,serverThreadFunc,this,0,NULL);
    if(serverThreadHandle==NULL){running=false;closesocket(serverSocket);serverSocket=INVALID_SOCKET;return false;}
    OutputDebugString(("FAUNA: Server on "+localIP+":"+juce::String(port)+"\n").toUTF8());
    return true;
}

void HTTPServer::stop()
{
    running=false;
    if(serverSocket!=INVALID_SOCKET){shutdown(serverSocket,SD_BOTH);closesocket(serverSocket);serverSocket=INVALID_SOCKET;}
    if(serverThreadHandle!=NULL){
        if(WaitForSingleObject(serverThreadHandle,2000)==WAIT_TIMEOUT) TerminateThread(serverThreadHandle,0);
        CloseHandle(serverThreadHandle); serverThreadHandle=NULL;
    }
    juce::ScopedLock lock(clientsLock);
    for(auto& c:audioClients) if(c.socket!=INVALID_SOCKET){shutdown(c.socket,SD_BOTH);closesocket(c.socket);c.socket=INVALID_SOCKET;}
    audioClients.clear();
    connectedClientCount.store(0);
}

DWORD WINAPI HTTPServer::serverThreadFunc(LPVOID p){return ((HTTPServer*)p)->serverThread();}

DWORD HTTPServer::serverThread()
{
    DWORD timeout=100;
    setsockopt(serverSocket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));

    while(running && serverSocket!=INVALID_SOCKET)
    {
        sockaddr_in ca; int cl=sizeof(ca);
        SOCKET cs=accept(serverSocket,(sockaddr*)&ca,&cl);
        if(cs!=INVALID_SOCKET) handleClient(cs);

        // Copy socket handles out before releasing lock — never hold lock during recv()
        juce::Array<SOCKET> socketsToCheck;
        {
            juce::ScopedLock lock(clientsLock);
            for(int i=0;i<audioClients.size();++i)
                socketsToCheck.add(audioClients.getReference(i).socket);
        }

        for(int i=0;i<socketsToCheck.size();++i)
        {
            SOCKET s=socketsToCheck[i];
            if(s==INVALID_SOCKET) continue;

            fd_set rs; FD_ZERO(&rs); FD_SET(s,&rs);
            TIMEVAL tv={0,0};
            int r=select(0,&rs,nullptr,nullptr,&tv);

            if(r>0)
            {
                // Re-acquire lock only to find the client, then handle it
                juce::ScopedLock lock(clientsLock);
                for(int j=audioClients.size()-1;j>=0;--j)
                {
                    AudioClient& c=audioClients.getReference(j);
                    if(c.socket==s)
                    {
                        handleWebSocketClient(c);
                        break;
                    }
                }
            }
            else if(r<0)
            {
                juce::ScopedLock lock(clientsLock);
                for(int j=audioClients.size()-1;j>=0;--j)
                {
                    if(audioClients.getReference(j).socket==s)
                    {
                        audioClients.remove(j);
                        connectedClientCount.store(audioClients.size());
                        break;
                    }
                }
            }
        }

        // Clean up any sockets marked invalid during handling
        {
            juce::ScopedLock lock(clientsLock);
            for(int i=audioClients.size()-1;i>=0;--i)
                if(audioClients.getReference(i).socket==INVALID_SOCKET)
                    audioClients.remove(i);
            connectedClientCount.store(audioClients.size());
        }
    }
    return 0;
}

SOCKET HTTPServer::createServerSocket(int p)
{
    SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s==INVALID_SOCKET) return INVALID_SOCKET;
    int opt=1; setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt,sizeof(opt));
    sockaddr_in addr; addr.sin_family=AF_INET; addr.sin_addr.s_addr=INADDR_ANY; addr.sin_port=htons((u_short)p);
    if(bind(s,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR){closesocket(s);return INVALID_SOCKET;}
    if(listen(s,5)==SOCKET_ERROR){closesocket(s);return INVALID_SOCKET;}
    return s;
}

void HTTPServer::handleClient(SOCKET clientSocket)
{
    char buffer[8192];
    int bytes=recv(clientSocket,buffer,sizeof(buffer)-1,0);
    if(bytes<=0){closesocket(clientSocket);return;}
    buffer[bytes]='\0';
    juce::String request(buffer);

    if(request.contains("Upgrade")&&request.contains("websocket"))
    {
        {
            juce::ScopedLock lock(clientsLock);
            if(audioClients.size()>=2){closesocket(clientSocket);return;}
        }
        const char* kp=strstr(buffer,"Sec-WebSocket-Key:");
        juce::String wsKey="dGhlIHNhbXBsZSBub25jZQ==";
        if(kp){kp+=18;while(*kp==' ')kp++;const char* ep=strstr(kp,"\r\n");if(ep&&ep>kp)wsKey=juce::String(kp,(int)(ep-kp));}

        std::string combined=wsKey.toStdString()+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        unsigned char hash[20]; sha1(combined.c_str(),(int)combined.size(),hash);
        juce::String acceptKey=base64Encode(hash,20);

        juce::String resp="HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+acceptKey+"\r\n\r\n";
        send(clientSocket,resp.toUTF8(),resp.length(),0);

        u_long nb=1; ioctlsocket(clientSocket,FIONBIO,&nb);
        int sb=256*1024; setsockopt(clientSocket,SOL_SOCKET,SO_SNDBUF,(const char*)&sb,sizeof(sb));

        AudioClient client;
        client.socket=clientSocket; client.isWebSocket=true;
        client.muted=false; client.lastPingTime=0;
        client.sampleRateSent=false;

        sockaddr_in addr; int al=sizeof(addr);
        if(getpeername(clientSocket,(sockaddr*)&addr,&al)==0){
            char ip[INET_ADDRSTRLEN]; inet_ntop(AF_INET,&addr.sin_addr,ip,INET_ADDRSTRLEN);
            client.ipAddress=ip;
        }
        juce::ScopedLock lock(clientsLock);
        audioClients.add(client);
        connectedClientCount.store(audioClients.size());
        OutputDebugString(("FAUNA: Client connected. Total: "+juce::String(audioClients.size())+"\n").toUTF8());
    }
    else
    {
        juce::String r=buildHTTPResponse(request);
        send(clientSocket,r.toUTF8(),r.length(),0);
        closesocket(clientSocket);
    }
}

void HTTPServer::handleWebSocketClient(AudioClient& client)
{
    char buffer[8192];
    int bytes=recv(client.socket,buffer,sizeof(buffer),0);
    if(bytes==0||(bytes<0&&WSAGetLastError()!=WSAEWOULDBLOCK)){
        closesocket(client.socket); client.socket=INVALID_SOCKET; return;
    }
    if(bytes<2) return;
    unsigned char opcode=buffer[0]&0x0F;
    bool masked=(buffer[1]&0x80)!=0;
    unsigned int plen=buffer[1]&0x7F;
    int hlen=2; if(plen==126)hlen+=2; if(plen==127)hlen+=8; if(masked)hlen+=4;

    if(opcode==0x01&&bytes>=hlen){
        char* d=buffer+hlen; int dl=bytes-hlen;
        if(masked&&dl>0){char* mk=buffer+hlen-4;for(int i=0;i<dl;i++)d[i]^=mk[i%4];}
        juce::String msg(d,dl);
        if(msg.contains("mute")) client.muted=msg.contains("true");
    }
    else if(opcode==0x08){sendWebSocketFrame(client.socket,nullptr,0,0x08);closesocket(client.socket);client.socket=INVALID_SOCKET;}
    else if(opcode==0x09){
        char pp[125]; int pl=0;
        if(plen>0&&plen<=125&&bytes>hlen){pl=(int)plen;memcpy(pp,buffer+hlen,pl);if(masked){char* mk=buffer+hlen-4;for(int i=0;i<pl;i++)pp[i]^=mk[i%4];}}
        sendWebSocketFrame(client.socket,pl>0?pp:nullptr,pl,0x0A);
    }
}

void HTTPServer::sendSampleRateMessage(AudioClient& client)
{
    int sr=(int)sampleRate;
    juce::String msg="{\"type\":\"init\",\"sampleRate\":"+juce::String(sr)+"}";
    sendWebSocketFrame(client.socket,msg.toUTF8(),msg.length(),0x01);
    OutputDebugString(("FAUNA: Init sent, sampleRate="+juce::String(sr)+" Hz\n").toUTF8());
}

void HTTPServer::broadcastAudio(const float* audioData, int numSamples)
{
    int numFloats=numSamples*2;
    int byteCount=numFloats*(int)sizeof(float);

    juce::ScopedLock lock(clientsLock);
    if(audioClients.size()==0) return;

    for(int i=audioClients.size()-1;i>=0;i--)
    {
        AudioClient& client=audioClients.getReference(i);
        if(client.socket==INVALID_SOCKET){audioClients.remove(i);connectedClientCount.store(audioClients.size());continue;}
        if(!client.isWebSocket) continue;

        if(!client.sampleRateSent)
        {
            sendSampleRateMessage(client);
            client.sampleRateSent=true;
            continue;
        }

        if(client.muted)
        {
            juce::HeapBlock<float> silence(numFloats,true);
            sendWebSocketFrame(client.socket,(const char*)silence.get(),byteCount,0x02);
        }
        else
        {
            sendWebSocketFrame(client.socket,(const char*)audioData,byteCount,0x02);
        }
    }
}

void HTTPServer::writeAudioData(const float* audioData, int numSamples)
{
    broadcastAudio(audioData,numSamples);
}

void HTTPServer::sendWebSocketFrame(SOCKET socket, const char* data, int length, int opcode)
{
    if(socket==INVALID_SOCKET) return;

    int hlen=2;
    if(length>=126&&length<65536) hlen=4;
    else if(length>=65536)        hlen=10;

    juce::HeapBlock<char> frame(hlen+length);
    frame[0]=(char)(0x80|(opcode&0x0F));

    if(length<126)
    {
        frame[1]=(char)(length&0x7F);
    }
    else if(length<65536)
    {
        frame[1]=126;
        frame[2]=(char)((length>>8)&0xFF);
        frame[3]=(char)(length&0xFF);
    }
    else
    {
        frame[1]=127;
        frame[2]=0;
        frame[3]=0;
        frame[4]=0;
        frame[5]=0;
        frame[6]=(char)((length>>24)&0xFF);
        frame[7]=(char)((length>>16)&0xFF);
        frame[8]=(char)((length>>8) &0xFF);
        frame[9]=(char)(length      &0xFF);
    }

    if(length>0&&data!=nullptr) memcpy(frame.get()+hlen,data,length);
    send(socket,frame.get(),hlen+length,0);
}

juce::String HTTPServer::generateWebSocketKey(const char* key)
{
    std::string combined=std::string(key)+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char hash[20]; sha1(combined.c_str(),(int)combined.size(),hash);
    return base64Encode(hash,20);
}

juce::String HTTPServer::getHTMLPage()
{
    juce::String html="<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">";
    html+="<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    html+="<title>FAUNA Audio Stream</title><style>";
    html+="*{margin:0;padding:0;box-sizing:border-box}";
    html+="body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#1a1a2e 0%,#16213e 100%);min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center;color:white;padding:20px}";
    html+=".container{background:rgba(255,255,255,0.1);border-radius:20px;padding:40px;text-align:center;backdrop-filter:blur(10px);max-width:400px;width:100%}";
    html+="h1{font-size:2em;margin-bottom:10px;color:#00d4ff}";
    html+=".status{font-size:0.9em;color:#aaa;margin-bottom:30px}";
    html+=".level-meter{width:100%;height:30px;background:rgba(0,0,0,0.3);border-radius:15px;overflow:hidden;margin-bottom:30px}";
    html+=".level-bar{height:100%;width:0%;background:linear-gradient(90deg,#00ff88,#00d4ff,#ffaa00,#ff4444);transition:width 0.08s;border-radius:15px}";
    html+=".mute-btn{padding:20px 60px;font-size:1.5em;border:none;border-radius:15px;cursor:pointer;transition:all 0.3s;font-weight:bold}";
    html+=".mute-btn.muted{background:#ff4444;color:white}.mute-btn.unmuted{background:#00ff88;color:#1a1a2e}";
    html+=".mute-btn:hover{transform:scale(1.05)}";
    html+=".connection-status{margin-top:20px;font-size:0.8em;color:#888}";
    html+=".connected{color:#00ff88}";
    html+=".info-box{background:rgba(0,0,0,0.2);padding:15px;border-radius:10px;margin-bottom:20px}";
    html+=".info-box p{margin:5px 0}";
    html+=".start-btn{padding:15px 40px;font-size:1.2em;background:#00d4ff;color:#1a1a2e;border:none;border-radius:10px;cursor:pointer;margin-bottom:20px;transition:all 0.3s}";
    html+=".start-btn:hover{transform:scale(1.05)}.start-btn:disabled{opacity:0.5;cursor:not-allowed;transform:none}";
    html+="</style></head><body><div class=\"container\">";
    html+="<h1>FAUNA</h1><p class=\"status\">Audio Streaming Control</p>";
    html+="<div class=\"info-box\">";
    html+="<p><strong>Server:</strong> <span id=\"serverStatus\">Checking...</span></p>";
    html+="<p><strong>Devices:</strong> <span id=\"deviceCount\">0</span></p>";
    html+="<p><strong>Audio:</strong> <span id=\"audioStatus\">Click Start to begin</span></p>";
    html+="<p><strong>Sample Rate:</strong> <span id=\"srStatus\">-</span></p>";
    html+="</div>";
    html+="<button class=\"start-btn\" id=\"startBtn\" onclick=\"startAudio()\">START AUDIO</button>";
    html+="<div class=\"level-meter\"><div class=\"level-bar\" id=\"levelBar\"></div></div>";
    html+="<button class=\"mute-btn unmuted\" id=\"muteBtn\" onclick=\"toggleMute()\">UNMUTED</button>";
    html+="<p class=\"connection-status\" id=\"status\">Ready</p>";
    html+="</div><script>";

    html+="var isMuted=false,ws=null,audioCtx=null,started=false;";
    html+="var nextPlayTime=0,SAMPLE_RATE=44100,initReceived=false,audioBufferCount=0;";

    html+="function startAudio(){";
    html+="if(started)return;started=true;";
    html+="document.getElementById('startBtn').textContent='Connecting...';";
    html+="document.getElementById('startBtn').disabled=true;";
    html+="document.getElementById('audioStatus').textContent='Waiting for DAW...';";
    html+="connectWS();";
    html+="}";

    html+="function connectWS(){";
    html+="ws=new WebSocket('ws://'+location.host);";
    html+="ws.binaryType='arraybuffer';";
    html+="ws.onopen=function(){document.getElementById('status').textContent='Connected';};";
    html+="ws.onclose=function(e){";
    html+="document.getElementById('audioStatus').textContent='Disconnected ('+e.code+')';";
    html+="document.getElementById('audioStatus').style.color='#ff4444';";
    html+="document.getElementById('startBtn').disabled=false;";
    html+="document.getElementById('startBtn').textContent='RECONNECT';";
    html+="started=false;initReceived=false;audioBufferCount=0;";
    html+="setTimeout(function(){if(!started){started=true;connectWS();}},3000);";
    html+="};";
    html+="ws.onerror=function(){document.getElementById('audioStatus').textContent='Connection error';};";

    html+="ws.onmessage=function(e){";

    html+="if(typeof e.data==='string'){";
    html+="try{var msg=JSON.parse(e.data);";
    html+="if(msg.type==='init'&&msg.sampleRate){";
    html+="SAMPLE_RATE=msg.sampleRate;";
    html+="document.getElementById('srStatus').textContent=SAMPLE_RATE+' Hz';";
    html+="document.getElementById('srStatus').className='connected';";
    html+="audioCtx=new(window.AudioContext||window.webkitAudioContext)({sampleRate:SAMPLE_RATE});";
    html+="nextPlayTime=audioCtx.currentTime+0.1;";
    html+="initReceived=true;";
    html+="document.getElementById('audioStatus').textContent='Playing!';";
    html+="document.getElementById('audioStatus').className='connected';";
    html+="document.getElementById('startBtn').textContent='Streaming...';";
    html+="}";
    html+="}catch(err){}";
    html+="return;";
    html+="}";

    html+="if(!(e.data instanceof ArrayBuffer)||!initReceived||!audioCtx)return;";
    html+="audioBufferCount++;";
    html+="var floats=new Float32Array(e.data);";
    html+="var numFrames=floats.length/2;if(numFrames<1)return;";
    html+="var abuf=audioCtx.createBuffer(2,numFrames,SAMPLE_RATE);";
    html+="var L=abuf.getChannelData(0),R=abuf.getChannelData(1);";
    html+="for(var i=0;i<numFrames;i++){L[i]=floats[i*2];R[i]=floats[i*2+1];}";
    html+="var mx=0;for(var i=0;i<Math.min(200,L.length);i++){var av=Math.abs(L[i]);if(av>mx)mx=av;}";
    html+="document.getElementById('levelBar').style.width=(mx*100)+'%';";
    html+="if(audioBufferCount<3)return;";
    html+="if(nextPlayTime<audioCtx.currentTime+0.04)nextPlayTime=audioCtx.currentTime+0.04;";
    html+="var src=audioCtx.createBufferSource();";
    html+="src.buffer=abuf;src.connect(audioCtx.destination);src.start(nextPlayTime);";
    html+="nextPlayTime+=abuf.duration;";
    html+="};";
    html+="}";

    html+="function toggleMute(){";
    html+="isMuted=!isMuted;";
    html+="var btn=document.getElementById('muteBtn');";
    html+="if(isMuted){btn.textContent='MUTED';btn.className='mute-btn muted';}";
    html+="else{btn.textContent='UNMUTED';btn.className='mute-btn unmuted';}";
    html+="document.getElementById('status').textContent=isMuted?'Muted':'Unmuted';";
    html+="if(ws&&ws.readyState===WebSocket.OPEN)ws.send(isMuted?'mute:true':'mute:false');";
    html+="}";

    html+="function updateStatus(){";
    html+="fetch('/status').then(function(r){return r.json();})";
    html+=".then(function(d){";
    html+="document.getElementById('serverStatus').textContent='Running';";
    html+="document.getElementById('serverStatus').className='connected';";
    html+="document.getElementById('deviceCount').textContent=d.clients;";
    html+="}).catch(function(){";
    html+="document.getElementById('serverStatus').textContent='Not Running';";
    html+="document.getElementById('serverStatus').style.color='#ff4444';";
    html+="});}";
    html+="updateStatus();setInterval(updateStatus,2000);";
    html+="</script></body></html>";
    return html;
}

juce::String HTTPServer::buildHTTPResponse(const juce::String& request)
{
    if(request.startsWith("GET / ")||request.startsWith("GET /index.html")||
       request.startsWith("GET / HTTP")||request=="GET /")
    {
        juce::String body=getHTMLPage();
        juce::String r="HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n";
        r+="Content-Length: "+juce::String(body.getNumBytesAsUTF8())+"\r\n";
        r+="Connection: close\r\nAccess-Control-Allow-Origin: *\r\n\r\n"+body;
        return r;
    }
    else if(request.startsWith("GET /status"))
    {
        juce::String s="{\"clients\":"+juce::String(getConnectedClients())+
                       ",\"port\":"+juce::String(port)+
                       ",\"ip\":\""+localIP+"\"}";
        juce::String r="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";
        r+="Content-Length: "+juce::String(s.length())+"\r\nAccess-Control-Allow-Origin: *\r\n\r\n"+s;
        return r;
    }
    else
    {
        juce::String body="<html><body><h1>FAUNA</h1><p><a href='/'>Home</a></p></body></html>";
        juce::String r="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";
        r+="Content-Length: "+juce::String(body.length())+"\r\nConnection: close\r\n\r\n"+body;
        return r;
    }
}
