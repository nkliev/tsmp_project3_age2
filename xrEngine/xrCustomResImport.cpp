#include "stdafx.h"
#include "resource.h"
#include "..\xrCustomRes\resource.h"

HMODULE hCustomRes = nullptr;
const u32 bufferSize = 128;
char textBuf[bufferSize];

void TryLoadXrCustomResDll()
{
	hCustomRes = LoadLibraryA("xrCustomRes.dll");
}

// ������ ������� �� ��������, ����������� �� xrCustomRes.dll
void TryToChangeLogoImageToCustom(HWND logoWindow)
{
	if (hCustomRes)
	{		
		HWND hStatic = GetDlgItem(logoWindow, IDC_STATIC_PICTURE);
		HBITMAP hBmp = LoadBitmap(hCustomRes, MAKEINTRESOURCE(IDB_LOGO_BITMAP));
		SendMessage(hStatic, STM_SETIMAGE, static_cast<WPARAM>(IMAGE_BITMAP), (LPARAM)hBmp);
	}
}

const char *TryToGetNewWindowText()
{
	if (!hCustomRes)
		return nullptr;

	LoadString(hCustomRes, IDS_STRING_APPNAME, textBuf, bufferSize);
	return textBuf;
}

HICON TryToGetNewAppIcon()
{
	if (!hCustomRes)
		return nullptr;

	return LoadIcon(hCustomRes, MAKEINTRESOURCE(IDI_APP_ICON));
}

s64 TryToGetDiscordAppID()
{
	if (!hCustomRes)
		return 0;

	char buffer[bufferSize]{'\0'};

	if (!LoadString(hCustomRes, IDS_STRING_DISCORD_APP_ID, buffer, bufferSize) || buffer[0] == '\0')
		return 0;

	return atoll(buffer);
}
