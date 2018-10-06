#include <iostream>
#include <time.h>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits>
#include "str_tok.h"
#include "nighttpsock.h"
#include "halfmod.h"

using namespace std;

nigSock::nigSock()
{
    mark.clear();
    ip.clear();
    name.clear();
    cookie.clear();
    body.clear();
    session.clear();
    call.clear();
    req.clear();
    host.clear();
    sock = 0;
    port = 0;
    playerFlags = 0;
    length = 0;
    sendCook = false;
    pastHead = false;
    busy = false;
}

nigSock::~nigSock()
{
    nClose();
}

bool nigSock::isOpen()
{
    if (sock == 0) return false;
    return true;
}

void nigSock::assign(int socket, struct sockaddr_in addr)
{
    sock = socket;
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
    ip = inet_ntoa(addr.sin_addr);
    port = ntohs(addr.sin_port);
    /*ifstream file ("./halfMod/plugins/webgui/userlist.ip");
    string line, tmp;
    hmGlobal *global = recallGlobal(NULL);
    playerFlags = 0;
    if (file.is_open())
    {
        while (getline(file,line))
        {
            if (gettok(line,1,"=") == ip)
            {
                tmp = gettok(line,2,"=");
                name = addtok(name,tmp,"|");
                tmp = lower(tmp);
                for (vector<hmAdmin>::iterator it = global->admins.begin(), ite = global->admins.end();it != ite;++it)
                {
                    if (lower(it->client) == tmp)
                    {
                        if (it->flags > playerFlags)
                            playerFlags = it->flags;
                        break;
                    }
                }
            }
        }
        file.close();
    }*/
    if (name == "")
        name = ip;
}

int nigSock::nRead(string &buffer,int size)
{
    //fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) & ~O_NONBLOCK);
    buffer = "";
    char *in = (char*)malloc(sizeof(char) * size);
    int r = read(sock,in,size);
    in[r] = '\0';
    buffer = in;
    free(in);
    return r;
}

int nigSock::nReadLine(string &buffer)
{
    buffer = "";
    char c[1];
    int s = 0;
    while (read(sock,c,1) > 0)
    {
        //cout<<c[0];
        if (c[0] == '\r')// || (c[0] == '\n'))
        {
            //cout<<"newline: "<<int(c[0])<<" pos: "<<s<<endl;
            if (s == 0) s = -1;
            break;
        }
        if (c[0] == '\n')
        {
            if (s)
                break;
        }
        else
        {
            buffer += c[0];
            s++;
        }
    }
    //cout<<endl;
    return s;
}

int nigSock::nWrite(string buffer,int flags)
{
    return send(sock,buffer.c_str(),buffer.size(),flags);
}

int nigSock::writeFile(string filename,int flags)
{
    /*
        flags = options OR'd together:
        1     = Send http header info first. Note: Do not use with 2. -why? -because it will increase the file size beyond what was set in the header
        2     = Convert symbols to their &name; format (amp format)
        4     = HEAD request. Must OR with 1.
        8     = Set Content-Length to 200mb, used with 1 to prevent a connection from closing.
        16    = Set-Cookie according to cookies file or var. Must OR with 1.
        32    = Set Connection: keep-alive. Without this it uses close.
    */
    
    ifstream aFile;
    string line, temp;
    bool remFile = false;
    if (flags & 1)
    {
        line = gettok(filename,-1,".");
        if (istok("jpg.jpeg.jpe",line,".")) line = "image/jpeg";
        else if (istok("png.bmp.gif.cgm.ief",line,".")) line = "image/" + line;
        else if (istok("ico.icon",line,".")) line = "image/x-icon";
        else if (istok("exe.msi",line,".")) line = "application/x-msdownload";
        else if (istok("txt.text.htm.html.log.tmp.c.cpp.h.hpp.sh.out",line,".")) line = "text/html";
        else if (istok("mp3.mpga.mp2",line,".")) line = "audio/mpeg";
        else if ("swf" == line) line = "application/x-shockwave-flash";
        else if (istok("snd.au",line,".")) line = "audio/basic";
        else if ("wav" == line) line = "audio/wav";
        else if ("js" == line) line = "application/javascript";
        else if ((line == "") || (line == filename)) line = "text/html";
        else if (istok("btm.btml",line,"."))
        {
            line = "text/html";
            system(string("/bin/bash " + filename + " " + ip + " " + name + " " + to_string(playerFlags) + " " + host + " " + hexify(mark) + " " + to_string(port) + " " + hexify(cookie) + " > " + filename + ".out").c_str());
            filename += ".out";
            remFile = true;
        }
        else if (istok("gz.gzip.gunzip",line,".")) line = "application/gzip";
        else if ("tar" == line) line = "application/tar";
        else line = "application/" + line;
        /*if (line != "text/html")
        {
            flags = (flags | 8);
            cout<<"not text/html"<<flags<<endl;
        }*/
        line = "Content-Type: " + line + "\r\nLast-Modified: ";
        char date[40];
        struct stat stats;
        stat(filename.c_str(),&stats);
        strftime(date,sizeof(date),"%a, %d %B %Y %H:%M:%S %z",localtime(&(stats.st_mtime)));
        line = line + date + "\r\nContent-Length: ";
        if (flags & 8) 
            line = line + "209715200";
        else
            line = line + to_string(filesize(filename.c_str()));
        if (flags & 32)
            line = line + "\r\nConnection: keep-alive\r\n";
        else
            line = line + "\r\nConnection: close\r\n";
        if ((flags & 16) || (sendCook))
        {
            if ((cookie == "") || (cookie == "null"))
            {
                aFile.open(string("./halfMod/plugins/webgui/cookies/" + ip).c_str());
                if (aFile.is_open())
                {
                    getline(aFile,temp);
                    cookie = temp;
                    aFile.close();
                }
                else
                    cookie = "null";
            }
            line += "Set-Cookie: " + cookie + "\r\n";
            sendCook = false;
        }
        line += "\r\n";
        time_t timer;
        struct tm* tm_info;
        time(&timer);
        tm_info = localtime(&timer);
        strftime(date,sizeof(date),"%a, %d %B %Y %H:%M:%S %z",tm_info);
        line = "HTTP/1.1 200 OK\r\nServer: Minecraft halfMod Web GUI " + string(VER) + "\r\nAccept-Ranges: bytes\r\nDate: " + date + "\r\n" + line;
        //cout<<"Header: \n"<<line<<endl;
        if (flags & 4)
        {
            if (remFile) remove(filename.c_str());
            return send(sock,line.c_str(),line.size(),0);
        }
        else send(sock,line.c_str(),line.size(),0);
    }
    if (!(flags & 4))
    {
        int ret = 0, v = 0;
        ifstream file (filename, ios::in | ios::binary);
        char buf[1024];
        if (file.is_open())
        {
            do
            {
                file.read(buf,1023);
                v = file.gcount();
                line.assign(buf,v);
                if (flags & 2) line = txt2amp(line);
                if ((v = send(sock,line.c_str(),line.length(),MSG_NOSIGNAL)) < 1)
                {
                    cout<<"Error: "<<strerror(errno)<<endl;
                    break;
                }
                else
                    ret += v;
            } while (!file.eof());
            //cout<<"fin. Wrote "<<ret<<" bytes"<<endl;
            file.close();
        }
        else perror("error, file not open!");
        if (remFile) remove(filename.c_str());
        return ret;
    }
    if (remFile) remove(filename.c_str());
    return 0;
}

