/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "qwsvdef.h"
#include <sys/types.h>
#include <sys/timeb.h>

#ifdef SERVERONLY

#include <winsock.h>
#include <conio.h>

#ifdef MULTITHREAD
#include <process.h>
#endif

#ifndef MINIMAL
//#define USESERVICE
#endif
#define SERVICENAME	DISTRIBUTION"SV"


static HANDLE hconsoleout;





#ifdef _DEBUG
#if _MSC_VER >= 1300
#define CATCHCRASH
#endif
#endif


#ifdef CATCHCRASH
#include "dbghelp.h"
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP) (
	HANDLE hProcess,
	DWORD ProcessId,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

DWORD CrashExceptionHandler (DWORD exceptionCode, LPEXCEPTION_POINTERS exceptionInfo)
{
	char dumpPath[1024];
	HANDLE hProc = GetCurrentProcess();
	DWORD procid = GetCurrentProcessId();
	HANDLE dumpfile;
	HMODULE hDbgHelp;
	MINIDUMPWRITEDUMP fnMiniDumpWriteDump;
	HMODULE hKernel;
	BOOL (WINAPI *pIsDebuggerPresent)(void);

	hKernel = LoadLibrary ("kernel32");
	pIsDebuggerPresent = (void*)GetProcAddress(hKernel, "IsDebuggerPresent");

#ifdef GLQUAKE
	GLVID_Crashed();
#endif

	if (pIsDebuggerPresent ())
	{
		/*if we have a current window, minimize it to bring us out of fullscreen*/
		return EXCEPTION_CONTINUE_SEARCH;
	}

	hDbgHelp = LoadLibrary ("DBGHELP");
	if (hDbgHelp)
		fnMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress (hDbgHelp, "MiniDumpWriteDump");
	else
		fnMiniDumpWriteDump = NULL;

	if (fnMiniDumpWriteDump)
	{
		if (MessageBox(NULL, "KABOOM! We crashed!\nBlame the monkey in the corner.\nI hope you saved your work.\nWould you like to take a dump now?", DISTRIBUTION " Sucks", MB_ICONSTOP|MB_YESNO) != IDYES)
			return EXCEPTION_EXECUTE_HANDLER;

		/*take a dump*/
		GetTempPath (sizeof(dumpPath)-16, dumpPath);
		Q_strncatz(dumpPath, DISTRIBUTION"CrashDump.dmp", sizeof(dumpPath));
		dumpfile = CreateFile (dumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (dumpfile)
		{
			MINIDUMP_EXCEPTION_INFORMATION crashinfo;
			crashinfo.ClientPointers = TRUE;
			crashinfo.ExceptionPointers = exceptionInfo;
			crashinfo.ThreadId = GetCurrentThreadId ();
			if (fnMiniDumpWriteDump(hProc, procid, dumpfile, MiniDumpWithIndirectlyReferencedMemory|MiniDumpWithDataSegs, &crashinfo, NULL, NULL))
			{
				CloseHandle(dumpfile);
				MessageBox(NULL, va("You can find the crashdump at\n%s\nPlease send this file to someone.\n\nWarning: sensitive information (like your current user name) might be present in the dump.\nYou will probably want to compress it.", dumpPath), DISTRIBUTION " Sucks", 0);
				return EXCEPTION_EXECUTE_HANDLER;
			}
		}
	}
	else
		MessageBox(NULL, "Kaboom! Sorry. No MiniDumpWriteDump function.", DISTRIBUTION " Sucks", 0);
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif









void Sys_CloseLibrary(dllhandle_t *lib)
{
	FreeLibrary((HMODULE)lib);
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	int i;
	HMODULE lib;

	lib = LoadLibrary(name);
	if (!lib)
		return NULL;

	if (funcs)
	{
		for (i = 0; funcs[i].name; i++)
		{
			*funcs[i].funcptr = GetProcAddress(lib, funcs[i].name);
			if (!*funcs[i].funcptr)
				break;
		}
		if (funcs[i].name)
		{
			Sys_CloseLibrary((dllhandle_t*)lib);
			lib = NULL;
		}
	}

	return (dllhandle_t*)lib;
}

void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	if (!module)
		return NULL;
	return GetProcAddress((HINSTANCE)module, exportname);
}
#ifdef HLSERVER
char *Sys_GetNameForAddress(dllhandle_t *module, void *address)
{
	//windows doesn't provide a function to do this, so we have to do it ourselves.
	//this isn't the fastest way...
	//halflife needs this function.
	char *base = (char *)module;

	IMAGE_DATA_DIRECTORY *datadir;
	IMAGE_EXPORT_DIRECTORY *block;
	IMAGE_NT_HEADERS *ntheader;
	IMAGE_DOS_HEADER *dosheader = (void*)base;

	int i, j;
	DWORD *funclist;
	DWORD *namelist;
	SHORT *ordilist;

	if (!dosheader || dosheader->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL; //yeah, that wasn't an exe

	ntheader = (void*)(base + dosheader->e_lfanew);
	if (!dosheader->e_lfanew || ntheader->Signature != IMAGE_NT_SIGNATURE)
		return NULL;	//urm, wait, a 16bit dos exe?


	datadir = &ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

	block = (IMAGE_EXPORT_DIRECTORY *)(base + datadir->VirtualAddress);
	funclist = (DWORD*)(base+block->AddressOfFunctions);
	namelist = (DWORD*)(base+block->AddressOfNames);
	ordilist = (SHORT*)(base+block->AddressOfNameOrdinals);
	for (i = 0; i < block->NumberOfFunctions; i++)
	{
		if (base+funclist[i] == address)
		{
			for (j = 0; j < block->NumberOfNames; j++)
			{
				if (ordilist[j] == i)
				{
					return base+namelist[i];
				}
			}
			//it has no name. huh?
			return NULL;
		}
	}
	return NULL;
}
#endif


#ifdef Q2SERVER
static HINSTANCE	game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (!FreeLibrary (game_library))
		Sys_Error ("FreeLibrary failed for game library");
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
	char	name[MAX_OSPATH];
	char	*path;
	char	cwd[MAX_OSPATH];
// _M_X64 should be really loading gamex64.dll
#if defined _M_IX86 || defined _M_X64
	const char *gamename = "gamex86.dll";

#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

#elif defined __amd64__
	const char *gamename = "gameamd.dll";

#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif


#elif defined _M_ALPHA
	const char *gamename = "gameaxp.dll";

#ifdef NDEBUG
	const char *debugdir = "releaseaxp";
#else
	const char *debugdir = "debugaxp";
#endif

#endif

	if (game_library)
		Sys_Error ("Sys_GetGameAPI without Sys_UnloadingGame");

	// check the current debug directory first for development purposes
#ifdef _WIN32
	GetCurrentDirectory(sizeof(cwd), cwd);
#else
	_getcwd (cwd, sizeof(cwd));
#endif
	snprintf (name, sizeof(name), "%s/%s/%s", cwd, debugdir, gamename);
	game_library = LoadLibrary ( name );
	if (game_library)
	{
		Con_DPrintf ("LoadLibrary (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		_snprintf (name, sizeof(name), "%s/%s", cwd, gamename);
		game_library = LoadLibrary ( name );
		if (game_library)
		{
			Con_DPrintf ("LoadLibrary (%s)\n", name);
		}
		else
#endif
		{
			// now run through the search paths
			path = NULL;
			while (1)
			{
				path = COM_NextPath (path);
				if (!path)
					return NULL;		// couldn't find one anywhere
				snprintf (name, sizeof(name), "%s/%s", path, gamename);
				game_library = LoadLibrary (name);
				if (game_library)
				{
					Con_DPrintf ("LoadLibrary (%s)\n",name);
					break;
				}
			}
		}
	}

	GetGameAPI = (void *)GetProcAddress (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();
		return NULL;
	}

	return GetGameAPI (parms);
}
#endif



#include <fcntl.h>
#include <io.h>


#include <signal.h>

#include <shellapi.h>

#ifdef USESERVICE
qboolean asservice;
SERVICE_STATUS_HANDLE   ServerServiceStatusHandle;
SERVICE_STATUS          MyServiceStatus;
void CreateSampleService(qboolean create);
#endif

void PR_Deinit(void);

cvar_t	sys_nostdout = {"sys_nostdout","0"};
cvar_t	sys_colorconsole = {"sys_colorconsole", "1"};

HWND consolewindowhandle;
HWND hiddenwindowhandler;

int Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr;
    static char data[1024];
    int fd;

    va_start(argptr, fmt);
    vsnprintf(data, sizeof(data)-1, fmt, argptr);
    va_end(argptr);
    fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (fd)
	{
	    write(fd, data, strlen(data));
		close(fd);
		return 0;
	}
	return 1; // error
};

/*
================
Sys_FileTime
================
*/
int	Sys_FileTime (char *path)
{
	FILE	*f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}

/*
================
Sys_mkdir
================
*/
int _mkdir(const char *path);;
void Sys_mkdir (char *path)
{
	_mkdir(path);
}

qboolean Sys_remove (char *path)
{
	remove(path);

	return true;
}

int Sys_EnumerateFiles (const char *gpath, const char *match, int (*func)(const char *, int, void *), void *parm)
{
	HANDLE r;
	WIN32_FIND_DATA fd;
	char apath[MAX_OSPATH];
	char file[MAX_OSPATH];
	char *s;
	int go;
	Q_strncpyz(apath, match, sizeof(apath));
//	sprintf(apath, "%s%s", gpath, match);
	for (s = apath+strlen(apath)-1; s>= apath; s--)
	{
		if (*s == '/')
			break;
	}
	s++;
	*s = '\0';



	Q_snprintfz(file, sizeof(file), "%s/%s", gpath, match);
	r = FindFirstFile(file, &fd);
	if (r==(HANDLE)-1)
		return 1;
    go = true;
	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
		{
			if (*fd.cFileName != '.')
			{
				Q_snprintfz(file, sizeof(file), "%s%s/", apath, fd.cFileName);
				go = func(file, fd.nFileSizeLow, parm);
			}
		}
		else
		{
			Q_snprintfz(file, sizeof(file), "%s%s", apath, fd.cFileName);
			go = func(file, fd.nFileSizeLow, parm);
		}
	}
	while(FindNextFile(r, &fd) && go);
	FindClose(r);

	return go;
}

/*
================
Sys_Error
================
*/
#include <process.h>
void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];
	double end;
	STARTUPINFO startupinfo;
	PROCESS_INFORMATION processinfo;

	va_start (argptr,error);
	vsnprintf (text,sizeof(text)-1, error,argptr);
	va_end (argptr);


