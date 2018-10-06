#include <stdio.h>
#include <string.h>   //strlen
#include <cstdlib>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <fcntl.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>
#include "webgui_server.h"
#include "nighttpsock.h"
#include "str_tok.h"
#include "halfmod.h"
#include "sha1.h"
#include "base64.h"
using namespace std;

vector<wsEndPoint> endPoints;

int maxclients;
int master_socket;
int addrlen;
struct sockaddr_in address;
nigSock *sockets;
bool running;

short webguiInit(string ip, int port, int max)
{
    maxclients = max;
    sockets = new nigSock[maxclients];
    if((master_socket = socket(AF_INET,SOCK_STREAM,0)) == 0) 
        return 1;
    
    int opt = 1;
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
        return 2;
    string bindIP = ip;
    int bindPort = port;
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr( bindIP.c_str() );
    address.sin_port = htons( bindPort );
    
    if (::bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        hmOutDebug("Unable to bind WebGUI listening server to " + bindIP + ":" + to_string(bindPort) + " . . .");
        return 4;
    }
    
    hmOutDebug("WebGUI now listening on " + bindIP + ":" + to_string(bindPort));
    
    if (listen(master_socket,3) < 0)
        return 3;
    
    addrlen = sizeof(address);
    return 0;
}

void webguiRun()
{
    int max_sd, activity, new_socket, valread;
    fd_set readfds;
    string buffer;
    running = true;
    while (running)
    {
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
        
        //add child sockets to set
        for (int i = 0;i < maxclients;i++) 
        {
            //if valid socket descriptor then add to read list
            if(sockets[i].sock > 0)
                FD_SET( sockets[i].sock , &readfds);
            
            //highest file descriptor number, need it for the select function
            if(sockets[i].sock > max_sd)
                max_sd = sockets[i].sock;
        }
        
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        if ((!running) || ((activity < 1) && (errno!=EINTR)))
            continue;
        if (FD_ISSET(master_socket, &readfds))
        {
            //cout<<"wat\n";
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
                continue;
            //cout<<"accepted\n";
            for (int i = 0;i < MAXCLIENTS;i++)
            {
                if (!sockets[i].isOpen())
                {
                    //cout<<"assigned\n";
                    sockets[i].assign(new_socket,address);
                    //connectedClients++;
                    break;
                }
                else if (i == maxclients-1)
                {
                    //cout<<"no room\n";
                    perror("Too many open connections. . .");
                    send(new_socket,"Sorry, no room for you right now!\r\n",35,0);
                    close(new_socket);
                    new_socket = 0;
                }
            }
            /*if (new_socket)
            {
                if (sockets[i].ip != sockets[i].name)
                    cout<<"New connection established with "<<sockets[i].ip<<"("<<sockets[i].name<<")"<<":"<<sockets[i].port<<endl;
                else
                    cout<<"New connection established with "<<sockets[i].ip<<":"<<sockets[i].port<<endl;
                cout<<"Total open connections: "<<connectedClients<<endl;
            }*/
        }
        for (int i = 0;i < maxclients;i++)
        {
            if (sockets[i].sock == 0)
                continue;
            if (FD_ISSET(sockets[i].sock,&readfds))
            {
                //cout<<"reading\n";
                if ((sockets[i].pastHead) && (sockets[i].call == "post")) // body
                {
                    if ((valread = sockets[i].nRead(buffer,1024)) > 0)
                    {
                        if (sockets[i].body.size() > 0)
                            sockets[i].body.append(buffer);
                        else
                            sockets[i].body = buffer;
                    }
                    if (sockets[i].body.size() >= sockets[i].length)
                    {
                        ofstream file (string("./halfMod/plugins/webgui/post." + to_string(sockets[i].port)).c_str(),ios_base::out|ios_base::trunc);
                        if (file.is_open())
                        {
                            file<<sockets[i].body;
                            file.close();
                        }
                        sockets[i].sendCook = true;
                        //cout<<"Post: "<<sockets[i].body<<endl;
                        sockets[i].writeFile(buildPath(sockets[i].req),1);
                        //cout<<"Sent.\n";
                        remove(string("./halfMod/plugins/webgui/post." + to_string(sockets[i].port)).c_str());
                        //cout<<"Removed.\n";
                        sockets[i].pastHead = false;
                        sockets[i].mark.clear();
                        sockets[i].req.clear();
                        sockets[i].call.clear();
                        sockets[i].host.clear();
                    }
                }
                else if (isEndPoint(sockets[i].session))
                {
                    /*if ((valread = sockets[i].nReadLine(buffer)) > 0)
                    {
                        cout<<":: "<<buffer<<endl;
                        //hmServerCommand(buffer);
                        sockets[i].nWrite("sent");
                    }=*/
                    //cout<<"reading payload.\n";
                    unsigned char first[2];
                    read(sockets[i].sock,(char *)&first[0],2);
                    unsigned char fin_rsv_opcode = first[0];
                    /*if (first[1] < 128)
                    {
                        cout<<"error.\n";
                        sockets[i].nClose();
                    }
                    else
                    {*/
                        size_t len = (first[1] & 127);
                        //cout<<"WSDebug: len: "<<len<<" fin_rsv_opcode: "<<fin_rsv_opcode<<endl;
                        if (len == 126)
                        {
                            unsigned char length[2];
                            read(sockets[i].sock,(char *)&length[0],2);
                            len = 0;
                            size_t num_bytes = 2;
                            for (size_t c = 0;c < num_bytes;c++)
                                len += static_cast<std::size_t>(length[c]) << (8 * (num_bytes - 1 - c));
                            if ((int)len < 0)
                                len += 256;
                            sockets[i].wsReadContent(buffer,len,fin_rsv_opcode);
                        }
                        else if (len == 127)
                        {
                            unsigned char length[8];
                            read(sockets[i].sock,(char *)&length[0],8);
                            len = 0;
                            size_t num_bytes = 8;
                            for (size_t c = 0;c < num_bytes;c++)
                                len += static_cast<std::size_t>(length[c]) << (8 * (num_bytes - 1 - c));
                            sockets[i].wsReadContent(buffer,len,fin_rsv_opcode);
                        }
                        else
                        {
                            sockets[i].wsReadContent(buffer,len,fin_rsv_opcode);
                        }
                        //cout<<":: "<<buffer<<endl;
                        if (sockets[i].isOpen())
                        {
                            if (sockets[i].pastHead)
                            {
                                if (((fin_rsv_opcode & 0x80) == 0) || ((fin_rsv_opcode & 0x0f) == 0) || (sockets[i].body.size() > 0))
                                {
                                    //cout<<"fragment\n";
                                    sockets[i].body += buffer;
                                    if ((fin_rsv_opcode & 0x80) != 0)
                                    {
                                        //cout<<"fragments complete: "<<sockets[i].body<<endl;
                                        handleReceiveCallback(sockets[i].session,i,sockets[i].ip,sockets[i].name,sockets[i].body);
                                        sockets[i].body.clear();
                                    }
                                }
                                else
                                {
                                    //cout<<"full message: "<<buffer<<endl;
                                    handleReceiveCallback(sockets[i].session,i,sockets[i].ip,sockets[i].name,buffer);
                                }
                            }
                            else
                                sockets[i].pastHead = true;
                        }
                        buffer.clear();
                        //sockets[i].nWrite("sent");
                    //}
                }
                else if ((valread = sockets[i].nReadLine(buffer)) == 0) // eof/disconnect
                {
                        //socket is closing
                    //cout<<"Host disconnect: "<<sockets[i].name<<":"<<sockets[i].port<<endl;
                    sockets[i].nClose();
                    //connectedClients--;
                }
                else if ((valread == -1) && (!sockets[i].pastHead) && (sockets[i].req.size() > 0) && (sockets[i].host.size() > 0)) // empty line (\n|\r)
                {
                    sockets[i].pastHead = true;
                    if ((sockets[i].session == "cookie_required") && ((sockets[i].cookie == "null") || (sockets[i].cookie == "")))
                    {
                        //sockets[i].req = "./halfMod/plugins/webgui/www/403.htm";
                        //cout<<"<<Redirecting\n";
                        sockets[i].nWrite("HTTP/1.1 307 Temporary Redirect\r\nLocation: /login.html?error=403\r\n\r\n");
                        //sockets[i].nClose();
                    }
                    else
                    {
                        //cout<<"sending file: "<<buildPath(sockets[i].req)<<" to "<<sockets[i].ip<<":"<<sockets[i].port<<endl;
                        if ((sockets[i].call == "get") && (sockets[i].session.size() > 16))
                        { // connection established
                            if (!isEndPoint(sockets[i].mark))
                                sockets[i].nClose();
                            else
                            {
                                fcntl(sockets[i].sock, F_SETFL, fcntl(sockets[i].sock, F_GETFL) & ~O_NONBLOCK);
                                unsigned char hash[20];
                                sha1::calc(sockets[i].session.c_str(),sockets[i].session.size(),hash);
                                string auth = base64_encode(reinterpret_cast<const unsigned char*>(hash), 20);
                                sockets[i].nRead(buffer,1);
                                buffer.clear();
                                sockets[i].nWrite("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + auth + "\r\n\r\n");
                                sockets[i].session = sockets[i].mark;
                                buffer.clear();
                                handleConnectCallback(sockets[i].session,i,sockets[i].ip,sockets[i].name);
                            }
                        }
                        else if (sockets[i].call == "get")// || (sockets[i].call == "post"))
                            sockets[i].writeFile(buildPath(sockets[i].req),1);
                        else if (sockets[i].call == "head")
                            sockets[i].writeFile(buildPath(sockets[i].req),5);
                        else if (sockets[i].call == "post")
                            /*cout<<"Test: "<<*/sockets[i].nRead(buffer,1);//<<" :: "<<buffer<<endl;
                    }
                    /*else if (sockets[i].call == "post")
                    {
                        //sockets[i].writeFile(buildPath(hex2txt(sockets[i].req)),5);
                        //sockets[i].nWrite("RequestContents: \r\n");
                        //sockets[i].nWrite("HTTP/1.1 200 OK\r\nServer: Minecraft halfMod Web GUI v0.4.3\r\nAccept-Ranges: bytes\r\nDate: Sat, 14 April 2018 03:20:16 -0400\r\nContent-Type: text/html\r\nLast-Modified: Sat, 14 April 2018 03:20:16 -0400\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n");
                        //sockets[i].writeFile(buildPath(hex2txt(sockets[i].req)),49);
                        while ((valread = sockets[i].nRead(buffer,1024)) > 0)
                        {
                            if (sockets[i].body.size() > 0)
                                sockets[i].body = sockets[i].body + " " + buffer;
                            else
                                sockets[i].body = buffer;
                        }
                        sockets[i].sendCook = true;
                        if (sockets[i].body.size() >= sockets[i].length())
                        {
                            ofstream file (string("./halfMod/plugins/webgui/post." + to_string(sockets[i].port)).c_str(),ios_base::out|ios_base::trunc);
                            if (file.is_open())
                            {
                                file<<sockets[i].body;
                                file.close();
                            }
                            sockets[i].writeFile(buildPath(sockets[i].req),1);
                            remove(string("./halfMod/plugins/webgui/post." + to_string(sockets[i].port)).c_str());
                        }
                    }*/
                    //sockets[i].nClose();
                }
                else // header
                {
                    //cout<<":: "<<buffer<<endl;
                    //if ((sockets[i].pastHead) && (sockets[i].call == "post")) // body
                    //{
                        /*regex ptrn ("(.*)(GET [^ ]+ [^ ]+)$");//("^&?([^ =]+=[^&]+)");
                        string temp = "";
                        if (regex_match(buffer,ml,ptrn))
                        {
                            buffer = ml[1].str();
                            temp = ml[2].str();
                        }*/
                        /*regex ptrn ("^&?([^ =]+=[^&]+)");
                        smatch ml;
                        while (regex_search(buffer,ml,ptrn))
                        {
                            if (sockets[i].body.size() > 0)
                                sockets[i].body = sockets[i].body + " " + ml[1].str();
                            else
                                sockets[i].body = ml[1].str();
                            buffer = ml.suffix().str();
                        }*/
                        /*if (sockets[i].body.size() > 0)
                            sockets[i].body = sockets[i].body + " " + buffer;
                        else
                            sockets[i].body = buffer;*/
                        //cout<<"Post data: "<<sockets[i].body<<endl;
                        //if (temp.size() > 0)
                        //    buffer = temp;
                        /*sockets[i].sendCook = true;
                        if (sockets[i].body.size() >= sockets[i].length())
                        {
                            ofstream file (string("./halfMod/plugins/webgui/post." + to_string(sockets[i].port)).c_str(),ios_base::out|ios_base::trunc);
                            if (file.is_open())
                            {
                                file<<sockets[i].body;
                                file.close();
                            }
                            sockets[i].writeFile(buildPath(sockets[i].req),1);
                            remove(string("./halfMod/plugins/webgui/post." + to_string(sockets[i].port)).c_str());
                        }
                    }*/
                    //else
                    //{
                    if (sockets[i].pastHead)
                    {
                        sockets[i].pastHead = false;
                        sockets[i].mark.clear();
                        sockets[i].req.clear();
                        sockets[i].call.clear();
                        sockets[i].host.clear();
                    }
                    //cout<<":: "<<buffer<<endl;
                    string key = lower(gettok(buffer,1," "));
                    if (istok("get post head",key," "))
                    {
                        sockets[i].call = key;
                        sockets[i].mark = hex2txt(gettok(buffer,2," "));
                        sockets[i].req = "./halfMod/plugins/webgui/www" + gettok(sockets[i].mark,1,"?");
                        if (isin(hex2txt(sockets[i].req),"panel"))
                            sockets[i].session = "cookie_required";
                    }
                    else if (key == "host:")
                        sockets[i].host = deltok(buffer,1," ");
                    else if (key == "cookie:")
                    {
                        string cookie = "null";
                        ifstream file (string("./halfMod/plugins/webgui/cookies/" + sockets[i].ip).c_str());
                        if (file.is_open())
                        {
                            getline(file,cookie);
                            file.close();
                        }
                        if ((cookie != "null") && (istokcs(strremove(deltok(buffer,1," ")," "),cookie,";")))
                        {
                            sockets[i].cookie = cookie;
                            sockets[i].name = gettok(cookie,2,"=");
                            string tmp = lower(sockets[i].name);
                            auto c = recallGlobal(NULL)->admins.find(tmp);
                            if (c != recallGlobal(NULL)->admins.end())
                                sockets[i].playerFlags = c->second.flags;
                        }
                        else
                            sockets[i].cookie = "null";
                    }
                    else if (key == "content-length:")
                    {
                        key = gettok(buffer,2," ");
                        if (stringisnum(key))
                            sockets[i].length = stoi(key);
                    }
                    else if (key == "sec-websocket-key:")
                        sockets[i].session = deltok(buffer,1," ") + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                        //sockets[i].body = "dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                }
            }
        }
    }
}

bool isEndPoint(string endPoint)
{
    for (auto it = endPoints.begin(), ite = endPoints.end();it != ite;++it)
        if (it->point == endPoint)
            return true;
    return false;
}

void sendToWebSockets(string message)
{
    for (int i = 0;i < maxclients;i++)
    {
        if (sockets[i].sock == 0)
            continue;
        if (sockets[i].session == "established")
            sockets[i].wsSend(wsMessage(message));
    }
}

void wsSendToEndPoint(string endPoint, string message)
{
    for (int i = 0;i < maxclients;i++)
    {
        if (sockets[i].sock == 0)
            continue;
        if (sockets[i].session == endPoint)
            sockets[i].wsSend(wsMessage(message));
    }
}

void wsSendToSocket(int sock, string message)
{
    if (sockets[sock].sock == 0)
        return;
    if (sockets[sock].session.size() > 0)
        sockets[sock].wsSend(wsMessage(message));
}

void wsCreateEndPoint(string endPoint, int (*connectCallback)(std::string,int,std::string,std::string), int (*receiveCallback)(std::string,int,std::string,std::string,std::string))
{
    endPoints.push_back({endPoint,connectCallback,receiveCallback});
}

void wsCloseEndPoint(string endPoint)
{
    for (auto it = endPoints.begin();it != endPoints.end();)
    {
        if (it->point == endPoint)
        {
            disconnectEndPoint(endPoint);
            endPoints.erase(it);
        }
        else
            ++it;
    }
}

void wsCloseSocket(int sock)
{
    sockets[sock].nClose();
}

void disconnectEndPoint(string endPoint)
{
    for (int i = 0;i < maxclients;i++)
        if (sockets[i].session == endPoint)
            sockets[i].nClose();
}

void handleConnectCallback(string endPoint, int sock, string ip, string player)
{
    for (auto it = endPoints.begin(), ite = endPoints.end();it != ite;++it)
    {
        if (it->point == endPoint)
        {
            if ((*it->connectCallback)(endPoint,sock,ip,player))
                break;
        }
    }
}

void handleReceiveCallback(string endPoint, int sock, string ip, string player, string message)
{
    for (auto it = endPoints.begin(), ite = endPoints.end();it != ite;++it)
    {
        if (it->point == endPoint)
        {
            if ((*it->receiveCallback)(endPoint,sock,ip,player,message))
                break;
        }
    }
}

void webguiStop()
{
    running = false;
    delete[] sockets;
    close(master_socket);
}

string buildPath(string path)
{
    path = remtok(path,"..",0,"/");
    if (path == "") path = "./halfMod/plugins/webgui/www/";
    struct stat buffer;
    int r = 0;
    while (true)
    {
        if (stat(path.c_str(),&buffer) == 0)
        {
            if (buffer.st_mode & S_IFREG)
            {
                break;
            }
            else if (buffer.st_mode & S_IFDIR)
            {
                path = appendtok(path,"index.htm","/");
                r = 1;
            }
            else path = "./halfMod/plugins/webgui/www/404.htm";
        }
        else if (r == 1)
        {
            path += "l";
            r = 2;
        }
        else if (r == 2)
        {
            path = deltok(path,-1,".") + ".btm";
            r = 3;
        }
        else if (r == 3)
        {
            path += "l";
            r = 4;
        }
        else if (r == 4)
        {
            path = "./halfMod/plugins/webgui/default.htm";
            r = 5;
        }
        else if (r == 5)
        {
            perror("No \"./halfMod/plugins/webgui/www/index.htm(l)\" and no \"./halfMod/plugins/webgui/default.htm\" . . .");
            path = "www/404.htm";
        }
        else if (path == "./halfMod/plugins/webgui/www/404.htm")
        {
            perror("No \"./halfMod/plugins/webgui/www/404.htm\" . . .");
            path = "";
            break;
        }
        else path = "./halfMod/plugins/webgui/www/404.htm";
    }
    return path;
}

int hex2dec(string hex)
{
    return stoi(hex,0,16);
}
string hex2txt(string in)
{
    string ret;
    
    for (int i = 0;i < in.size();i++)
    {
        if ((in[i] == '%') && (i < in.size()-2))
        {
            ret += char(hex2dec(strmid(in,i+1,2)));
            i+=2;
        }
        else ret += in[i];
    }
    return strreplace(ret,"+"," ");
}



//71.33.222.227: GET / HTTP/1.1
//71.33.222.227: Host: minecraft.mudkip.in:9001
//71.33.222.227: User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:57.0) Gecko/20100101 Firefox/57.0
//71.33.222.227: Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
//71.33.222.227: Accept-Language: en-US,en;q=0.5
//71.33.222.227: Accept-Encoding: gzip, deflate
//71.33.222.227: Connection: keep-alive
//71.33.222.227: Upgrade-Insecure-Requests: 1
//71.33.222.227: 








