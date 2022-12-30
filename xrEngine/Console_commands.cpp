#include "stdafx.h"
#include "igame_level.h"
#include "console.h"
#include "console_commands.h"
#include "fbasicvisual.h"
#include "cameramanager.h"
#include "environment.h"
#include "xr_input.h"
#include "CustomHUD.h"
#include "SkeletonAnimated.h"
#include "ResourceManager.h"
#include "xr_object.h"
#include "..\TSMP3_Build_Config.h"

extern ENGINE_API const char* RadminIdPrefix = "raid:";

void ExcludeRadminIdFromCommandArguments(char* args)
{
	char* pSearchFrom = strrchr(args, ' ');

	if (!pSearchFrom)
		pSearchFrom = args;

	if (char* raidStr = strstr(pSearchFrom, RadminIdPrefix))
	{
		if (raidStr <= args)
			*raidStr = '\0';
		else
			*(raidStr - 1) = '\0';	//with ' '
	}
}

extern xr_token *vid_mode_token;

xr_token renderer_type_token[] =
	{
		{"renderer_r1", 0},
		{"renderer_r2a", 1},
		{"renderer_r2", 2},
		{0, 0}};

xr_token vid_bpp_token[] =
	{
		{"16", 16},
		{"32", 32},
		{0, 0}};

void IConsole_Command::add_to_LRU(shared_str const& arg)
{
	if (arg.size() == 0 || bEmptyArgsHandled)	
		return;	

	if (std::find(m_LRU.begin(), m_LRU.end(), arg) != m_LRU.end())
		return;

	m_LRU.push_back(arg);

	if (m_LRU.size() > LRU_MAX_COUNT)
		m_LRU.erase(m_LRU.begin());
}

void IConsole_Command::add_LRU_to_tips(vecTips &tips)
{
	for (shared_str &StrLR : m_LRU)
		tips.push_back(StrLR);
}

class CCC_Quit : public IConsole_Command
{
public:
	CCC_Quit(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Console->Hide();
		Engine.Event.Defer("KERNEL:disconnect");
		Engine.Event.Defer("KERNEL:quit");
	}
};

#ifdef DEBUG_MEMORY_MANAGER
class CCC_MemStat : public IConsole_Command
{
public:
	CCC_MemStat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		string_path fn;

		if (args && args[0])
			sprintf_s(fn, sizeof(fn), "%s.dump", args);
		else
			strcpy_s_s(fn, sizeof(fn), "x:\\$memory$.dump");

		Memory.mem_statistic(fn);
	}
};

class CCC_DbgMemCheck : public IConsole_Command
{
public:
	CCC_DbgMemCheck(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		if (Memory.debug_mode)
			Memory.dbg_check();
		else
			Msg("~ Run with -mem_debug options.");
	}
};
#endif // DEBUG_MEMORY_MANAGER

class CCC_Help : public IConsole_Command
{
public:
	CCC_Help(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Log("- --- Command listing: start ---");
		CConsole::vecCMD_IT it;

		for (it = Console->Commands.begin(); it != Console->Commands.end(); it++)
		{
			IConsole_Command &C = *(it->second);

			if (C.bHidden)
				continue;

			TStatus _S;
			C.Status(_S);
			TInfo _I;
			C.Info(_I);

			Msg("%-20s (%-10s) --- %s", C.Name(), _S, _I);
		}

		Log("- --- Command listing: end ----");
	}
};

class CCC_SaveCFG : public IConsole_Command
{
public:
	CCC_SaveCFG(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		string_path cfg_full_name;
		strcpy_s(cfg_full_name, (xr_strlen(args) > 0) ? args : Console->ConfigFile);

		bool b_abs_name = xr_strlen(cfg_full_name) > 2 && cfg_full_name[1] == ':';

		if (!b_abs_name)
			FS.update_path(cfg_full_name, "$app_data_root$", cfg_full_name);

		if (strext(cfg_full_name))
			*strext(cfg_full_name) = 0;

		strcat(cfg_full_name, ".ltx");
		BOOL b_allow = TRUE;

		if (FS.exist(cfg_full_name))
			b_allow = SetFileAttributes(cfg_full_name, FILE_ATTRIBUTE_NORMAL);

		if (b_allow)
		{
			IWriter *F = FS.w_open(cfg_full_name);
			CConsole::vecCMD_IT it;

			for (it = Console->Commands.begin(); it != Console->Commands.end(); it++)
				it->second->Save(F);

			FS.w_close(F);
			Msg("Config-file [%s] saved successfully", cfg_full_name);
		}
		else
			Msg("!Cannot store config file [%s]", cfg_full_name);
	}
};

