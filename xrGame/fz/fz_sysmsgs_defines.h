#pragma once

#include "windows.h"

struct SMyUserData
{
    xrServer *server;
    ClientID idOfPlayer;
};

#pragma pack(push, 1)

using FZSysmsgPayloadWriter = void *;
using FZSysMsgSender = void *;
using FZSysMsgsProcessClientModDll = void *;
using FZSysMsgsProcessClientMap = void *;

using FZSysMsgsInit = bool(__stdcall *)();
using FZSysMsgsFlags = bool(__stdcall *)();
using FZSysMsgsFree = bool(__stdcall *)();

using FZSysMsgsSendSysMessage_SOC = void(__stdcall *)(void *, void *, FZSysMsgSender, void *);

using FZSysmsgsCommonFlags = int;
const FZSysmsgsCommonFlags FZ_SYSMSGS_ENABLE_LOGS = 1;
const FZSysmsgsCommonFlags FZ_SYSMSGS_PATCH_UI_PROGRESSBAR = 2;

using SetCommonSysmsgsFlags = void(__stdcall*)(FZSysmsgsCommonFlags);

//��� ����������, �������������� � ����������� �� ������� �����
using FZArchiveCompressionType = DWORD;
const FZArchiveCompressionType FZ_COMPRESSION_NO_COMPRESSION = 0;  //���������� ���, ���� �� ������� �� ����
const FZArchiveCompressionType FZ_COMPRESSION_LZO_COMPRESSION = 1; //���� ���� ������������� LZO-������������
const FZArchiveCompressionType FZ_COMPRESSION_CAB_COMPRESSION = 2; //���� ���� ����������� �������� MAKECAB �� ������� Windows

//���������� ������, � ������� ������ ���������� ������� ����
using FZModdingPolicy = DWORD;

//������� �� DLL ���� ����� ���������� � ����� �����. �������������� ��������� �� ������������.
const FZModdingPolicy FZ_MODDING_ANYTIME = 0;

//������� �� DLL ���� ������ ���������� ������ � �������� �������� � ������� (�.�. ��� ������������� ������������� ������������ ��������� ����� ������)
//������ ��� �����, �� ��������� �������� �������
const FZModdingPolicy FZ_MODDING_WHEN_CONNECTING = 1;

//������� �� DLL ���� ������ ���������� ������ ����� ������ �� � ��������� �������� (�.�. ���� ���� �� ������� ��� ���� - ��� ����� ����� ���������� ����������)
//��������������� ���������� �� ������������, ��� ��� �������� �� ��������� � ������ ������������� ��������
//������ � ����: � ������ ������ ����� ����� ����� ���� �� ������ ������! � ���� ������ ��������������� ����������� � ������� ������� ����������� �� �����!
const FZModdingPolicy FZ_MODDING_WHEN_NOT_CONNECTING = 2;

//��������� ������������ �����
struct FZFileDownloadInfo
{
    //��� ����� (������ � �����) �� �������, �� �������� �� ������ ���� ��������
    const char *filename;
    //URL, � �������� ����� ������������� �������� �����
    const char *url;
    //����������� ����� CRC32 ��� ����� (� ������������� ����)
    DWORD crc32;
    //������������ ��� ����������
    FZArchiveCompressionType compression;
    //���������, ��������� ������������ �� ����� �������
    const char *progress_msg;
    //���������, ��������� ������������ ��� ������������� ������ �� ����� �������
    const char *error_already_has_dl_msg;
};

//��������� ���������� ������� � �������
struct FZReconnectInetAddrData
{
    //IPv4-����� ������� (��������, 127.0.0.1)
    const char *ip;
    //���� �������
    DWORD port;
};

using FZMapLoadingFlags = DWORD;
const FZMapLoadingFlags FZ_MAPLOAD_MANDATORY_RECONNECT = 1; //������������ ��������� ����� �������� ��������� ��������� �����

