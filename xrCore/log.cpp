#include "stdafx.h"
#pragma hdrstop

#include <time.h>
#include "log.h"

extern BOOL LogExecCB = TRUE;
static string_path logFName = "engine.log";
static BOOL no_log = TRUE;
#ifdef PROFILE_CRITICAL_SECTIONS
static xrCriticalSection logCS(MUTEX_PROFILE_ID(log));
#else  // PROFILE_CRITICAL_SECTIONS
static xrCriticalSection logCS;
#endif // PROFILE_CRITICAL_SECTIONS
xr_vector<shared_str> *LogFile = NULL;
static LogCallback LogCB = 0;

void FlushLog(LPCSTR file_name)
{
	if (no_log)
		return;

	logCS.Enter();

	IWriter *f = FS.w_open(file_name);
	if (f)
	{
		for (u32 it = 0; it < LogFile->size(); it++)
		{
			LPCSTR s = *((*LogFile)[it]);
			f->w_string(s ? s : "");
		}
		FS.w_close(f);
	}

	logCS.Leave();
}

void FlushLog()
{
	FlushLog(logFName);
}

void AddOne(const char *split)
{
	if (!LogFile)
		return;

	logCS.Enter();

#ifndef DEBUG
	if (IsDebuggerPresent())
#endif
	{
		OutputDebugString(split);
		OutputDebugString("\n");
	}
	
	{
		shared_str temp = shared_str(split);
		LogFile->push_back(temp);
	}

	//exec CallBack
	if (LogExecCB && LogCB)
		LogCB(split);

	logCS.Leave();
}

void Log(const char *s)
{
	int i, j;
	char split[1024];

	for (i = 0, j = 0; s[i] != 0; i++)
	{
		if (s[i] == '\n')
		{
			split[j] = 0; // end of line
			if (split[0] == 0)
			{
				split[0] = ' ';
				split[1] = 0;
			}
			AddOne(split);
			j = 0;
		}
		else
		{
			split[j++] = s[i];
		}
	}
	split[j] = 0;
	AddOne(split);
}

void __cdecl Msg(const char *format, ...)
{
	va_list mark;
	string1024 buf;
	va_start(mark, format);
	int sz = _vsnprintf(buf, sizeof(buf) - 1, format, mark);
	buf[sizeof(buf) - 1] = 0;
	va_end(mark);
	if (sz)
		Log(buf);
}

void Log(const char *msg, const char *dop)
{
	char buf[1024];

	if (dop)
		sprintf_s(buf, sizeof(buf), "%s %s", msg, dop);
	else
		sprintf_s(buf, sizeof(buf), "%s", msg);

	Log(buf);
}

void Log(const char *msg, u32 dop)
{
	char buf[1024];

	sprintf_s(buf, sizeof(buf), "%s %d", msg, dop);
	Log(buf);
}

void Log(const char *msg, int dop)
{
	char buf[1024];

	sprintf_s(buf, sizeof(buf), "%s %d", msg, dop);
	Log(buf);
}

void Log(const char *msg, float dop)
{
	char buf[1024];

	sprintf_s(buf, sizeof(buf), "%s %f", msg, dop);
	Log(buf);
}

void Log(const char *msg, const Fvector &dop)
{
	char buf[1024];

	sprintf_s(buf, sizeof(buf), "%s (%f,%f,%f)", msg, dop.x, dop.y, dop.z);
	Log(buf);
}

void Log(const char *msg, const Fmatrix &dop)
{
	char buf[1024];

	sprintf_s(buf, sizeof(buf), "%s:\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n", msg, dop.i.x, dop.i.y, dop.i.z, dop._14_, dop.j.x, dop.j.y, dop.j.z, dop._24_, dop.k.x, dop.k.y, dop.k.z, dop._34_, dop.c.x, dop.c.y, dop.c.z, dop._44_);
	Log(buf);
}

void LogWinErr(const char *msg, long err_code)
{
	Msg("%s: %s", msg, Debug.error2string(err_code));
}

void SetLogCB(LogCallback cb)
{
	LogCB = cb;
}

LPCSTR log_name()
{
	return (logFName);
}

void InitLog()
{
	R_ASSERT(LogFile == NULL);
	LogFile = xr_new<xr_vector<shared_str>>();
}

void CreateLog(BOOL nl)
{
	no_log = nl;
	strconcat(sizeof(logFName), logFName, Core.ApplicationName, "_", Core.UserName, ".log");
	if (FS.path_exist("$logs$"))
		FS.update_path(logFName, "$logs$", logFName);
	if (!no_log)
	{
		IWriter *f = FS.w_open(logFName);
		if (f == NULL)
		{
			MessageBox(NULL, "Can't create log file.", "Error", MB_ICONERROR);
			abort();
		}
		FS.w_close(f);
	}
	LogFile->reserve(128);
}

void CloseLog(void)
{
	FlushLog();
	LogFile->clear();
	xr_delete(LogFile);
}