//    MessageBox(NULL, text, "Error", 0 /* MB_OK */ );
	Sys_Printf ("ERROR: %s\n", text);

	Con_Log(text);

	NET_Shutdown();	//free sockets and stuff.

#ifdef USESERVICE
	if (asservice)
		Sys_Quit();
#endif

	if (COM_CheckParm("-noreset"))
		Sys_Quit();

	Sys_Printf ("A new server will be started in 10 seconds unless you press a key\n");


	//check for a key press, quitting if we get one in 10 secs
	end = Sys_DoubleTime() + 10;
	while(Sys_DoubleTime() < end)
	{
		Sleep(500); // don't burn up CPU with polling
		if (_kbhit())
			Sys_Quit();
	}

	Sys_Printf("\nLoading new instance of FTE...\n\n\n");
	PR_Deinit();	//this takes a bit more mem
	Rank_Flush();
#ifndef MINGW
	fcloseall();	//make sure all files are written.
#endif
	VirtualFree (host_parms.membase, 0, MEM_RELEASE);
//	free(host_parms.membase);	//get rid of the mem. We don't need it now.
//	system("dqwsv.exe");	//spawn a new server to take over. This way, if debugging, then any key will quit, otherwise the server will just spawn a new one.

	memset(&startupinfo, 0, sizeof(startupinfo));
	memset(&processinfo, 0, sizeof(processinfo));

	CreateProcess(NULL,
		GetCommandLine(),
		NULL,
		NULL,
		false,
		0,
		NULL,
		NULL,
		&startupinfo,
		&processinfo);

	CloseHandle(processinfo.hProcess);
	CloseHandle(processinfo.hThread);

	Sys_Quit ();

	exit (1); // this function is NORETURN type, complains without this
}

