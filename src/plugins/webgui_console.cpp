#include "halfmod.h"
#include <fstream>
#include "str_tok.h"
using namespace std;

#define VERSION "v0.1.3"

static string threadBuffer;

bool keepInvSetting = false;

static void (*sendToEndPoint)(string,string);
static void (*sendToSocket)(int,string);
static void (*regEndPoint)(string,int (*)(std::string,int,std::string,std::string),int (*)(std::string,int,std::string,std::string,std::string));
static void (*closeEndPoint)(string);

/*data get entity nigathan Inventory
[09:18:33] [Server thread/INFO]: nigathan has the following entity data: []
data get entity nigathan Inventory
[09:18:51] [Server thread/INFO]: nigathan has the following entity data: [{Slot: 0b, id: "minecraft:dirt", Count: 1b}]*/
int getPlayerInv(hmHandle &handle, hmHook hook, smatch args)
{
    string client = lower(args[1].str());
    string inv = args[2].str();
    ofstream file ("./halfMod/plugins/webgui/inventory/" + client,ios_base::trunc);
    if (file.is_open())
    {
        file<<inv;
        file.close();
    }
    handle.unhookPattern(hook.name);
    return 0;
}

int onEndPointRecv(string endPoint, int socket, string ip, string client, string message)
{
    hmSendRaw("hs raw [99:99:99] [Server thread/INFO]: <" + client + "> !rcon " + message);
    return 0;
}

int onEndPointConnect(string endPoint, int socket, string ip, string client)
{
    ifstream file ("./halfMod/plugins/webgui/console.log");
    if (file.is_open())
    {
        string buffer;
        while (getline(file,buffer))
            sendToSocket(socket,buffer);
        file.close();
    }
    return 0;
}

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Web GUI Console",
                      "nigel",
                      "Server console in the Web GUI",
                      VERSION,
                      "http://localhost:9001/panel/console");
    handle.createTimer("writeBuf",1,"writeThreadBuffer");
    handle.createTimer("writeVar",10,"writeVariables");
    remove("./halfMod/plugins/webgui/console.log");
    mkdirIf("./halfMod/plugins/webgui/inventory");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&sendToEndPoint) = it->getFunc("sendToEndPoint");
            *(void **) (&sendToSocket) = it->getFunc("sendToSocket");
            *(void **) (&regEndPoint) = it->getFunc("regEndPoint");
            *(void **) (&closeEndPoint) = it->getFunc("closeEndPoint");
            (*regEndPoint)("/panel/ws/console",&onEndPointConnect,&onEndPointRecv);
            return 0; // extension found so return no error
        }
    }
    return 1; // return error if the webgui extension isn't loaded
}

int onPluginsLoaded(hmHandle &handle)
{
    hmSendRaw("gamerule keepInventory");
    return 0;
}

int onGamerule(hmHandle &handle, smatch args)
{
    if (args[1].str() == "keepInventory")
    {
        if (args[3].str() == "true")
            keepInvSetting = true;
        else
            keepInvSetting = false;
    }
    return 0;
}

int onPlayerJoin(hmHandle &handle, smatch args)
{
    string client = args[1].str();
    handle.hookPattern(client + "Inv","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (" + client + ") has the following entity data: \\[(.*)\\]$",&getPlayerInv);
    hmSendRaw("data get entity " + client + " Inventory");
    return 0;
}

int onPlayerDeath(hmHandle &handle, smatch args)
{
    string client = args[1].str();
    if (!keepInvSetting)
    {
        ofstream file ("./halfMod/plugins/webgui/inventory/" + lower(client),ios_base::trunc);
        if (file.is_open())
            file.close();
    }
    else
    {
        handle.hookPattern(client + "Inv","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (" + client + ") has the following entity data: \\[(.*)\\]$",&getPlayerInv);
        hmSendRaw("data get entity " + client + " Inventory");
    }
    return 0;
}

int onConsoleReceive(hmHandle &handle, smatch args)
{
    string buffer = strreplace(strreplace(args[0].str(),"<","&lt;"),">","&gt;");
    if (threadBuffer.size() > 0)
        threadBuffer = threadBuffer + "\n" + buffer;
    else
        threadBuffer = buffer;
    sendToEndPoint("/panel/ws/console",buffer);
    return 0;
}

int onGlobalMessage(hmHandle &handle, smatch args)
{
    string buffer = strreplace(strreplace(args[1].str(),"<","&lt;"),">","&gt;");
    if (threadBuffer.size() > 0)
        threadBuffer = threadBuffer + "\n" + buffer;
    else
        threadBuffer = buffer;
    sendToEndPoint("/panel/ws/console",buffer);
    return 0;
}

int onPrintMessage(hmHandle &handle, smatch args)
{
    string buffer = strreplace(strreplace(args[1].str(),"<","&lt;"),">","&gt;");
    if (threadBuffer.size() > 0)
        threadBuffer = threadBuffer + "\n" + buffer;
    else
        threadBuffer = buffer;
    sendToEndPoint("/panel/ws/console",buffer);
    return 0;
}