void nigSock::nClose()
{
    if (sock)
        close(sock);
    mark.clear();
    ip.clear();
    name.clear();
    cookie.clear();
    body.clear();
    session.clear();
    call.clear();
    req.clear();
    host.clear();
    sock = 0;
    port = 0;
    playerFlags = 0;
    length = 0;
    sendCook = false;
    pastHead = false;
    busy = false;
}

int nigSock::wsSend(string message, int flags)
{
    while (busy)
        sleep(10);
    busy = true;
    int ret = send(sock,message.c_str(),message.size(),flags);
    busy = false;
    return ret;
}

string wsMessage(string message, char fin_rsv_opcode)
{
    size_t size = message.size();
    string formatted = "";
    formatted = fin_rsv_opcode;
    if (size >= 126)
    {
        size_t num_bytes;
        if (size > 0xffff)
        {
            num_bytes = 8;
            formatted += char(127);
        }
        else
        {
            num_bytes = 2;
            formatted += char(126);
        }
        for(size_t c = num_bytes - 1; c != static_cast<size_t>(-1); c--)
            formatted += char((static_cast<unsigned long long>(size) >> (8 * c)) % 256);
    }
    else
        formatted += static_cast<char>(size);
    formatted += message;
    return formatted;
}

/*int nigSock::wsRead(char *buffer, int bytes)
{
    return read(sock,(char *)&buffer[0],bytes);
}*/

void nigSock::wsReadContent(string &buffer, int size, unsigned char fin_rsv_opcode)
{
    if (size > numeric_limits<std::size_t>::max())
    {
        hmOutDebug("WebSocket message too long.");
        nClose();
        return;
    }
    //cout<<"WSReadDebug: Size: "<<size<<endl;
    unsigned char mask[4];
    read(sock,(char *)&mask[0],4);
    unsigned char bit[2];
    for (size_t c = 0;c < size;c++)
    {
        read(sock,(char *)&bit[0],1);
        buffer += (bit[0] ^ mask[c % 4]);
    }
    if ((fin_rsv_opcode & 0x0f) == 8)
        nClose();
    else if (((fin_rsv_opcode & 0x0f) == 9) || ((fin_rsv_opcode & 0x0f) == 10))
    {
        buffer.clear();
    }
}

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg(); 
}

string txt2amp(string in)
{
    return strreplace(strreplace(strreplace(strreplace(strreplace(strreplace(in,"&","&amp;"),"<","&lt;"),">","&gt;"),"\"","&quot;"),"$","&#36;"),"'*'","*");
}

string dec2hex(unsigned int n)
{
    string res;
    do
    {
        res += "0123456789ABCDEF"[n % 16];
        n >>= 4;
    } while (n);
    return string(res.rbegin(), res.rend());
}
string hexify(string in)
{
    string out;
    for (auto it = in.begin(), ite = in.end();it != ite;++it)
        out += dec2hex(int(*it));
    return out;
}
string decify(string in)
{
    string out;
    for (int i = 0, j = in.size();i < j;i+=2)
        out += char(stoi(in.substr(i,2),0,16));
    return out;
}