/*
================
Sys_Milliseconds
================
*/
unsigned int Sys_Milliseconds (void)
{
	static DWORD starttime;
	static qboolean first = true;
	DWORD now;
//	double t;

	now = timeGetTime();

	if (first) {
		first = false;
		starttime = now;
		return 0.0;
	}
	/*
	if (now < starttime) // wrapped?
	{
		double r;
		r = (now) + (LONG_MAX - starttime);
		starttime = now;
		return r;
	}

	if (now - starttime == 0)
		return 0.0;
*/
	return (now - starttime);
}

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	double t;
    struct _timeb tstruct;
	static int	starttime;

	_ftime( &tstruct );

	if (!starttime)
		starttime = tstruct.time;
	t = (tstruct.time-starttime) + tstruct.millitm*0.001;

	return t;
}


/*
================
Sys_ConsoleInput
================
*/

char	coninput_text[256];
int		coninput_len;
char *Sys_ConsoleInput (void)
{
	int		c;

	if (consolewindowhandle)
	{
		MSG msg;
		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0))
				return NULL;
      		TranslateMessage (&msg);
      		DispatchMessage (&msg);
		}
		return NULL;
	}

	// read a line out
	while (_kbhit())
	{
		c = _getch();
		if (c == '\r')
		{
			coninput_text[coninput_len] = 0;
			putch ('\n');
			putch (']');
			coninput_len = 0;
			return coninput_text;
		}
		if (c == 8)
		{
			if (coninput_len)
			{
				putch (c);
				putch (' ');
				putch (c);
				coninput_len--;
				coninput_text[coninput_len] = 0;
			}
			continue;
		}
		if (c == '\t')
		{
			int i;
			char *s = Cmd_CompleteCommand(coninput_text, true, true, 0);
			if(s)
			{
				for (i = 0; i < coninput_len; i++)
					putch('\b');
				for (i = 0; i < coninput_len; i++)
					putch(' ');
				for (i = 0; i < coninput_len; i++)
					putch('\b');

				strcpy(coninput_text, s);
				coninput_len = strlen(coninput_text);
				printf("%s", coninput_text);
			}
			continue;
		}
		putch (c);
		coninput_text[coninput_len] = c;
		coninput_len++;
		coninput_text[coninput_len] = 0;
		if (coninput_len == sizeof(coninput_text))
			coninput_len = 0;
	}

	return NULL;
}

