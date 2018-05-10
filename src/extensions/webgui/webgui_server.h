#ifndef webgui_server
#define webgui_server

#define MAXCLIENTS  30
#define LOCALHOST   "127.0.0.1"
#define DEFPORT     9001
#define CONF        "./halfMod/plugins/webgui/settings.conf"

#include "halfmod.h"

struct wsEndPoint
{
    std::string point;
    int (*connectCallback)(std::string,int,std::string,std::string);
    int (*receiveCallback)(std::string,int,std::string,std::string,std::string);
};

short webguiInit(std::string ip = LOCALHOST, int port = DEFPORT, int max = MAXCLIENTS);
void webguiRun();
void webguiStop();
std::string buildPath(std::string path);
int hex2dec(std::string hex);
std::string hex2txt(std::string in);
bool isEndPoint(std::string endPoint);
void sendToWebSockets(std::string message);
void wsSendToEndPoint(std::string endPoint, std::string message);
void wsSendToSocket(int sock, std::string message);
void wsCreateEndPoint(std::string endPoint, int (*connectCallback)(std::string,int,std::string,std::string), int (*receiveCallback)(std::string,int,std::string,std::string,std::string));
void wsCloseEndPoint(std::string endPoint);
void wsCloseSocket(int sock);
void disconnectEndPoint(std::string endPoint);
void handleConnectCallback(std::string endPoint, int sock, std::string ip, std::string player);
void handleReceiveCallback(std::string endPoint, int sock, std::string ip, std::string player, std::string message);

#endif

