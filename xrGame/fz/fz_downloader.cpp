#include "stdAfx.h"
#include "..\xrserver.h"
#include "fz_sysmsgs_defines.h"
#include "..\Level.h"
#include "..\xrGameSpyServer.h"

int fz_downloader_new = 0;
int fz_downloader_enabled = 1;

std::string fz_downloader_mod_name = "tsmp";
std::string fz_downloader_reconnect_ip;
std::string fz_downloader_message = "Preparing mod";

bool LoadedDLL = false;
HMODULE dll;

FZSysMsgsSendSysMessage_SOC ProcSendSysMessage;
FZSysMsgsProcessClientModDll ProcProcessClientMod;

static void __stdcall SendCallback(void *msg, unsigned int len, void *userdata)
{
	SMyUserData *data = (SMyUserData *)userdata;
	xrServer *srv = data->server;
	ClientID ID = data->idOfPlayer;

	srv->IPureServer::SendTo_LL(ID, msg, len, net_flags(TRUE, TRUE, TRUE, TRUE));
}

void LoadDLL()
{
	if (LoadedDLL)
		return;

	dll = LoadLibrary("sysmsgs.dll");

	if (!dll)
	{
		Msg("! error, cant load dll with sysmsgs api");
		return;
	}

	FZSysMsgsInit SysInit = reinterpret_cast<FZSysMsgsInit>(GetProcAddress(dll, "FZSysMsgsInit"));
	(*SysInit)();

	ProcSendSysMessage = reinterpret_cast<FZSysMsgsSendSysMessage_SOC>(GetProcAddress(dll, "FZSysMsgsSendSysMessage_SOC"));
	ProcProcessClientMod = reinterpret_cast<FZSysMsgsProcessClientModDll>(GetProcAddress(dll, "FZSysMsgsProcessClientModDll"));

	auto SetFlags = reinterpret_cast<SetCommonSysmsgsFlags>(GetProcAddress(dll, "FZSysMsgsSetCommonSysmsgsFlags"));
	SetFlags(FZ_SYSMSGS_ENABLE_LOGS | FZ_SYSMSGS_PATCH_UI_PROGRESSBAR);

	LoadedDLL = true;
	Msg("- sysmsgs.dll loaded successfully");
}

void UnloadSysmsgsDLL()
{
#pragma TODO("TSMP: ����������� ��� ��� �������� ��� �������� FZ ����������!")
	// ��� ����� ���������� �� ��� ��������
	//if (LoadedDLL)
	//{
	//	FZSysMsgsFree FreeFZ = (FZSysMsgsFree)GetProcAddress(dll, "FZSysMsgsFree");
	//	FreeFZ();

	//	FreeLibrary(dll);
	//}

	LoadedDLL = false;
}

void DownloadingMod(xrServer *server, ClientID const &ID)
{
	SMyUserData userdata = {};
	userdata.idOfPlayer = ID;
	userdata.server = server;

	ip_address Address;
	DWORD dwPort = 0;
	Level().GetServerAddress(Address, &dwPort);
	std::string ModLoadArgs = "-srv " + fz_downloader_reconnect_ip + " -srvport " + std::to_string(dwPort)+" -gamespymode ";

	FZDllDownloadInfo moddllinfo = {};

	if (fz_downloader_new)
	{
		moddllinfo.fileinfo.filename = "fz_mod_loader_tsmp_v1.mod";
		moddllinfo.fileinfo.url = "http://stalker-life.com/stalker_files/mods_shoc/tsmp3/loader/tsmp_mod_loader_v1.dll";
		moddllinfo.fileinfo.crc32 = 0x1331A4CD; // crc �����
		
		//�������� ������� ��� ����������� DLL - ����������� ����� ���, ��� �������� ���������� � ������� ����
		moddllinfo.dsign = "302D02141C641554517BB48B6FF2372C3C66C18E9A0E4A13021500B2E7AC52970DEB63D3155FE5B76C49CEACDD6E47";

		shared_str srvPasword = static_cast<xrGameSpyServer*>(server)->Password;

		if (srvPasword.size())
			ModLoadArgs += " -psw " + std::string(srvPasword.c_str());

		ModLoadArgs += " -includename -preservemessage ";
	}
	else
	{
		moddllinfo.fileinfo.filename = "";
		moddllinfo.fileinfo.url = "";
		moddllinfo.fileinfo.crc32 = 0x274A4EBD;
		moddllinfo.dsign = "";
	}

	moddllinfo.mod_is_applying_message = fz_downloader_message.c_str();
	moddllinfo.fileinfo.compression = FZ_COMPRESSION_NO_COMPRESSION;
	moddllinfo.procname = "ModLoad";
	moddllinfo.modding_policy = FZ_MODDING_WHEN_NOT_CONNECTING;
	moddllinfo.procarg1 = fz_downloader_mod_name.c_str();

	if (fz_downloader_reconnect_ip.empty())
		fz_downloader_reconnect_ip = Address.to_string().c_str();

	moddllinfo.procarg2 = ModLoadArgs.c_str(); //��������� ��� �������� � ���������
	
	// not used
	moddllinfo.name_lock = "123";				
	moddllinfo.reconnect_addr.ip = "127.0.0.1";
	moddllinfo.reconnect_addr.port = 5445;
	moddllinfo.incompatible_mod_message = "Incompatible";
	moddllinfo.fileinfo.progress_msg = "Progress";
	moddllinfo.fileinfo.error_already_has_dl_msg = "Error happens";

	ProcSendSysMessage(ProcProcessClientMod, &moddllinfo, SendCallback, &userdata);
}

void fz_download_mod(xrServer* server, ClientID const& ID)
{
	if (!fz_downloader_enabled)
		return;

	LoadDLL();

	if (!LoadedDLL)
		return;

	Msg("- Sending magic packet to client");
	DownloadingMod(server, ID);
}