void ApplyColour(unsigned int chr)
{
	static int oldchar = CON_WHITEMASK;
	chr &= CON_FLAGSMASK;

	if (oldchar == chr)
		return;
	oldchar = chr;

	if (hconsoleout)
	{
		unsigned short val = 0;

		// bits 28-31 of the console chars match up to the attributes for
		// the CHAR_INFO struct exactly
		if (chr & CON_NONCLEARBG)
			val = ((chr & (CON_FGMASK|CON_BGMASK)) >> CON_FGSHIFT);
		else
		{
			int fg = (chr & CON_FGMASK) >> CON_FGSHIFT;

			switch (fg)
			{
			case COLOR_BLACK: // reverse ^0 like the Linux version
				val = BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE;
				break;
			case COLOR_WHITE: // reset to defaults?
				val = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE; // use grey
				break;
			case COLOR_GREY:
				val = FOREGROUND_INTENSITY; // color light grey as dark grey
				break;
			default:
				val = fg; // send RGBI value as is
				break;
			}
		}

		if ((chr & CON_HALFALPHA) && (val & ~FOREGROUND_INTENSITY))
			val &= ~FOREGROUND_INTENSITY; // strip intensity to fake alpha

		SetConsoleTextAttribute(hconsoleout, val);
	}
}

void Sys_PrintColouredChar(unsigned int chr)
{
	ApplyColour(chr);

	printf("%c", chr & CON_CHARMASK);
}

/*
================
Sys_Printf
================
*/
#define	MAXPRINTMSG	4096
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;

	if (sys_nostdout.value)
		return;

	if (1)
	{
		char		msg[MAXPRINTMSG];
		unsigned char *t;

		va_start (argptr,fmt);
		vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
		va_end (argptr);

		{
			int i;

			for (i = 0; i < coninput_len; i++)
				putch('\b');
			putch('\b');
			for (i = 0; i < coninput_len; i++)
				putch(' ');
			putch(' ');
			for (i = 0; i < coninput_len; i++)
				putch('\b');
			putch('\b');
		}


		for (t = (unsigned char*)msg; *t; t++)
		{
			if (*t >= 146 && *t < 156)
				*t = *t - 146 + '0';
			if (*t >= 0x12 && *t <= 0x1b)
				*t = *t - 0x12 + '0';
			if (*t == 143)
				*t = '.';
			if (*t == 157 || *t == 158 || *t == 159)
				*t = '-';
			if (*t >= 128)
				*t -= 128;
			if (*t == 16)
				*t = '[';
			if (*t == 17)
				*t = ']';
			if (*t == 0x1c)
				*t = 249;
		}

		if (sys_colorconsole.value && hconsoleout)
		{
			int ext = CON_WHITEMASK;
			int extstack[4];
			int extstackdepth = 0;
			unsigned char *str = (unsigned char*)msg;


			while(*str)
			{
				if (*str == '^')
				{
					str++;
					if (*str >= '0' && *str <= '9')
					{
						ext = q3codemasks[*str++-'0'] | (ext&~CON_Q3MASK);	//change colour only.
						continue;
					}
					else if (*str == '&') // extended code
					{
						if (isextendedcode(str[1]) && isextendedcode(str[2]))
						{
							str++; // foreground char
							if (*str == '-') // default for FG
								ext = (COLOR_WHITE << CON_FGSHIFT) | (ext&~CON_FGMASK);
							else if (*str >= 'A')
								ext = ((*str - ('A' - 10)) << CON_FGSHIFT) | (ext&~CON_FGMASK);
							else
								ext = ((*str - '0') << CON_FGSHIFT) | (ext&~CON_FGMASK);
							str++; // background char
							if (*str == '-') // default (clear) for BG
								ext &= ~CON_BGMASK & ~CON_NONCLEARBG;
							else if (*str >= 'A')
								ext = ((*str - ('A' - 10)) << CON_BGSHIFT) | (ext&~CON_BGMASK) | CON_NONCLEARBG;
							else
								ext = ((*str - '0') << CON_BGSHIFT) | (ext&~CON_BGMASK) | CON_NONCLEARBG;
							str++;
							continue;
						}
						Sys_PrintColouredChar('^' | ext);
						// else invalid code
					}
					else if (*str == 'a')
					{
						str++;
						ext ^= CON_2NDCHARSETTEXT;
						continue;
					}
					else if (*str == 'b')
					{
						str++;
						ext ^= CON_BLINKTEXT;
						continue;
					}
					else if (*str == 'h')
					{
						str++;
						ext ^= CON_HALFALPHA;
						continue;
					}
					else if (*str == 's')	//store on stack (it's great for names)
					{
						str++;
						if (extstackdepth < sizeof(extstack)/sizeof(extstack[0]))
						{
							extstack[extstackdepth] = ext;
							extstackdepth++;
						}
						continue;
					}
					else if (*str == 'r')	//restore from stack (it's great for names)
					{
						str++;
						if (extstackdepth)
						{
							extstackdepth--;
							ext = extstack[extstackdepth];
						}
						continue;
					}
					else if (*str == '^')
					{
						Sys_PrintColouredChar('^' | ext);
						str++;
					}
					else
					{
						Sys_PrintColouredChar('^' | ext);
						Sys_PrintColouredChar ((*str++) | ext);
					}
					continue;
				}
				Sys_PrintColouredChar ((*str++) | ext);
			}

			ApplyColour(CON_WHITEMASK);
		}
		else
			printf("%s", msg);


		if (coninput_len)
			printf("]%s", coninput_text);
		else
			putch(']');
	}
	else
	{
		va_start (argptr,fmt);
		vprintf (fmt,argptr);
		va_end (argptr);
	}
}

