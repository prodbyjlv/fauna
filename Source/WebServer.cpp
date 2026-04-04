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
    Sleep(50);
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
                juce::ScopedLock lock(clientsLock);
                for(int j=audioClients.size()-1;j>=0;--j)
                {
                    AudioClient& c=audioClients.getReference(j);
                    if(c.socket==s){ handleWebSocketClient(c); break; }
                }
            }
            else if(r<0)
            {
                juce::ScopedLock lock(clientsLock);
                for(int j=audioClients.size()-1;j>=0;--j)
                {
                    if(audioClients.getReference(j).socket==s)
                    { audioClients.remove(j); connectedClientCount.store(audioClients.size()); break; }
                }
            }
        }

        {
            juce::ScopedLock lock(clientsLock);
            for(int i=audioClients.size()-1;i>=0;i--)
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
    if(length<126){ frame[1]=(char)(length&0x7F); }
    else if(length<65536){ frame[1]=126; frame[2]=(char)((length>>8)&0xFF); frame[3]=(char)(length&0xFF); }
    else{ frame[1]=127; frame[2]=0;frame[3]=0;frame[4]=0;frame[5]=0;
          frame[6]=(char)((length>>24)&0xFF);frame[7]=(char)((length>>16)&0xFF);
          frame[8]=(char)((length>>8)&0xFF);frame[9]=(char)(length&0xFF); }
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
    html+="<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
    html+="<title>FAUNA Audio Stream</title><style>";
    html+="*{margin:0;padding:0;box-sizing:border-box}";
    html+="body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#1a1a2e 0%,#16213e 100%);min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center;color:white;padding:20px}";
    html+=".container{background:rgba(255,255,255,0.1);border-radius:20px;padding:40px;text-align:center;backdrop-filter:blur(10px);max-width:400px;width:100%}";
    html+="h1{font-size:2em;margin-bottom:10px;color:#00d4ff}h2{font-size:1em}";
    html+=".status{font-size:0.9em;color:#aaa;margin-bottom:30px}";
    html+=".level-meter{width:100%;height:30px;background:rgba(0,0,0,0.3);border-radius:15px;overflow:hidden;margin-bottom:30px}";
    html+=".level-bar{height:100%;width:0%;background:linear-gradient(90deg,#00ff88,#00d4ff,#ffaa00,#ff4444);transition:width 0.08s;border-radius:15px}";
    html+=".mute-btn{padding:20px 60px;font-size:1.5em;border:none;border-radius:15px;cursor:pointer;transition:all 0.3s;font-weight:bold}";
    html+=".mute-btn.muted{background:#ff4444;color:white}.mute-btn.unmuted{background:#00ff88;color:#1a1a2e}";
    html+=".connection-status{margin-top:20px;font-size:0.8em;color:#888}";
    html+=".connected{color:#00ff88}";
    html+=".info-box{background:rgba(0,0,0,0.2);padding:15px;border-radius:10px;margin-bottom:20px}";
    html+=".info-box p{margin:5px 0;font-size:0.85em}";
    html+=".start-btn{padding:15px 40px;font-size:1.2em;background:#00d4ff;color:#1a1a2e;border:none;border-radius:10px;cursor:pointer;margin-bottom:20px;transition:all 0.3s;-webkit-tap-highlight-color:transparent}";
    html+=".start-btn:disabled{opacity:0.5;cursor:not-allowed}";
    html+="</style></head><body>";
    html+="<div class=\"container\">";
    html+="<h1>FAUNA</h1><p class=\"status\">Audio Streaming Control</p>";
    html+="<div class=\"info-box\">";
    html+="<p><strong>Server:</strong> <span id=\"serverStatus\">Checking...</span></p>";
    html+="<p><strong>Devices:</strong> <span id=\"deviceCount\">0</span></p>";
    html+="<p><strong>Audio:</strong> <span id=\"audioStatus\">Click Start to begin</span></p>";
    html+="<p><strong>Sample Rate:</strong> <span id=\"srStatus\">-</span></p>";
    html+="<p><strong>Buffer:</strong> <span id=\"bufStatus\">-</span></p>";
    html+="</div>";
    html+="<button class=\"start-btn\" id=\"startBtn\" onclick=\"startAudio()\">START AUDIO</button>";
    html+="<div class=\"level-meter\"><div class=\"level-bar\" id=\"levelBar\"></div></div>";
    html+="<button class=\"mute-btn unmuted\" id=\"muteBtn\" onclick=\"toggleMute()\">UNMUTED</button>";
    html+="<p class=\"connection-status\" id=\"status\">Ready</p>";
    html+="</div>";
    html+="<audio id=\"iosAudio\" playsinline></audio>";
    html+="<script>";

    // Global state
    html+="var isMuted=false,ws=null,audioCtx=null,scriptNode=null,started=false;";

    // Ring buffer — 2 seconds at 48kHz stereo = 192000 floats
    html+="var RING_SIZE=192000;";
    html+="var ringBuf=new Float32Array(RING_SIZE);";
    html+="var ringWrite=0,ringRead=0;";

    // Sample rates — serverSampleRate set from init message, deviceSampleRate from AudioContext
    html+="var serverSampleRate=0,deviceSampleRate=0;";

    // Linear interpolation resampler phase accumulator (persists across packets)
    html+="var resamplerPhase=0.0;";

    // Prebuffer: hold off playback until we have enough frames to be smooth
    html+="var PREBUFFER_FRAMES=4096;";
    html+="var prebuffering=true;";

    // Ring buffer helpers
    html+="function ringAvailable(){var a=ringWrite-ringRead;if(a<0)a+=RING_SIZE;return a;}";
    html+="function ringPush(f){ringBuf[ringWrite]=f;ringWrite=(ringWrite+1)%RING_SIZE;}";
    html+="function ringPop(){var f=ringBuf[ringRead];ringRead=(ringRead+1)%RING_SIZE;return f;}";

    // Resample and push with linear interpolation
    html+="function resampleAndPush(data){";
    html+=  "if(serverSampleRate===0||deviceSampleRate===0||serverSampleRate===deviceSampleRate){";
    html+=    "for(var i=0;i<data.length;i++)ringPush(data[i]);";
    html+=    "return;";
    html+=  "}";
    html+=  "var ratio=serverSampleRate/deviceSampleRate;";
    html+=  "var numInputFrames=data.length/2;";
    html+=  "while(resamplerPhase<numInputFrames){";
    html+=    "var idx=Math.floor(resamplerPhase);";
    html+=    "var frac=resamplerPhase-idx;";
    html+=    "var curL=data[idx*2],curR=data[idx*2+1];";
    html+=    "var ni=idx+1;";
    html+=    "var nL=(ni*2<data.length)?data[ni*2]:curL;";
    html+=    "var nR=(ni*2+1<data.length)?data[ni*2+1]:curR;";
    html+=    "ringPush(curL+(nL-curL)*frac);";
    html+=    "ringPush(curR+(nR-curR)*frac);";
    html+=    "resamplerPhase+=ratio;";
    html+=  "}";
    html+=  "resamplerPhase-=numInputFrames;";
    html+=  "if(resamplerPhase<0)resamplerPhase=0;";
    html+="}";

    // Unlock iOS audio - valid 44-byte WAV forces Media session (ignores mute switch)
    html+="function unlockIOSAudio(ctx){";
    html+=  "var a=document.getElementById('iosAudio');";
    html+=  "a.src='data:audio/wav;base64,UklGRigAAABXQVZFZm10IBIAAAABAAEARKwAAIhYAQACABAAAABkYXRhAgAAAAEA';";
    html+=  "a.loop=true;a.volume=0.001;a.play().catch(function(e){console.log('Unlock failed',e);});";
    html+="}";

    // Start audio
    html+="function startAudio(){";
    html+=  "if(started)return;";
    html+=  "started=true;";
    html+=  "document.getElementById('startBtn').disabled=true;";
    html+=  "document.getElementById('startBtn').textContent='Connecting...';";
    html+=  "if(navigator.audioSession){navigator.audioSession.type='playback';}";
    html+=  "document.getElementById('audioStatus').textContent='Starting...';";
    html+=  "try{audioCtx=new(window.AudioContext||window.webkitAudioContext)();}";
    html+=  "catch(e){document.getElementById('audioStatus').textContent='AudioContext failed: '+e.message;";
    html+=    "started=false;document.getElementById('startBtn').disabled=false;";
    html+=    "document.getElementById('startBtn').textContent='START AUDIO';return;}";
    html+=  "deviceSampleRate=audioCtx.sampleRate;";
    html+=  "document.getElementById('srStatus').textContent='Device: '+deviceSampleRate+' Hz';";
    html+=  "if(audioCtx.state==='suspended'){audioCtx.resume().then(function(){unlockIOSAudio(audioCtx);})}";
    html+=  "else{unlockIOSAudio(audioCtx);}";
    html+=  "scriptNode=audioCtx.createScriptProcessor(4096,1,2);";
    html+=  "var dummyOsc=audioCtx.createOscillator();";
    html+=  "dummyOsc.connect(scriptNode);dummyOsc.start(0);";
    html+=  "scriptNode.onaudioprocess=function(e){";
    html+=    "var L=e.outputBuffer.getChannelData(0);";
    html+=    "var R=e.outputBuffer.getChannelData(1);";
    html+=    "var frames=L.length;";
    html+=    "var avail=ringAvailable()/2;";
    html+=    "document.getElementById('bufStatus').textContent=Math.floor(avail)+' frames';";
    html+=    "if(prebuffering){";
    html+=      "if(avail>=PREBUFFER_FRAMES){prebuffering=false;}";
    html+=      "else{for(var i=0;i<frames;i++){L[i]=0;R[i]=0;}return;}";
    html+=    "}";
    html+=    "if(avail<frames){";
    html+=      "prebuffering=true;";
    html+=      "for(var i=0;i<frames;i++){L[i]=0;R[i]=0;}return;";
    html+=    "}";
    html+=    "for(var i=0;i<frames;i++){L[i]=ringPop();R[i]=ringPop();}";
    html+=  "};";
    html+=  "scriptNode.connect(audioCtx.destination);";
    html+=  "var wsUrl='ws://'+location.hostname+':'+(location.port||'8080');";
    html+=  "document.getElementById('status').textContent='Connecting to '+wsUrl;";
    html+=  "try{ws=new WebSocket(wsUrl);}";
    html+=  "catch(e){document.getElementById('audioStatus').textContent='WS failed: '+e.message;";
    html+=    "started=false;document.getElementById('startBtn').disabled=false;";
    html+=    "document.getElementById('startBtn').textContent='START AUDIO';return;}";
    html+=  "ws.binaryType='arraybuffer';";
    html+=  "ws.onopen=function(){";
    html+=    "document.getElementById('startBtn').textContent='Streaming...';";
    html+=    "document.getElementById('audioStatus').textContent='Buffering...';";
    html+=    "document.getElementById('status').textContent='Connected';};";
    html+=  "ws.onclose=function(e){";
    html+=    "document.getElementById('audioStatus').textContent='Disconnected ('+e.code+')';";
    html+=    "document.getElementById('audioStatus').style.color='#ff4444';";
    html+=    "document.getElementById('startBtn').textContent='START AUDIO';";
    html+=    "document.getElementById('startBtn').disabled=false;started=false;};";
    html+=  "ws.onerror=function(){";
    html+=    "document.getElementById('audioStatus').textContent='Connection error - check WiFi';";
    html+=    "document.getElementById('audioStatus').style.color='#ff4444';};";
    html+=  "ws.onmessage=function(e){";
    html+=    "if(e.data instanceof ArrayBuffer){";
    html+=      "var d=new Float32Array(e.data);";
    html+=      "resampleAndPush(d);";
    html+=      "if(!prebuffering){";
    html+=        "document.getElementById('audioStatus').textContent='Playing!';";
    html+=        "document.getElementById('audioStatus').className='connected';}";
    html+=      "var mx=0;";
    html+=      "for(var i=0;i<Math.min(100,d.length);i++){var a=Math.abs(d[i]);if(a>mx)mx=a;}";
    html+=      "document.getElementById('levelBar').style.width=(mx*100)+'%';";
    html+=    "}else{";
    html+=      "try{var msg=JSON.parse(e.data);";
    html+=        "if(msg.type==='init'&&msg.sampleRate){";
    html+=          "serverSampleRate=msg.sampleRate;";
    html+=          "var match=serverSampleRate===deviceSampleRate;";
    html+=          "document.getElementById('srStatus').textContent=";
    html+=            "'Server: '+serverSampleRate+' / Device: '+deviceSampleRate+' Hz';";
    html+=          "document.getElementById('srStatus').style.color=match?'#00ff88':'#ffaa00';";
    html+=        "}}catch(ex){}";
    html+=    "}";
    html+=  "};";
    html+="}";

    // Toggle mute
    html+="function toggleMute(){";
    html+=  "isMuted=!isMuted;";
    html+=  "var btn=document.getElementById('muteBtn');";
    html+=  "if(isMuted){btn.textContent='MUTED';btn.className='mute-btn muted';}";
    html+=  "else{btn.textContent='UNMUTED';btn.className='mute-btn unmuted';}";
    html+=  "document.getElementById('status').textContent=isMuted?'Muted':'Unmuted';";
    html+=  "if(ws&&ws.readyState===WebSocket.OPEN)ws.send(isMuted?'mute:true':'mute:false');";
    html+="}";

    // Update status
    html+="function updateStatus(){";
    html+=  "fetch('/status?t='+Date.now(),{headers:{'Accept':'application/json'}})";
    html+=  ".then(function(r){if(!r.ok)throw new Error();return r.json();})";
    html+=  ".then(function(d){";
    html+=    "document.getElementById('serverStatus').textContent='Running';";
    html+=    "document.getElementById('serverStatus').className='connected';";
    html+=    "document.getElementById('deviceCount').textContent=d.clients;})";
    html+=  ".catch(function(){";
    html+=    "document.getElementById('serverStatus').textContent='Not Running';";
    html+=    "document.getElementById('serverStatus').style.color='#ff4444';});";
    html+="}";

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
                       ",\"port\":"+juce::String(port)+",\"ip\":\""+localIP+"\"}";
        juce::String r="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";
        r+="Content-Length: "+juce::String(s.length())+"\r\n";
        r+="Cache-Control: no-cache, no-store\r\nAccess-Control-Allow-Origin: *\r\n\r\n"+s;
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