CCC_LoadCFG::CCC_LoadCFG(LPCSTR N) : IConsole_Command(N){};

void CCC_LoadCFG::Execute(LPCSTR args)
{
	Msg("Executing config-script \"%s\"...", args);
	string_path cfg_name;

	strcpy_s(cfg_name, args);

	if (strext(cfg_name))
		*strext(cfg_name) = 0;

	strcat(cfg_name, ".ltx");
	string_path cfg_full_name;

	FS.update_path(cfg_full_name, "$app_data_root$", cfg_name);

	if (NULL == FS.exist(cfg_full_name))
		strcpy_s(cfg_full_name, cfg_name);

	IReader *F = FS.r_open(cfg_full_name);

	string1024 str;

	if (F)
	{
		while (!F->eof())
		{
			F->r_string(str, sizeof(str));

			if (allow(str))
				Console->Execute(str);
		}

		FS.r_close(F);
		Msg("[%s] successfully loaded.", cfg_full_name);
	}
	else
		Msg("! Cannot open script file [%s]", cfg_full_name);
}

CCC_LoadCFG_custom::CCC_LoadCFG_custom(LPCSTR cmd)
	: CCC_LoadCFG(cmd)
{
	strcpy_s(m_cmd, cmd);
};

bool CCC_LoadCFG_custom::allow(LPCSTR cmd)
{
	return (cmd == strstr(cmd, m_cmd));
};

class CCC_Start : public IConsole_Command
{
	void parse(LPSTR dest, LPCSTR args, LPCSTR name)
	{
		dest[0] = 0;

		if (strstr(args, name))
			sscanf(strstr(args, name) + xr_strlen(name), "(%[^)])", dest);
	}

public:
	CCC_Start(LPCSTR N) : IConsole_Command(N){};

	virtual void Execute(LPCSTR args)
	{
		string4096 op_server, op_client;
		op_server[0] = 0;
		op_client[0] = 0;

		parse(op_server, args, "server"); // 1. server
		parse(op_client, args, "client"); // 2. client

		if (!op_client[0] && strstr(op_server, "single"))
			strcpy_s(op_client, "localhost");

		if (0 == xr_strlen(op_client))
		{
			Log("! Can't start game without client. Arguments: '%s'.", args);
			return;
		}

		Engine.Event.Defer("KERNEL:start", u64(xr_strlen(op_server) ? xr_strdup(op_server) : 0), u64(xr_strdup(op_client)));
	}
};

class CCC_Disconnect : public IConsole_Command
{
public:
	CCC_Disconnect(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Engine.Event.Defer("KERNEL:disconnect");
	}
};

class CCC_VID_Reset : public IConsole_Command
{
public:
	CCC_VID_Reset(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		if (Device.b_is_Ready)
			Device.Reset();
	}
};

class CCC_VidMode : public CCC_Token
{
	u32 _dummy;

public:
	CCC_VidMode(LPCSTR N) : CCC_Token(N, &_dummy, NULL) { bEmptyArgsHandled = FALSE; };
	virtual void Execute(LPCSTR args)
	{
		u32 _w, _h;
		int cnt = sscanf(args, "%dx%d", &_w, &_h);

		if (cnt == 2)
		{
			psCurrentVidMode[0] = _w;
			psCurrentVidMode[1] = _h;
		}
		else
		{
			Msg("! Wrong video mode [%s]", args);
			return;
		}
	}
	virtual void Status(TStatus &S)
	{
		sprintf_s(S, sizeof(S), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);
	}
	virtual xr_token *GetToken() { return vid_mode_token; }
	virtual void Info(TInfo &I)
	{
		strcpy_s(I, sizeof(I), "change screen resolution WxH");
	}
};

class CCC_SND_Restart : public IConsole_Command
{
public:
	CCC_SND_Restart(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Sound->_restart();
	}
};

float ps_gamma = 1.f, ps_brightness = 1.f, ps_contrast = 1.f;

class CCC_Gamma : public CCC_Float
{
public:
	CCC_Gamma(LPCSTR N, float *V) : CCC_Float(N, V, 0.5f, 1.5f) {}

	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute(args);
		Device.m_pRender->setGamma(ps_gamma);
		Device.m_pRender->setBrightness(ps_brightness);
		Device.m_pRender->setContrast(ps_contrast);
		Device.m_pRender->updateGamma();
	}
};

extern INT g_bDR_LM_UsePointsBBox;
extern INT g_bDR_LM_4Steps;
extern INT g_iDR_LM_Step;
extern Fvector g_DR_LM_Min, g_DR_LM_Max;

