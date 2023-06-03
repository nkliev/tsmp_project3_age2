#include "stdafx.h"
#include "PlayersBase.h"
#include "NET_Server.h"
#include <WinInet.h>
#include <thread>
#include "..\TSMP3_Build_Config.h"

#pragma comment(lib, "Wininet.lib")

const int MaxResponseLength = 256;
const char *BaseUrl = "http://194.147.90.72:8080/";
static std::string SessionId;

extern std::string UrlEncode(const std::string &str);

bool SendRequest(const char* request, char* responseBuffer)
{
    char url[256];    
    strcpy_s(url, BaseUrl);
    strcat_s(url, request);

    HINTERNET hInetSession = InternetOpen(0, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
    HINTERNET hURL = InternetOpenUrl(hInetSession, url, 0, 0, 0, 0);
    
    DWORD dwBytesRead = 0;
    BOOL bResult = InternetReadFile(hURL, responseBuffer, MaxResponseLength, &dwBytesRead);
    responseBuffer[dwBytesRead] = '\0';

    Msg("- sent request [%s] %s", request, bResult ? "succeded" : "failed");
    if (bResult) Msg("- get response [%s]", responseBuffer);

    InternetCloseHandle(hURL);
    InternetCloseHandle(hInetSession);

    return bResult;
}

int CalculateKey(std::string srvName, std::string srvVer)
{
    auto Hash = [](std::string str)
    {
        int hash = 7;

        for (u32 i = 0; i < str.size(); i++)
        {
            int val = static_cast<unsigned char>(str[i]);
            //Msg("%d", val);
            hash = hash * 31 + val;
        }

        return hash;
    };

    std::string strToHash = srvName + srvVer + "PlayersBase3218";
    return Hash(strToHash);
}

void InitSession(IPureServer* serv)
{
    if (!SessionId.empty())
        return;

    std::string srvName = serv->GetServerName();
    std::string srvVer = TSMP_VERSION;
    std::string key = std::to_string(CalculateKey(srvName, srvVer));
    std::string request = "PlayersBase/v1/StartSession?srv=" + UrlEncode(srvName) + "&key=" + UrlEncode(key) + "&ver=" + UrlEncode(srvVer);

    char response[MaxResponseLength];

    if (!SendRequest(request.c_str(), response))
        return;

    static const std::string SessionIdHeader = "id: ";
    std::string responseStr = response;

    if (responseStr.find(SessionIdHeader) == std::string::npos)
        return;

    std::string temp(responseStr.begin() + SessionIdHeader.size(), responseStr.end());
    SessionId = temp;
}

bool IsBanned(IClient* CL)
{
    if (SessionId.empty())
        return false;

    std::string ip = CL->m_cAddress.to_string().c_str();
    std::string name = CL->name.c_str();

    std::string request = "PlayersBase/v1/IsBanned?ip=" + UrlEncode(ip) +
        "&name=" + UrlEncode(name) +
        "&key=" + UrlEncode(SessionId);

    char response[MaxResponseLength];

    if (!SendRequest(request.c_str(), response))
        return false;

    if (response[0] == '1')
        return true;

    return false;
}

u32 GenUid()
{
	if (SessionId.empty())
		return 0;

	std::string request = "PlayersStats/v1/GenPlayerID?key=" + UrlEncode(SessionId);
	char response[MaxResponseLength];

	if (!SendRequest(request.c_str(), response))
		return 0;

	return std::stoul(response);
}

#pragma TODO("TSMP: remake without creating new thread always")

void CheckPlayerBannedInBase(IClient* cl, IPureServer* serv)
{
    std::thread thread([](IClient*client, IPureServer* srv)
    {
        InitSession(srv); 
        srv->OnPlayersBaseVerifyRespond(client, IsBanned(client));
    }, cl, serv);

   thread.detach();
}

void GenPlayerUID(IClient* cl, IPureServer* serv)
{
	std::thread thread([](IClient* client, IPureServer* srv)
	{
		InitSession(srv);
		srv->OnPlayersBaseGenUID(client, GenUid());
	}, cl, serv);

	thread.detach();
}