/*
================
Sys_Quit
================
*/

void Sys_Quit (void)
{
#ifdef USESERVICE
	if (asservice)
	{
		MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
		MyServiceStatus.dwCheckPoint         = 0;
		MyServiceStatus.dwWaitHint           = 0;
		MyServiceStatus.dwWin32ExitCode      = 0;
		MyServiceStatus.dwServiceSpecificExitCode = 0;

		SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);
	}
#endif
	exit (0);
}

int restorecode;

LRESULT (CALLBACK Sys_WindowHandler)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_USER)
	{
		if (lParam & 1)
		{
		}
		else if ((lParam & 2 && restorecode == 0) ||
			(lParam & 4 && restorecode == 1) ||
			(lParam & 4 && restorecode == 2) )
		{
// 			MessageBox(NULL, "Hello", "", 0);
			restorecode++;
		}
		else if (lParam & 2 && restorecode == 3)
		{
			DestroyWindow(hWnd);
			ShowWindow(consolewindowhandle, SW_SHOWNORMAL);
			consolewindowhandle = NULL;

			Cbuf_AddText("status\n", RESTRICT_LOCAL);
		}
		else if (lParam & 6)
		{
			restorecode = (lParam & 2)>0;
		}

		return 0;
	}
	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}
void Sys_HideConsole(void)
{
	HMODULE kernel32dll;
	HWND (WINAPI *GetConsoleWindow)(void);

	if (consolewindowhandle)
		return;	//err... already hidden... ?

	restorecode = 0;

	GetConsoleWindow = NULL;
	kernel32dll = LoadLibrary("kernel32.dll");
	consolewindowhandle = NULL;
	if (kernel32dll)
	{
		GetConsoleWindow = (void*)GetProcAddress(kernel32dll, "GetConsoleWindow");
		if (GetConsoleWindow)
			consolewindowhandle = GetConsoleWindow();

		FreeModule(kernel32dll);	//works because the underlying code uses kernel32, so this decreases the reference count rather than closing it.
	}

	if (!consolewindowhandle)
	{
		char old[512];
#define STRINGH	"Trying to hide"	//msvc sucks
		GetConsoleTitle(old, sizeof(old));
		SetConsoleTitle(STRINGH);
		consolewindowhandle = FindWindow(NULL, STRINGH);
		SetConsoleTitle(old);
#undef STRINGH
	}

	if (consolewindowhandle)
	{
		WNDCLASS wc;
		NOTIFYICONDATA d;

			/* Register the frame class */
		memset(&wc, 0, sizeof(wc));
		wc.style         = 0;
		wc.lpfnWndProc   = Sys_WindowHandler;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = GetModuleHandle(NULL);
		wc.hIcon         = 0;
		wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = 0;
		wc.lpszClassName = "DeadQuake";

		RegisterClass (&wc);

		hiddenwindowhandler = CreateWindow(wc.lpszClassName, "DeadQuake", 0, 0, 0, 16, 16, NULL, NULL, GetModuleHandle(NULL), NULL);
		if (!hiddenwindowhandler)
		{
			Con_Printf("Failed to create window\n");
			return;
		}
		ShowWindow(consolewindowhandle, SW_HIDE);

		d.cbSize = sizeof(NOTIFYICONDATA);
		d.hWnd = hiddenwindowhandler;
		d.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
		d.hIcon = NULL;
		d.uCallbackMessage = WM_USER;
		d.uID = 0;
		strcpy(d.szTip, "");
		Shell_NotifyIcon(NIM_ADD, &d);
	}
	else
		Con_Printf("Your OS doesn't seem to properly support the way this was implemented\n");
}

void Sys_ServerActivity(void)
{
	HMODULE kernel32dll;
	HWND (WINAPI *GetConsoleWindow)(void);
	HWND wnd;

	restorecode = 0;

	GetConsoleWindow = NULL;
	kernel32dll = LoadLibrary("kernel32.dll");
	wnd = NULL;
	if (kernel32dll)
	{
		GetConsoleWindow = (void*)GetProcAddress(kernel32dll, "GetConsoleWindow");
		if (GetConsoleWindow)
			wnd = GetConsoleWindow();

		FreeModule(kernel32dll);	//works because the underlying code uses kernel32, so this decreases the reference count rather than closing it.
	}

	if (!wnd)
	{
		char old[512];
#define STRINGF	"About To Flash"	//msvc sucks
		GetConsoleTitle(old, sizeof(old));
		SetConsoleTitle(STRINGF);
		wnd = FindWindow(NULL, STRINGF);
		SetConsoleTitle(old);
#undef STRINGF
	}

	if (wnd && GetActiveWindow() != wnd)
		FlashWindow(wnd, true);
}

/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init (void)
{
	Cvar_Register (&sys_nostdout, "System controls");
	Cvar_Register (&sys_colorconsole, "System controls");

	Cmd_AddCommand("hide", Sys_HideConsole);

	hconsoleout = GetStdHandle(STD_OUTPUT_HANDLE);
}

void Sys_Shutdown (void)
{
}

/*
==================
main

==================
*/
char	*newargv[256];