class CCC_DR_ClearPoint : public IConsole_Command
{
public:
	CCC_DR_ClearPoint(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		g_DR_LM_Min.x = 1000000.0f;
		g_DR_LM_Min.z = 1000000.0f;

		g_DR_LM_Max.x = -1000000.0f;
		g_DR_LM_Max.z = -1000000.0f;

		Msg("Local BBox (%f, %f) - (%f, %f)", g_DR_LM_Min.x, g_DR_LM_Min.z, g_DR_LM_Max.x, g_DR_LM_Max.z);
	}
};

class CCC_DR_TakePoint : public IConsole_Command
{
public:
	CCC_DR_TakePoint(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Fvector CamPos = Device.vCameraPosition;

		if (g_DR_LM_Min.x > CamPos.x)
			g_DR_LM_Min.x = CamPos.x;

		if (g_DR_LM_Min.z > CamPos.z)
			g_DR_LM_Min.z = CamPos.z;

		if (g_DR_LM_Max.x < CamPos.x)
			g_DR_LM_Max.x = CamPos.x;

		if (g_DR_LM_Max.z < CamPos.z)
			g_DR_LM_Max.z = CamPos.z;

		Msg("Local BBox (%f, %f) - (%f, %f)", g_DR_LM_Min.x, g_DR_LM_Min.z, g_DR_LM_Max.x, g_DR_LM_Max.z);
	}
};

class CCC_DR_UsePoints : public CCC_Integer
{
public:
	CCC_DR_UsePoints(LPCSTR N, int *V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max){};
	virtual void Save(IWriter *F){};
};

ENGINE_API BOOL r2_sun_static = TRUE;
ENGINE_API BOOL r2_advanced_pp = FALSE;
u32 renderer_value = 0;

class CCC_r2 : public CCC_Token
{
	typedef CCC_Token inherited;

public:
	CCC_r2(LPCSTR N) : inherited(N, &renderer_value, renderer_type_token) { renderer_value = 0; };

	virtual void Execute(LPCSTR args)
	{
#ifdef DEDICATED_SERVER
		inherited::Execute("renderer_r1");
#else
		inherited::Execute(args);
#endif // DEDICATED_SERVER

		psDeviceFlags.set(rsR2, (renderer_value > 0));
		r2_sun_static = (renderer_value != 2);
		r2_advanced_pp = (renderer_value == 2);
	}

	virtual void Save(IWriter *F)
	{
		if (!strstr(Core.Params, "-r2"))
			inherited::Save(F);
	}
};

class CCC_DumpResources : public IConsole_Command
{
public:
	CCC_DumpResources(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Device.Resources->Dump(args != NULL);
	}
};

XRCORE_API void _dump_open_files(int mode);
class CCC_DumpOpenFiles : public IConsole_Command
{
public:
	CCC_DumpOpenFiles(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = FALSE; };
	virtual void Execute(LPCSTR args)
	{
		int _mode = atoi(args);
		_dump_open_files(_mode);
	}
};

ENGINE_API float psHUD_FOV = 0.45f;
ENGINE_API int ps_r__Supersample = 1;

extern int g_ErrorLineCount;
extern int psSkeletonUpdate;
extern int rsDVB_Size;
extern int rsDIB_Size;
extern int psNET_ClientUpdate;
extern int psNET_ClientPending;
extern int psNET_ServerUpdate;
extern int psNET_ServerPending;
extern int psNET_DedicatedSleep;
extern int g_svDedicateServerUpdateReate;
extern int g_Dump_Export_Obj;
extern int g_Dump_Import_Obj;
extern Flags32 psEnvFlags;
extern float r__dtex_range;

void RegisterDebugCommands();