int writeThreadBuffer(hmHandle &handle, string args)
{
    if (threadBuffer.size() > 0)
    {
        ofstream file ("./halfMod/plugins/webgui/console.log", ios::out|ios::app);
        if (file.is_open())
        {
            file<<threadBuffer<<"\n";
            file.close();
        }
        threadBuffer.clear();
    }
    return 0;
}

int writeVariables(hmHandle &handle, string args)
{
    hmGlobal *global = recallGlobal(NULL);
    ofstream file ("./halfMod/plugins/webgui/variables.sh", ios::out|ios::trunc);
    if (file.is_open())
    {
        file<<"#!/bin/bash";
        file<<"\nmc_version=\""<<global->mcVer<<"\"\nhm_version=\""<<global->hmVer<<"\"\nhs_version=\""<<global->hsVer<<"\"\nworld_name="<<global->world<<"\nscreen_name=\""<<global->mcScreen<<"\"";
        file<<"\nopt_quiet="<<(int)global->quiet<<"\nopt_verbose="<<(int)global->verbose<<"\nopt_debug="<<(int)global->debug<<"\nlog_method="<<global->logMethod;
        file<<"\nmax_players="<<global->maxPlayers<<"\nplayers_count="<<global->players.size()<<"\nadmins_count="<<global->admins.size()<<"\nplugins_count="<<global->pluginList.size()<<"\nconsole_filter_count="<<global->conFilter->size();
        string pname = "\nplayers_name=( ";
        string puuid = "\nplayers_uuid=( ";
        string pip = "\nplayers_ip=( ";
        string pflag = "\nplayers_flags=( ";
        string pjoin = "\nplayers_join_time=( ";
        string pdeath = "\nplayers_death_time=( ";
        string pdmsg = "\nplayers_death_msg=( ";
        for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
        {
            pname = pname + it->second.name + " ";
            puuid = puuid + it->second.uuid + " ";
            pip = pip + it->second.ip + " ";
            pflag = pflag + to_string(it->second.flags) + " ";
            pjoin = pjoin + to_string(it->second.join) + " ";
            pdeath = pdeath + to_string(it->second.death) + " ";
            if (it->second.deathmsg.size() < 1)
                pdmsg += "null ";
            else
                pdmsg = pdmsg + "\"" + it->second.deathmsg + "\" ";
        }
        file<<pname<<")"<<puuid<<")"<<pip<<")"<<pflag<<")"<<pjoin<<")"<<pdeath<<")"<<pdmsg<<")";
        pname = "\nadmins_client=( ";
        pflag = "\nadmins_flags=( ";
        for (auto it = global->admins.begin(), ite = global->admins.end();it != ite;++it)
        {
            pname = pname + it->second.client + " ";
            pflag = pflag + to_string(it->second.flags) + " ";
        }
        file<<pname<<")"<<pflag<<")";
        puuid = "\nplugins_path=( ";
        pname = "\nplugins_name=( ";
        pflag = "\nplugins_version=( ";
        for (auto it = global->pluginList.begin(), ite = global->pluginList.end();it != ite;++it)
        {
            puuid = puuid + "\"" + it->path + "\" ";
            pname = pname + "\"" + it->name + "\" ";
            pflag = pflag + "\"" + it->version + "\" ";
        }
        file<<puuid<<")"<<pname<<")"<<pflag<<")";
        /*pname = "\nconsole_filter_name=( ";
        puuid = "\nconsole_filter_ptrn=( ";
        pflag = "\nconsole_filter_event=( ";
        pip = "\nconsole_filer_block_output=( ";
        pjoin = "\nconsole_filter_block_event=( ";
        pdeath = "\nconsole_filter_block_hook=( ";
        for (auto it = global->conFilter->begin(), ite = global->conFilter->end();it != ite;++it)
        {
            pname = pname + "\"" + it->name + "\" ";
            puuid = puuid + "\"" + it->filter + "\" ";
            pflag = pflag + to_string(it->event) + " ";
            // realized the filter is not a string, leaving all of this out till later . . .
        }*/
        file.close();
    }
    return 0;
}

int onPlayerConnect(hmHandle &handle, smatch args)
{
    string ip = args[2].str() + "=" + stripFormat(args[1].str()), line;
    bool write = true;
    fstream file ("./halfMod/plugins/webgui/userlist.ip", ios::in);
    if (file.is_open())
    {
        while (getline(file,line))
        {
            if (line == ip)
            {
                write = false;
                break;
            }
        }
        file.close();
    }
    file.clear();
    if (write)
    {
        file.open("./halfMod/plugins/webgui/userlist.ip", ios::out|ios::app);
        if (file.is_open())
        {
            file<<"\n"<<ip;
            file.close();
        }
    }
    return 0;
}

void onPluginEnd(hmHandle &handle)
{
    closeEndPoint("/panel/ws/console");
}

}