void Signal_Error_Handler (int sig)
{
	Sys_Error("Illegal error occured");
}


void StartQuakeServer(void)
{
	quakeparms_t	parms;
	//static	char	cwd[1024]; //unused variable
	int				t;

	TL_InitLanguages();

	parms.argc = com_argc;
	parms.argv = com_argv;

	parms.memsize = 32*1024*1024;

	if ((t = COM_CheckParm ("-heapsize")) != 0 &&
		t + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[t + 1]) * 1024;

	if ((t = COM_CheckParm ("-mem")) != 0 &&
		t + 1 < com_argc)
		parms.memsize = Q_atoi (com_argv[t + 1]) * 1024 * 1024;

	parms.membase = VirtualAlloc(NULL, parms.memsize, MEM_RESERVE, PAGE_NOACCESS);
//	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error("Insufficient memory.\n");

	parms.basedir = ".";

	SV_Init (&parms);

// run one frame immediately for first heartbeat
	SV_Frame ();
}


#ifdef USESERVICE
int servicecontrol;
#endif
void ServerMainLoop(void)
{
	double			newtime, time, oldtime;
	int delay = 1;
//
// main loop
//
	oldtime = Sys_DoubleTime () - 0.1;
	while (1)
	{
		NET_Sleep(delay, false);

	// find time passed since last cycle
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		oldtime = newtime;
		delay = SV_Frame()*1000;


#ifdef USESERVICE
		switch(servicecontrol)
		{
		case SERVICE_CONTROL_PAUSE:
			// Initialization complete - report running status.
			MyServiceStatus.dwCurrentState       = SERVICE_PAUSED;
			MyServiceStatus.dwCheckPoint         = 0;
			MyServiceStatus.dwWaitHint           = 0;

			SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);
			sv.paused |= 2;
			break;
		case SERVICE_CONTROL_CONTINUE:
			// Initialization complete - report running status.
			MyServiceStatus.dwCurrentState       = SERVICE_RUNNING;
			MyServiceStatus.dwCheckPoint         = 0;
			MyServiceStatus.dwWaitHint           = 0;

			SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);

			sv.paused &= ~2;
			break;
		case SERVICE_CONTROL_STOP:	//leave the loop
			return;
		default:
			break;
		}
#endif
	}
}
#ifdef USESERVICE

VOID WINAPI MyServiceCtrlHandler(DWORD    dwControl)
{
	servicecontrol = dwControl;
}

void WINAPI StartQuakeServerService	(DWORD argc, LPTSTR *argv)
{
	HKEY hk;
	char path[MAX_OSPATH];
	DWORD pathlen;
	DWORD type;

	asservice = true;

	MyServiceStatus.dwServiceType        = SERVICE_WIN32|SERVICE_INTERACTIVE_PROCESS;
	MyServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
	MyServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_PAUSE_CONTINUE;
	MyServiceStatus.dwWin32ExitCode      = 0;
	MyServiceStatus.dwServiceSpecificExitCode = 0;
	MyServiceStatus.dwCheckPoint         = 0;
	MyServiceStatus.dwWaitHint           = 0;

	ServerServiceStatusHandle = RegisterServiceCtrlHandler(
		SERVICENAME,
		MyServiceCtrlHandler);

	if (ServerServiceStatusHandle == (SERVICE_STATUS_HANDLE)0)
	{
		printf(" [MY_SERVICE] RegisterServiceCtrlHandler failed %d\n", GetLastError());
		return;
	}


	RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\"DISTRIBUTIONLONG"\\"FULLENGINENAME, &hk);
	RegQueryValueEx(hk, "servicepath", 0, &type, NULL, &pathlen);
	if (type == REG_SZ && pathlen < sizeof(path))
		RegQueryValueEx(hk, "servicepath", 0, NULL, path, &pathlen);
	RegCloseKey(hk);

	SetCurrentDirectory(path);


	COM_InitArgv (argc, argv);
	StartQuakeServer();


	// Handle error condition
	if (!sv.state)
	{
		MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
		MyServiceStatus.dwCheckPoint         = 0;
		MyServiceStatus.dwWaitHint           = 0;
		MyServiceStatus.dwWin32ExitCode      = 0;
		MyServiceStatus.dwServiceSpecificExitCode = 0;

		SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);
		return;
	}

	// Initialization complete - report running status.
	MyServiceStatus.dwCurrentState       = SERVICE_RUNNING;
	MyServiceStatus.dwCheckPoint         = 0;
	MyServiceStatus.dwWaitHint           = 0;

	if (!SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus))
	{
		printf(" [MY_SERVICE] SetServiceStatus error %ld\n",GetLastError());
	}

	ServerMainLoop();

	MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
	MyServiceStatus.dwCheckPoint         = 0;
	MyServiceStatus.dwWaitHint           = 0;
	MyServiceStatus.dwWin32ExitCode      = 0;
	MyServiceStatus.dwServiceSpecificExitCode = 0;

	SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);

	return;
}

SERVICE_TABLE_ENTRY   DispatchTable[] =
{
{ SERVICENAME, StartQuakeServerService      },
{ NULL,              NULL          }
};
#endif