void CCC_Register()
{
	// General
	CMD1(CCC_Help, "help");
	CMD1(CCC_Quit, "quit");
	CMD1(CCC_Start, "start");
	CMD1(CCC_Disconnect, "disconnect");
	CMD1(CCC_SaveCFG, "cfg_save");
	CMD1(CCC_LoadCFG, "cfg_load");

#ifdef DEBUG_MEMORY_MANAGER
	CMD1(CCC_MemStat, "dbg_mem_dump");
	CMD1(CCC_DbgMemCheck, "dbg_mem_check");
#endif // DEBUG_MEMORY_MANAGER

	// Render device states
	CMD4(CCC_Integer, "r__supersample", &ps_r__Supersample, 1, 4);

	CMD3(CCC_Mask, "rs_v_sync", &psDeviceFlags, rsVSync);
	CMD3(CCC_Mask, "rs_fullscreen", &psDeviceFlags, rsFullscreen);
	CMD3(CCC_Mask, "rs_refresh_60hz", &psDeviceFlags, rsRefresh60hz);
	CMD3(CCC_Mask, "rs_stats", &psDeviceFlags, rsStatistic);
	CMD4(CCC_Float, "rs_vis_distance", &psVisDistance, 0.4f, 1.5f);

	CMD2(CCC_Gamma, "rs_c_gamma", &ps_gamma);
	CMD2(CCC_Gamma, "rs_c_brightness", &ps_brightness);
	CMD2(CCC_Gamma, "rs_c_contrast", &ps_contrast);

	// Texture manager
	CMD4(CCC_Integer, "texture_lod", &psTextureLOD, 0, 4);
	CMD4(CCC_Integer, "net_dedicated_sleep", &psNET_DedicatedSleep, 0, 64);

	// General video control
	CMD1(CCC_VidMode, "vid_mode");
	CMD1(CCC_VID_Reset, "vid_restart");

	// Sound
	CMD2(CCC_Float, "snd_volume_eff", &psSoundVEffects);
	CMD2(CCC_Float, "snd_volume_music", &psSoundVMusic);
	CMD1(CCC_SND_Restart, "snd_restart");
	CMD3(CCC_Mask, "snd_acceleration", &psSoundFlags, ss_Hardware);
	CMD3(CCC_Mask, "snd_efx", &psSoundFlags, ss_EAX);
	CMD4(CCC_Integer, "snd_targets", &psSoundTargets, 4, 32);
	CMD4(CCC_Integer, "snd_cache_size", &psSoundCacheSizeMB, 4, 32);

	// Mouse
	CMD3(CCC_Mask, "mouse_invert", &psMouseInvert, 1);
	CMD4(CCC_Float, "mouse_sens", &psMouseSens, 0.05f, 0.6f);

	// Camera
	CMD2(CCC_Float, "cam_inert", &psCamInert);
	CMD2(CCC_Float, "cam_slide_inert", &psCamSlideInert);

	CMD1(CCC_r2, "renderer");

	CMD4(CCC_Integer, "net_dbg_dump_export_obj", &g_Dump_Export_Obj, 0, 1);
	CMD4(CCC_Integer, "net_dbg_dump_import_obj", &g_Dump_Import_Obj, 0, 1);

#pragma todo("tsmp: ������ �� ��� �������� � ������ �����? :D")

	psSoundOcclusionScale = pSettings->r_float("sound", "occlusion_scale");
	clamp(psSoundOcclusionScale, 0.1f, .5f);

	if (strstr(Core.Params, "-designer"))
	{
		CMD1(CCC_DR_TakePoint, "demo_record_take_point");
		CMD1(CCC_DR_ClearPoint, "demo_record_clear_points");
		CMD4(CCC_DR_UsePoints, "demo_record_use_points", &g_bDR_LM_UsePointsBBox, 0, 1);
		CMD4(CCC_DR_UsePoints, "demo_record_4step", &g_bDR_LM_4Steps, 0, 1);
		CMD4(CCC_DR_UsePoints, "demo_record_step", &g_iDR_LM_Step, 0, 3);
	}

	CMD1(CCC_DumpResources, "dump_resources");
	CMD1(CCC_DumpOpenFiles, "dump_open_files");

	CMD4(CCC_Integer, "sv_dedicated_server_update_rate", &g_svDedicateServerUpdateReate, 1, 1000);
	CMD3(CCC_Mask, "rs_cam_pos", &psDeviceFlags, rsCameraPos);

	RegisterDebugCommands();
}

// Debug mode commands

