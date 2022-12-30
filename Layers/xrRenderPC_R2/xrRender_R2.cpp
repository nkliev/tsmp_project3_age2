// xrRender_R2.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "../xrRender/xrRender_console.h"
#include "../xrRender/dxRenderFactory.h"
#include "../xrRender/dxUIRender.h"
#include "../xrRender/dxDebugRender.h"

#pragma comment(lib, "xrEngine")

BOOL APIENTRY DllMain(HANDLE hModule,
					  DWORD ul_reason_for_call,
					  LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (!xrRender_test_hw())
			return FALSE;
		::Render = &RImplementation;
		::RenderFactory = &RenderFactoryImpl;
		::DU = &DUImpl;
		UIRender = &UIRenderImpl;

#ifdef DEBUG
		DRender = &DebugRenderImpl;
#endif // DEBUG

		xrRender_initconsole();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