qboolean NET_Sleep(int msec, qboolean stdinissocket);
int main (int argc, char **argv)
{
#ifdef USESERVICE
	if (StartServiceCtrlDispatcher( DispatchTable))
	{
		return true;
	}
#endif

#ifdef CATCHCRASH
	__try
#endif
	{
		COM_InitArgv (argc, (const char **)argv);
	#ifdef USESERVICE
		if (COM_CheckParm("-register"))
		{
			CreateSampleService(1);
			return true;
		}
		if (COM_CheckParm("-unregister"))
		{
			CreateSampleService(0);
			return true;
		}
	#endif

	#ifndef _DEBUG
		if (COM_CheckParm("-noreset"))
		{
			signal (SIGFPE,	Signal_Error_Handler);
			signal (SIGILL,	Signal_Error_Handler);
			signal (SIGSEGV,	Signal_Error_Handler);
		}
	#endif
		StartQuakeServer();

		ServerMainLoop();
	}
#ifdef CATCHCRASH
	__except (CrashExceptionHandler(GetExceptionCode(), GetExceptionInformation()))
	{
		return 1;
	}
#endif

	return true;
}

#ifdef USESERVICE
void CreateSampleService(qboolean create)
{
	BOOL deleted;
	char path[MAX_OSPATH];
	char exe[MAX_OSPATH];
	SC_HANDLE schService;

	SC_HANDLE schSCManager;

// Open a handle to the SC Manager database.
	schSCManager = OpenSCManager(
		NULL,                    // local machine
		NULL,                    // ServicesActive database
		SC_MANAGER_ALL_ACCESS);  // full access rights

	if (NULL == schSCManager)
	{
		Con_Printf("Failed to open SCManager (%d)\n", GetLastError());
		return;
	}

	if (!GetModuleFileName(NULL, exe+1, sizeof(exe)-2))
	{
		Con_Printf("Path too long\n");
		return;
	}
	GetCurrentDirectory(sizeof(path), path);
	exe[0] = '\"';
	exe[strlen(path)+1] = '\0';
	exe[strlen(path)] = '\"';

	if (!create)
	{
		schService = OpenServiceA(schSCManager, SERVICENAME, SERVICE_ALL_ACCESS);
		if (schService)
		{
			deleted = DeleteService(schService);
		}
	}
	else
	{
		HKEY hk;
		RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\"DISTRIBUTIONLONG"\\"FULLENGINENAME, &hk);
		if (!hk)RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\"DISTRIBUTIONLONG"\\"FULLENGINENAME, &hk);
		RegSetValueEx(hk, "servicepath", 0, REG_SZ, path, strlen(path));
		RegCloseKey(hk);

		schService = CreateService(
			schSCManager,				// SCManager database
			SERVICENAME,				// name of service
			FULLENGINENAME" Server",	// service name to display
			SERVICE_ALL_ACCESS,			// desired access
			SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,	// service type
			SERVICE_AUTO_START,			// start type
			SERVICE_ERROR_NORMAL,		// error control type
			exe,						// service's binary
			NULL,						// no load ordering group
			NULL,						// no tag identifier
			NULL,						// no dependencies
			NULL,						// LocalSystem account
			NULL);						// no password
	}

    if (schService == NULL)
    {
        Con_Printf("CreateService failed.\n");
        return;
    }
    else
    {
        CloseServiceHandle(schService);
        return;
    }
}
#endif

#ifdef MULTITHREAD
/* Thread creation calls */
typedef struct threadwrap_s
{
	void (*func)(void *);
	void *args;
} threadwrap_t;

// the thread call is wrapped so we don't need WINAPI everywhere
DWORD WINAPI threadwrapper(void *args)
{
	threadwrap_t tw;
	tw.func = ((threadwrap_t *)args)->func;
	tw.args = ((threadwrap_t *)args)->args;

	free(args);
	tw.func(tw.args);

#ifndef WIN32CRTDLL
	_endthreadex(0);
#endif
	return 0;
}

void *Sys_CreateThread(int (*func)(void *), void *args, int stacksize)
{
	threadwrap_t *tw = (threadwrap_t *)malloc(sizeof(threadwrap_t));
	HANDLE handle;

	if (!tw)
		return NULL;

	stacksize += 128; // wrapper overhead, also prevent default stack size
	tw->func = func;
	tw->args = args;
#ifdef WIN32CRTDLL
	handle = (HANDLE)CreateThread(NULL, stacksize, &threadwrapper, (void *)tw, 0, NULL);
#else
	handle = (HANDLE)_beginthreadex(NULL, stacksize, &threadwrapper, (void *)tw, 0, NULL);
#endif
	if (!handle)
	{
		free(tw);
		return NULL;
	}

	return (void *)handle;
}

void Sys_WaitOnThread(void *thread)
{
	WaitForSingleObject((HANDLE)thread, INFINITE);
	CloseHandle((HANDLE)thread);
}

/* Mutex calls */
void *Sys_CreateMutex(void)
{
	return (void *)CreateMutex(NULL, 0, NULL);
}