//��������� �������� �������� db-������ � ������
struct FZMapInfo
{
    //��������� �����
    FZFileDownloadInfo fileinfo;
    //IP-����� � ���� ��� ���������� ����� ���������� �������. ���� IP ������, �� ��������� ���������� ������������� ������� ����� �� ���, �� ����� ������� ��������� ����������.
    FZReconnectInetAddrData reconnect_addr;
    //������������� ��� ����������� ����� (��������, mp_pool)
    const char *mapname;
    //������ ����������� ����� (������ 1.0)
    const char *mapver;
    //�������� xml-����� � ��������������� ��������� � ��������� ����� (nil, ���� ����� �� ���������)
    const char *xmlname;
    //����� ��� ��������� ������������ ���������� �����
    FZMapLoadingFlags flags;
};

using FZDllModFunResult = DWORD;
const FZDllModFunResult FZ_DLL_MOD_FUN_SUCCESS_LOCK = 0;   //��� ������� ����������, ��������� �������� ������� �� name_lock
const FZDllModFunResult FZ_DLL_MOD_FUN_SUCCESS_NOLOCK = 1; //�����, ������ ������� (� �������������� name_lock) ���� �� ����
const FZDllModFunResult FZ_DLL_MOD_FUN_FAILURE = 2;        //������ �������� ����

using FZDllModFun = FZDllModFunResult(__stdcall*)(char *procarg1, char *procarg2);

//��������� �������� �������� DLL-���� ����������� ProcessClientModDll
struct FZDllDownloadInfo
{
    //��������� ����� ��� dll ����
    FZFileDownloadInfo fileinfo;

    //��� ��������� � dll ����, ������� ������ ���� �������; ������ ����� ��� FZDllModFun
    const char *procname;

    //��������� ��� �������� � ���������
    const char *procarg1;
    const char *procarg2;

    //�������� ������� ��� ����������� DLL - ����������� ����� ���, ��� �������� ���������� � ������� ����
    const char *dsign;

    //IP-����� � ���� ��� ����������. ���� IP �������, �� ��������� ���������� ������������� ������� ����� �� ���, �� ����� ������� ��������� ����������.
    FZReconnectInetAddrData reconnect_addr;

    //��������� ������� ��������� ����
    FZModdingPolicy modding_policy;

    //�������� ��� �������� � ��������� -fzmod.
    //���� �������� nil - ������ �� �����������.
    //���� �������� ��������� � ������� � ��������� ������ - ��� ��������� ��� �������������, ������� ������������
    //���� �������� �� ��������� � ��������� � ��������� ������ - ���������� ���������� ������
    //���� � ��������� ������ ��������� ��� - ���������� ��������� ����.
    const char *name_lock;

    //������, ��������� ��� ����������� �������������� ����
    const char *incompatible_mod_message;

    //������, ��������� ��� ���������� ���� ����� ����������� (���� ������� FZ_MODDING_WHEN_NOT_CONNECTING)
    const char *mod_is_applying_message;
};

//��������� ��������� ����� ��� ���������� � ������ �����������
struct FZClientVotingElement
{
    //������������� ��� ����� (��������, mp_pool). � ������ ���� nil - ������������ ������� ������ ����!
    //������������� ���������� nil ������ ��������� � ������
    const char *mapname;
    //������ �����
    const char *mapver;
    //�������������� �������� �����; ���� nil, ����� ����������� ��������� ������ ������������ �������������� ����������� �� �������
    const char *description;
};

//��������� ���������� ���� � ������, ��������� ��� �����������, ������������ � ProcessClientVotingMaplist
struct FZClientVotingMapList
{
    //��������� �� ������ �� FZClientVotingElement. ������ ������� ������� �������� ��������� ����� �����,
    //������� ��������� �������� � ������ ����, ��������� ��� �����������
    FZClientVotingElement *maps;

    //����� ��������� � ������� maps
    DWORD count;

    //������������� ���� ����, ��� �������� ��������� �������� ������ ����. � ������ ������� ��� ���� ���������� � � game_GameState.m_type
    DWORD gametype;

    //�������� ��������, ����������, ������� ���� �� ������� ���� ���������� �������, ������ ���� � ������ �������.
    DWORD was_sent;
};

#pragma pack(pop)