#ifdef DEBUG
class CCC_DbgStrCheck : public IConsole_Command
{
public:
	CCC_DbgStrCheck(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { g_pStringContainer->verify(); }
};

class CCC_DbgStrDump : public IConsole_Command
{
public:
	CCC_DbgStrDump(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { g_pStringContainer->dump(); }
};

class CCC_MotionsStat : public IConsole_Command
{
public:
	CCC_MotionsStat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { g_pMotionsContainer->dump(); }
};

class CCC_TexturesStat : public IConsole_Command
{
public:
	CCC_TexturesStat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { Device.Resources->_DumpMemoryUsage(); }
};

class CCC_E_Dump : public IConsole_Command
{
public:
	CCC_E_Dump(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { Engine.Event.Dump(); }
};

class CCC_E_Signal : public IConsole_Command
{
public:
	CCC_E_Signal(LPCSTR N) : IConsole_Command(N){};
	virtual void Execute(LPCSTR args)
	{
		char Event[128], Param[128];
		Event[0] = 0;
		Param[0] = 0;
		sscanf(args, "%[^,],%s", Event, Param);
		Engine.Event.Signal(Event, (u64)Param);
	}
};

void crashthread(void *)
{
	Sleep(1000);
	Msg("~ crash thread activated");
	u64 clk = CPU::GetCLK();
	CRandom rndg;
	rndg.seed(s32(clk));

	for (;;)
	{
		Sleep(1);
		__try
		{
			//try {
			union
			{
				struct
				{
					u8 _b0;
					u8 _b1;
					u8 _b2;
					u8 _b3;
				};
				uintptr_t _ptri;
				u32 *_ptr;
			} rndptr;
			rndptr._b0 = u8(rndg.randI(0, 256));
			rndptr._b1 = u8(rndg.randI(0, 256));
			rndptr._b2 = u8(rndg.randI(0, 256));
			rndptr._b3 = u8(rndg.randI(0, 256));
			rndptr._ptri &= (1ul < 31ul) - 1;
			*rndptr._ptr = 0xBAADF00D;
			//} catch(...) {
			//	// OK
			//}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			// OK
		}
	}
}

class CCC_Crash : public IConsole_Command
{
public:
	CCC_Crash(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { thread_spawn(crashthread, "crash", 0, 0); }
};

#endif // DEBUG

void RegisterDebugCommands()
{
#ifdef DEBUG
	CMD1(CCC_Crash, "crash");
	CMD1(CCC_MotionsStat, "stat_motions");
	CMD1(CCC_TexturesStat, "stat_textures");

	CMD3(CCC_Mask, "mt_particles", &psDeviceFlags, mtParticles);

	CMD1(CCC_DbgStrCheck, "dbg_str_check");
	CMD1(CCC_DbgStrDump, "dbg_str_dump");

	CMD3(CCC_Mask, "mt_sound", &psDeviceFlags, mtSound);
	CMD3(CCC_Mask, "mt_physics", &psDeviceFlags, mtPhysics);
	CMD3(CCC_Mask, "mt_network", &psDeviceFlags, mtNetwork);

	// Events
	CMD1(CCC_E_Dump, "e_list");
	CMD1(CCC_E_Signal, "e_signal");

	CMD3(CCC_Mask, "rs_wireframe", &psDeviceFlags, rsWireframe);
	CMD3(CCC_Mask, "rs_clear_bb", &psDeviceFlags, rsClearBB);
	CMD3(CCC_Mask, "rs_occlusion", &psDeviceFlags, rsOcclusion);

	CMD3(CCC_Mask, "rs_detail", &psDeviceFlags, rsDetails);
	CMD4(CCC_Float, "r__dtex_range", &r__dtex_range, 5, 175);

	CMD3(CCC_Mask, "rs_constant_fps", &psDeviceFlags, rsConstantFPS);
	CMD3(CCC_Mask, "rs_render_statics", &psDeviceFlags, rsDrawStatic);
	CMD3(CCC_Mask, "rs_render_dynamics", &psDeviceFlags, rsDrawDynamic);

	CMD3(CCC_Mask, "rs_disable_objects_as_crows", &psDeviceFlags, rsDisableObjectsAsCrows);
		
	CMD3(CCC_Mask, "rs_occ_draw", &psDeviceFlags, rsOcclusionDraw);
	CMD3(CCC_Mask, "rs_occ_stats", &psDeviceFlags, rsOcclusionStats);
	CMD4(CCC_Integer, "rs_skeleton_update", &psSkeletonUpdate, 2, 128);

	CMD4(CCC_Integer, "rs_vb_size", &rsDVB_Size, 32, 4096);
	CMD4(CCC_Integer, "rs_ib_size", &rsDIB_Size, 32, 4096);

	CMD3(CCC_Token, "vid_bpp", &psCurrentBPP, vid_bpp_token);

	CMD3(CCC_Mask, "snd_stats", &g_stats_flags, st_sound);
	CMD3(CCC_Mask, "snd_stats_min_dist", &g_stats_flags, st_sound_min_dist);
	CMD3(CCC_Mask, "snd_stats_max_dist", &g_stats_flags, st_sound_max_dist);
	CMD3(CCC_Mask, "snd_stats_ai_dist", &g_stats_flags, st_sound_ai_dist);
	CMD3(CCC_Mask, "snd_stats_info_name", &g_stats_flags, st_sound_info_name);
	CMD3(CCC_Mask, "snd_stats_info_object", &g_stats_flags, st_sound_info_object);

	CMD4(CCC_Integer, "error_line_count", &g_ErrorLineCount, 6, 1024);
#endif // DEBUG
}