qboolean Sys_TryLockMutex(void *mutex)
{
	return WaitForSingleObject(mutex, 0) == WAIT_OBJECT_0;
}

qboolean Sys_LockMutex(void *mutex)
{
	return WaitForSingleObject(mutex, INFINITE) == WAIT_OBJECT_0;
}

qboolean Sys_UnlockMutex(void *mutex)
{
	return !!ReleaseMutex(mutex);
}

void Sys_DestroyMutex(void *mutex)
{
	CloseHandle(mutex);
}

/* Conditional wait calls */
/*
TODO: Windows Vista has condition variables as documented here:
http://msdn.microsoft.com/en-us/library/ms682052(VS.85).aspx
Note this uses Slim Reader/Writer locks (Vista+ exclusive)
or critical sections.

The condition variable implementation is based on the libSDL implementation.
This code could probably be made more efficient with the use of events or
different mechanisms but for now the main concern is a correct and
complete solution.
*/
typedef struct condvar_s
{
    int waiting;
    int signals;
    CRITICAL_SECTION countlock;
	CRITICAL_SECTION mainlock;
    HANDLE wait_sem;
    HANDLE wait_done;
} condvar_t;

void *Sys_CreateConditional(void)
{
	condvar_t *cv;

	cv = (condvar_t *)malloc(sizeof(condvar_t));
	if (!cv)
		return NULL;

	cv->waiting = 0;
	cv->signals = 0;
	InitializeCriticalSection (&cv->mainlock);
	InitializeCriticalSection (&cv->countlock);
	cv->wait_sem = CreateSemaphore (NULL, 0, 0x7fffffff, NULL);
	cv->wait_done = CreateSemaphore (NULL, 0, 0x7fffffff, NULL);

	if (cv->wait_sem && cv->wait_done)
		return (void *)cv;

	// something failed so deallocate everything
	if (cv->wait_done)
		CloseHandle(cv->wait_done);
	if (cv->wait_sem)
		CloseHandle(cv->wait_sem);
	DeleteCriticalSection(&cv->countlock);
	DeleteCriticalSection(&cv->mainlock);
	free(cv);

	return NULL;
}

qboolean Sys_LockConditional(void *condv)
{
	EnterCriticalSection(&((condvar_t *)condv)->mainlock);
	return true;
}

qboolean Sys_UnlockConditional(void *condv)
{
	LeaveCriticalSection(&((condvar_t *)condv)->mainlock);
	return true;
}

qboolean Sys_ConditionWait(void *condv)
{
	condvar_t *cv = (condvar_t *)condv;
	qboolean success;

	// increase count for non-signaled waiting threads
	EnterCriticalSection(&cv->countlock);
	cv->waiting++;
	LeaveCriticalSection(&cv->countlock);

	LeaveCriticalSection(&cv->mainlock); // unlock as per condition variable definition

	// wait on a signal
	success = (WaitForSingleObject(cv->wait_sem, INFINITE) != WAIT_FAILED);

	// update waiting count and alert signaling thread that we're done to avoid the deadlock condition
	EnterCriticalSection(&cv->countlock);
	if (cv->signals > 0)
	{
		ReleaseSemaphore(cv->wait_done, cv->signals, NULL);
		cv->signals = 0;
	}
	cv->waiting--;
	LeaveCriticalSection(&cv->countlock);

	EnterCriticalSection(&cv->mainlock); // lock as per condition variable definition

	return success;
}

qboolean Sys_ConditionSignal(void *condv)
{
	condvar_t *cv = (condvar_t *)condv;

	// if there are non-signaled waiting threads, we signal one and wait on the response
	EnterCriticalSection(&cv->countlock);
	if (cv->waiting > cv->signals)
	{
		cv->signals++;
		ReleaseSemaphore(cv->wait_sem, 1, NULL);
		LeaveCriticalSection(&cv->countlock);
		WaitForSingleObject(cv->wait_done, INFINITE);
	}
	else
		LeaveCriticalSection(&cv->countlock);

    return true;
}

qboolean Sys_ConditionBroadcast(void *condv)
{
	condvar_t *cv = (condvar_t *)condv;

	// if there are non-signaled waiting threads, we signal all of them and wait on all the responses back
	EnterCriticalSection(&cv->countlock);
	if (cv->waiting > cv->signals)
	{
		int i, num_waiting;

		num_waiting = (cv->waiting - cv->signals);
		cv->signals = cv->waiting;

		ReleaseSemaphore(cv->wait_sem, num_waiting, NULL);
		LeaveCriticalSection(&cv->countlock);
		// there's no call to wait for the same object multiple times so we need to loop through
		// and burn up the semaphore count
		for (i = 0; i < num_waiting; i++)
			WaitForSingleObject(cv->wait_done, INFINITE);
	}
	else
		LeaveCriticalSection(&cv->countlock);

	return true;
}

void Sys_DestroyConditional(void *condv)
{
	condvar_t *cv = (condvar_t *)condv;

	CloseHandle(cv->wait_done);
	CloseHandle(cv->wait_sem);
	DeleteCriticalSection(&cv->countlock);
	DeleteCriticalSection(&cv->mainlock);
	free(cv);
}
#endif

#endif
