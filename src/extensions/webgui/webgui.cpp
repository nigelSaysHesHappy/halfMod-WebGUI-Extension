#include <iostream>
#include "halfmod.h"
#include "webgui_server.h"
#include <thread>
//#include <unistd.h>
//#include <atomic>
#include <fstream>
#include <vector>
#include "str_tok.h"
using namespace std;

#define WEBGUI_VERSION "v0.1.3"

struct configButton
{
    string name;
    bool useCb;
    string display;
    int flags;
    string command;
    std::string (*callback)(std::string,int,std::string,std::string);
};

vector<configButton> cfgButtons;

thread webgui;

void setupConfig();

extern "C" {

int onExtensionLoad(hmExtension &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.extensionInfo("Web GUI",
                      "nigel",
                      "halfMod Interactive Server Control Panel",
                      WEBGUI_VERSION,
                      "http://localhost:9001/");
    ifstream file(CONF);
    string bindIP;
    int bindPort;
    bool configEnabled = false;
    if (file.is_open())
    {
        string line;
        while (getline(file,line))
        {
            if (iswm(nospace(line),"#*")) continue;
            line = lower(line);
            if (gettok(line,1,"=") == "ip") bindIP = gettok(line,2,"=");
            if (gettok(line,1,"=") == "port") bindPort = str2int(gettok(line,2,"="));
            if (gettok(line,1,"=") == "enable-config-page")
            {
                line = gettok(line,2,"=");
                if ((line == "true") || (line == "1"))
                    configEnabled = true;
                else
                    configEnabled = false;
            }
        }
        file.close();
    }
    int err = webguiInit(bindIP,bindPort);
    if (err)
    {
        switch (err)
        {
            case 1:
                hmOutDebug("Failure creating webgui master socket . . .");
            case 2:
                hmOutDebug("Failure enabling simultaneous webgui socket connections . . .");
            case 3:
                hmOutDebug("Unable to begin listening for incoming WebGUI connections . . .");
        }
        return err;
    }
    hmOutDebug("WebGUI initialized . . .");
    webgui = thread(webguiRun);
    if (configEnabled)
        setupConfig();
    //handle.hookPattern("thread",".*","onServerOutput");
    return 0;
}

void sendToEndPoint(string endPoint,string message)
{
    wsSendToEndPoint(endPoint, message);
}

void sendToSocket(int socket, string message)
{
    wsSendToSocket(socket, message);
}

void regEndPoint(string endPoint, int (*connectCallback)(std::string,int,std::string,std::string), int (*receiveCallback)(std::string,int,std::string,std::string,std::string))
{
    wsCreateEndPoint(endPoint, connectCallback, receiveCallback);
}

void closeEndPoint(string endPoint)
{
    wsCloseEndPoint(endPoint);
}

void closeSocket(int socket)
{
    wsCloseSocket(socket);
}

void addConfigButton(string name, string display, int flags, string command, string (*callback)(std::string/*name*/,int/*socket*/,std::string/*ip*/,std::string/*client*/))
{
    bool useCb = true;
    if (callback == nullptr)
        useCb = false;
    if (command.substr(0,3) == "hm_")
    {
        command.erase(0,3);
        command = "!" + command;
    }
    else
        command = "!rcon " + command;
    cfgButtons.push_back({name,useCb,display,flags,command,callback});
}

void addConfigButtonCmd(string name, string display, int flags, string command)
{
    if (command.substr(0,3) == "hm_")
    {
        command.erase(0,3);
        command = "!" + command;
    }
    else
        command = "!rcon " + command;
    cfgButtons.push_back({name,false,display,flags,command,nullptr});
}

void addConfigButtonCallback(string name, string display, int flags, string (*callback)(std::string/*name*/,int/*socket*/,std::string/*ip*/,std::string/*client*/))
{
    cfgButtons.push_back({name,true,display,flags,"",callback});
}

}

int configConnect(string endPoint, int socket, string ip, string client)
{
    int flags = hmGetPlayerData(client).flags;
    // send buttons
    int i = 0;
    string list = "";// = "<center><h1>";
    for (auto it = cfgButtons.begin(), ite = cfgButtons.end();it != ite;++it)
    {
        if ((flags & it->flags) == it->flags)
        {
            if (!i)
                list.append("<center><h1>");
            //<input type="submit" value="[Button 1]" style="background: #c48321; border: none; color: #2a2d2d;">
            list.append("<input type=\"submit\" value=\"" + it->display + "\" style=\"background: #c48321; border: none; color: #2a2d2d;\" onclick=\"window.wsButtonConnection.send('" + it->name + "')\"> ");
            i = (i+1) % 3;
            if (!i)
                list.append("</h1></center>\n");
        }
    }
    if (i)
        list.append("</h1></center>\n");
    wsSendToSocket(socket,list);
    wsSendToSocket(socket,"complete");
    return 0;
}

int configReceive(string endPoint, int socket, string ip, string client, string message)
{
    //cout<<"[::] "<<client<<": "<<message<<endl;
    int flags = hmGetPlayerData(client).flags;
    string out;
    for (auto it = cfgButtons.begin(), ite = cfgButtons.end();it != ite;++it)
    {
        if ((flags & it->flags) == it->flags)
        {
            if (it->name == message)
            {
                if (it->command.size() > 0)
                {
                    //if (it->command.substr(0,3) == "hm_")
                    hmSendRaw("hs raw [99:99:99] [Server thread/INFO]: <" + client + "> " + it->command);
                    out = "Ran command: hm_" + it->command.substr(1,it->command.size()-1);
                }
                if (it->useCb)
                    out = (*it->callback)(message,socket,ip,client);  
                wsSendToSocket(socket,out);
                out.clear();
            }
        }
    }
    return 0;
}

void setupConfig()
{
    wsCreateEndPoint("/panel/ws/config",&configConnect,&configReceive);
}

