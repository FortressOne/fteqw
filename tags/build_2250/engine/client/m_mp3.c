//mp3 menu and track selector.
//was origonally an mp3 track selector, now handles lots of media specific stuff - like q3 films!
//should rename to m_media.c
#include "quakedef.h"
#ifdef RGLQUAKE
#include "glquake.h"//fixme
#endif



#if !defined(__CYGWIN__) && !defined(NOMEDIA)


#include "winquake.h"
#ifdef _WIN32
#define WINAMP
#endif

#ifdef WINAMP

#include "winamp.h"
HWND hwnd_winamp;

#endif

#ifdef SWQUAKE
#include "d_local.h"
#endif
qboolean Media_EvaluateNextTrack(void);

typedef struct mediatrack_s{
	char filename[128];
	char nicename[128];
	int length;
	struct mediatrack_s *next;
} mediatrack_t;

static mediatrack_t currenttrack;
int lasttrackplayed;

int media_playing=true;//try to continue from the standard playlist
cvar_t media_shuffle = {"media_shuffle", "1"};
cvar_t media_repeat = {"media_repeat", "1"};
#ifdef WINAMP
cvar_t media_hijackwinamp = {"media_hijackwinamp", "0"};
#endif

int selectedoption=-1;
int numtracks;
int nexttrack=-1;
mediatrack_t *tracks;

char media_iofilename[MAX_OSPATH]="";

int loadedtracknames;

#ifdef WINAMP
qboolean WinAmp_GetHandle (void)
{
	if ((hwnd_winamp = FindWindow("Winamp", NULL)))
		return true;
	if ((hwnd_winamp = FindWindow("Winamp v1.x", NULL)))
		return true;

	*currenttrack.nicename = '\0';

	return false;
}

qboolean WinAmp_StartTune(char *name)
{
	int trys;
	int pos;
	COPYDATASTRUCT cds;
	if (!WinAmp_GetHandle())
		return false;

	//FIXME: extract from fs if it's in a pack.
	//FIXME: always give absolute path
	cds.dwData = IPC_PLAYFILE;
	cds.lpData = (void *) name;
	cds.cbData = strlen((char *) cds.lpData)+1; // include space for null char
	SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_DELETE);
	SendMessage(hwnd_winamp,WM_COPYDATA,(WPARAM)NULL,(LPARAM)&cds);
	SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)0,IPC_STARTPLAY );
	
	for (trys = 1000; trys; trys--)
	{
		pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		if (pos>100 && SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME)>=0)	//tune has started
			break;

		Sleep(10);	//give it a chance.
		if (!WinAmp_GetHandle())
			break;
	}

	return true;
}

void WinAmp_Think(void)
{
	int pos;
	int len;

	if (!WinAmp_GetHandle())
		return;	

	pos = bgmvolume.value*255;
	if (pos > 255) pos = 255;
	if (pos < 0) pos = 0;
	PostMessage(hwnd_winamp, WM_WA_IPC,pos,IPC_SETVOLUME);

//optimise this to reduce calls?
	pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
	len = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETOUTPUTTIME)*1000;

	if ((pos > len || pos <= 100) && len != -1)
	if (Media_EvaluateNextTrack())
		WinAmp_StartTune(currenttrack.filename);
}
#endif
void Media_Seek (float time)
{	
	soundcardinfo_t *sc;
#ifdef WINAMP
	if (media_hijackwinamp.value)
	{
		int pos;
		if (WinAmp_GetHandle())
		{
			pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
			pos += time*1000;
			PostMessage(hwnd_winamp,WM_WA_IPC,pos,IPC_JUMPTOTIME);

			WinAmp_Think();
		}
	}
#endif
	for (sc = sndcardinfo; sc; sc=sc->next)
	{
		sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].pos += sc->sn.speed*time;
		sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].end += sc->sn.speed*time;

		if (sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].pos < 0)
		{
			sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].end -= sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].pos;
			sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].pos=0;
		}
		//if we seek over the end, ignore it. The sound playing code will spot that.
	}
}

void Media_FForward_f(void)
{
	float time = atoi(Cmd_Argv(1));
	if (!time)
		time = 15;
	Media_Seek(time);
}
void Media_Rewind_f (void)
{
	float time = atoi(Cmd_Argv(1));
	if (!time)
		time = 15;
	Media_Seek(-time);
}

qboolean Media_EvaluateNextTrack(void)
{
	mediatrack_t *track;
	int trnum;
	if (!tracks)
		return false;
	if (nexttrack>=0)
	{
		trnum = nexttrack;
		for (track = tracks; track; track=track->next)
		{
			if (!trnum)
			{
				memcpy(&currenttrack, track->filename, sizeof(mediatrack_t));
				lasttrackplayed = nexttrack;
				break;
			}
			trnum--;
		}
		nexttrack = -1;
	}
	else
	{
		if (media_shuffle.value)
			nexttrack=((float)(rand()&0x7fff)/0x7fff)*numtracks;
		else
		{
			nexttrack = lasttrackplayed+1;
			if (nexttrack >= numtracks)
			{
				if (media_repeat.value)
					nexttrack = 0;
				else
				{
					*currenttrack.filename='\0';
					*currenttrack.nicename='\0';
					nexttrack = -1;
					media_playing = false;
					return false;
				}
			}
		}
		trnum = nexttrack;
		for (track = tracks; track; track=track->next)
		{
			if (!trnum)
			{
				memcpy(&currenttrack, track->filename, sizeof(mediatrack_t));
				lasttrackplayed = nexttrack;
				break;
			}
			trnum--;
		}
		nexttrack = -1;
	}

	return true;
}

//flushes music channel on all soundcards, and the tracks that arn't decoded yet.
void Media_Clear (void)
{
	sfx_t *s;
	soundcardinfo_t *sc;
	for (sc = sndcardinfo; sc; sc=sc->next)
	{
		sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].end = 0;
		s = sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].sfx;
		sc->channel[MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS].sfx = NULL;

		if (s)
		if (s->decoder)
		if (!S_IsPlayingSomewhere(s))	//if we aint playing it elsewhere, free it compleatly.
		{
			s->decoder->abort(s);
		}
	}
}

//actually, this func just flushes and states that it should be playing. the ambientsound func actually changes the track.
void Media_Next_f (void)
{
	Media_Clear();
	media_playing=true;

#ifdef WINAMP
	if (media_hijackwinamp.value)
	{
		if (WinAmp_GetHandle())
		if (Media_EvaluateNextTrack())
			WinAmp_StartTune(currenttrack.filename);
	}
#endif
}








void M_Menu_Media_f (void) {
	key_dest = key_menu;
	m_state = m_media;
	m_entersound = true;
}

void Media_LoadTrackNames (char *listname);

#define MEDIA_MIN	-8
#define MEDIA_VOLUME -8
#define MEDIA_REWIND -7
#define MEDIA_FASTFORWARD -6
#define MEDIA_CLEARLIST -5
#define MEDIA_ADDTRACK -4
#define MEDIA_ADDLIST -3
#define MEDIA_SHUFFLE -2
#define MEDIA_REPEAT -1

void M_Media_Draw (void)
{
	mpic_t	*p;
	mediatrack_t *track;
	int y;
	int op, i;

#define MP_Hightlight(x,y,text,hl) (hl?M_PrintWhite(x, y, text):M_Print(x, y, text))

	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	if (!bgmvolume.value)
		M_Print (12, 32, "Not playing - no volume");
	else if (!*currenttrack.nicename)
	{
		if (!tracks)
			M_Print (12, 32, "Not playing - no track to play");
		else
		{
#ifdef WINAMP
			if (!WinAmp_GetHandle())
				M_Print (12, 32, "Please start WinAmp 2");
			else
#endif
				M_Print (12, 32, "Not playing - switched off");
		}
	}
	else
	{
		M_Print (12, 32, "Currently playing:");
		M_Print (12, 40, currenttrack.nicename);
	}

	op = selectedoption - (vid.height-52)/16;
	if (op + (vid.height-52)/8>numtracks)
		op = numtracks - (vid.height-52)/8;
	if (op < MEDIA_MIN)
		op = MEDIA_MIN;
	y=52;
	while(op < 0)
	{
		switch(op)
		{
		case MEDIA_VOLUME:
			MP_Hightlight (12, y, "Volume", op == selectedoption);
			y+=8;
			break;
		case MEDIA_CLEARLIST:
			MP_Hightlight (12, y, "Clear all", op == selectedoption);
			y+=8;
			break;
		case MEDIA_FASTFORWARD:
			MP_Hightlight (12, y, ">> Fast Forward", op == selectedoption);
			y+=8;
			break;
		case MEDIA_REWIND:
			MP_Hightlight (12, y, "<< Rewind", op == selectedoption);
			y+=8;
			break;
		case MEDIA_ADDTRACK:
			MP_Hightlight (12, y, "Add Track", op == selectedoption);
			if (op == selectedoption)
				M_PrintWhite (12+9*8, y, media_iofilename);
			y+=8;
			break;
		case MEDIA_ADDLIST:
			MP_Hightlight (12, y, "Add List", op == selectedoption);
			if (op == selectedoption)
				M_PrintWhite (12+9*8, y, media_iofilename);
			y+=8;
			break;
		case MEDIA_SHUFFLE:
			if (media_shuffle.value)
				MP_Hightlight (12, y, "Shuffle on", op == selectedoption);
			else
				MP_Hightlight (12, y, "Shuffle off", op == selectedoption);
			y+=8;
			break;
		case MEDIA_REPEAT:
			if (media_shuffle.value)
			{
				if (media_repeat.value)
					MP_Hightlight (12, y, "Repeat on", op == selectedoption);
				else
					MP_Hightlight (12, y, "Repeat off", op == selectedoption);				
			}
			else
			{
				if (media_repeat.value)
					MP_Hightlight (12, y, "(Repeat on)", op == selectedoption);
				else
					MP_Hightlight (12, y, "(Repeat off)", op == selectedoption);				
			}
			y+=8;
			break;
		}
		op++;
	}

	for (track = tracks, i=0; track && i<op; track=track->next, i++);
	for (; track; track=track->next, y+=8, op++)
	{
		if (op == selectedoption)
			M_PrintWhite (12, y, track->nicename);
		else
			M_Print (12, y, track->nicename);
	}
}

char compleatenamepath[MAX_OSPATH];
char compleatenamename[MAX_OSPATH];
qboolean compleatenamemultiple;
int Com_CompleatenameCallback(char *name, int size, void *data)
{
	if (*compleatenamename)
		compleatenamemultiple = true;
	strcpy(compleatenamename, name);	

	return true;
}
void Com_CompleateOSFileName(char *name)
{	
	char *ending;
	compleatenamemultiple = false;

	strcpy(compleatenamepath, name);
	ending = COM_SkipPath(compleatenamepath);
	if (compleatenamepath!=ending)
		ending[-1] = '\0';	//strip a slash
	*compleatenamename='\0';

	Sys_EnumerateFiles(NULL, va("%s*", name), Com_CompleatenameCallback, NULL);
	Sys_EnumerateFiles(NULL, va("%s*.*", name), Com_CompleatenameCallback, NULL);

	if (*compleatenamename)
		strcpy(name, compleatenamename);
}

void M_Media_Key (int key)
{
	int dir;	
	if (key == K_ESCAPE)
		M_Menu_Main_f();
	else if (key == K_RIGHTARROW || key == K_LEFTARROW)
	{
		if (key == K_RIGHTARROW)
			dir = 1;
		else dir = -1;
		switch(selectedoption)
		{
		case MEDIA_VOLUME:
			bgmvolume.value += dir * 0.1;
			if (bgmvolume.value < 0)
				bgmvolume.value = 0;
			if (bgmvolume.value > 1)
				bgmvolume.value = 1;
			Cvar_SetValue (&bgmvolume, bgmvolume.value);
			break;
		default:
			if (selectedoption >= 0)
				Media_Next_f();
			break;
		}
	}
	else if (key == K_DOWNARROW)
	{
		selectedoption++;
		if (selectedoption>=numtracks)
			selectedoption = numtracks-1;
	}
	else if (key == K_PGDN)
	{
		selectedoption+=10;
		if (selectedoption>=numtracks)
			selectedoption = numtracks-1;
	}
	else if (key == K_UPARROW)
	{
		selectedoption--;
		if (selectedoption < MEDIA_MIN)
			selectedoption = MEDIA_MIN;
	}
	else if (key == K_PGUP)
	{
		selectedoption-=10;
		if (selectedoption < MEDIA_MIN)
			selectedoption = MEDIA_MIN;
	}
	else if (key == K_DEL)
	{
		if (selectedoption>=0)
		{
			mediatrack_t *prevtrack=NULL, *tr;
			int num=0;
			tr=tracks;
			while(tr)
			{
				if (num == selectedoption)
				{
					if (prevtrack)
						prevtrack->next = tr->next;
					else
						tracks = tr->next;
					Z_Free(tr);
					numtracks--;
					break;
				}

				prevtrack = tr;
				tr=tr->next;
				num++;
			}
		}
	}
	else if (key == K_ENTER)
	{
		switch(selectedoption)
		{
		case MEDIA_FASTFORWARD:
			Media_Seek(15);
			break;
		case MEDIA_REWIND:
			Media_Seek(-15);
			break;
		case MEDIA_CLEARLIST:
			{
				mediatrack_t *prevtrack;
				while(tracks)
				{
					prevtrack = tracks;
					tracks=tracks->next;
					Z_Free(prevtrack);
					numtracks--;
				}
				if (numtracks!=0)
				{
					numtracks=0;
					Con_SafePrintf("numtracks should be 0\n");
				}
			}
			break;
		case MEDIA_ADDTRACK:
			if (*media_iofilename)
			{
				mediatrack_t *newtrack;
				newtrack = Z_Malloc(sizeof(mediatrack_t));
				Q_strncpyz(newtrack->filename, media_iofilename, sizeof(newtrack->filename));
				Q_strncpyz(newtrack->nicename, COM_SkipPath(media_iofilename), sizeof(newtrack->filename));
				newtrack->length = 0;
				newtrack->next = tracks;
				tracks = newtrack;
				numtracks++;
			}
			break;
		case MEDIA_ADDLIST:
			if (*media_iofilename)
				Media_LoadTrackNames(media_iofilename);
			break;						
		case MEDIA_SHUFFLE:
			Cvar_Set(&media_shuffle, media_shuffle.value?"0":"1");
			break;
		case MEDIA_REPEAT:
			Cvar_Set(&media_repeat, media_repeat.value?"0":"1");
			break;
		default:
			if (selectedoption>=0)
			{
				media_playing = true;
				nexttrack = selectedoption;
				Media_Next_f();				
			}
			break;
		}
	}
	else
	{
		if (selectedoption == MEDIA_ADDLIST || selectedoption == MEDIA_ADDTRACK)
		{
			if (key == K_TAB)
				Com_CompleateOSFileName(media_iofilename);
			else if (key == K_BACKSPACE)
			{				
				dir = strlen(media_iofilename);
				if (dir)
					media_iofilename[dir-1] = '\0';
			}
			else if ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9') || key == '/' || key == '_' || key == '.' || key == ':')
			{
				dir = strlen(media_iofilename);
				media_iofilename[dir] = key;
				media_iofilename[dir+1] = '\0';
			}
		}
		else if (selectedoption>=0)
		{
			mediatrack_t *prevtrack, *tr;
			int num=0;
			tr=tracks;
			while(tr)
			{
				if (num == selectedoption)		
					break;				

				prevtrack = tr;
				tr=tr->next;
				num++;
			}
			if (!tr)
				return;

			if (key == K_BACKSPACE)
			{				
				dir = strlen(tr->nicename);
				if (dir)
					tr->nicename[dir-1] = '\0';
			}
			else if ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9') || key == '/' || key == '_' || key == '.' || key == ':'  || key == '&' || key == '|' || key == '#' || key == '\'' || key == '\"' || key == '\\' || key == '*' || key == '@' || key == '!' || key == '(' || key == ')' || key == '%' || key == '^' || key == '?' || key == '[' || key == ']' || key == ';' || key == ':' || key == '+' || key == '-' || key == '=')
			{
				dir = strlen(tr->nicename);
				tr->nicename[dir] = key;
				tr->nicename[dir+1] = '\0';
			}
		}
	}
}





//safeprints only.
void Media_LoadTrackNames (char *listname)
{	
	char *lineend;
	char *len;
	char *filename;
	char *trackname;
	mediatrack_t *newtrack;
	char *data = COM_LoadTempFile(listname);

	loadedtracknames=true;

	if (!data)
		return;

	if (!Q_strncasecmp(data, "#extm3u", 7))
	{
		data = strchr(data, '\n')+1;
		for(;;)
		{
			lineend = strchr(data, '\n');

			if (Q_strncasecmp(data, "#extinf:", 8))
			{
				if (!lineend)
					return;
				Con_SafePrintf("Bad m3u file\n");
				return;
			}
			len = data+8;
			trackname = strchr(data, ',')+1;
			if (!trackname)
				return;

			lineend[-1]='\0';

			filename = data = lineend+1;

			lineend = strchr(data, '\n');

			if (lineend)
			{
				lineend[-1]='\0';
				data = lineend+1;
			}

			newtrack = Z_Malloc(sizeof(mediatrack_t));
#ifndef _WIN32	//crossplatform - lcean up any dos names
			if (filename[1] == ':')
			{
				snprintf(newtrack->filename, sizeof(newtrack->filename)-1, "/mnt/%c/%s", filename[0]-'A'+'a', filename+3);
				while((filename = strchr(newtrack->filename, '\\')))
					*filename = '/';
			
			}
			else
#endif
				Q_strncpyz(newtrack->filename, filename, sizeof(newtrack->filename));
			Q_strncpyz(newtrack->nicename, trackname, sizeof(newtrack->filename));
			newtrack->length = atoi(len);
			newtrack->next = tracks;
			tracks = newtrack;
			numtracks++;

			if (!lineend)
				return;
		}
	}
	else
	{
		for(;;)
		{
			trackname = filename = data;
			lineend = strchr(data, '\n');

			if (!lineend && !*data)
				break;
			lineend[-1]='\0';			
			data = lineend+1;

			newtrack = Z_Malloc(sizeof(mediatrack_t));
			Q_strncpyz(newtrack->filename, filename, sizeof(newtrack->filename));
			Q_strncpyz(newtrack->nicename, COM_SkipPath(trackname), sizeof(newtrack->filename));
			newtrack->length = 0;
			newtrack->next = tracks;
			tracks = newtrack;
			numtracks++;

			if (!lineend)
				break;
		}
	}
}

//safeprints only.
char *Media_NextTrack(void)
{
#ifdef WINAMP
	if (media_hijackwinamp.value)
	{
		WinAmp_Think();
		return NULL;
	}
#endif
	if (bgmvolume.value <= 0 || !media_playing)
		return NULL;

	if (!loadedtracknames)
		Media_LoadTrackNames("sound/media.m3u");
	if (!tracks)
	{
		*currenttrack.filename='\0';
		*currenttrack.nicename='\0';
		lasttrackplayed=-1;
		media_playing = false;
		return NULL;
	}

//	if (cursndcard == sndcardinfo)	//figure out the next track (primary sound card, we could actually get different tracks on different cards (and unfortunatly do))
//	{
		Media_EvaluateNextTrack();
//	}
	return currenttrack.filename;
}







//Avi files are specific to windows. Bit of a bummer really.
#if defined(_WIN32) && !defined(__GNUC__)
#define WINAVI
#endif









#undef	dwFlags
#undef	lpFormat
#undef	lpData
#undef	cbData
#undef	lTime





///temporary residence for media handling
#include "roq.h"
roq_info *roqfilm;

sfxcache_t *moviesoundbuffer;
sfx_t mediaaudio = {
	"movieaudio",
	{NULL, true},
	NULL
};

qbyte *staticfilmimage;	//rgba
int imagewidth;
int imageheight;

#ifdef WINAVI
#undef CDECL	//windows is stupid at times.
#define CDECL __cdecl
#include <vfw.h>
AVISTREAMINFO		psi;										// Pointer To A Structure Containing Stream Info
PAVISTREAM			pavivideo=NULL;
PAVISTREAM			pavisound=NULL;
PAVIFILE			pavi=NULL;
PGETFRAME			pgf=NULL;

LPWAVEFORMAT pWaveFormat;

HWND capturewindow;

int aviinited;

int filmwidth;
int filmheight;
float filmfps;
int num_frames;
int currentframe;
float filmstarttime;
int soundpos;
#pragma comment( lib, "vfw32.lib" )
#endif

#define MFT_CAPTURE 5 //fixme

media_filmtype_t media_filmtype;

int filmnwidth;
int filmnheight;

qboolean Media_PlayFilm(char *name)
{
	char *dot;
	sfx_t *s;
	soundcardinfo_t *sc;

	switch(media_filmtype)	//shut down the old media.
	{
	case MFT_ROQ:
		roq_close(roqfilm);
		roqfilm=NULL;

		for (sc = sndcardinfo; sc; sc=sc->next)
		{
			s = sc->channel[NUM_AMBIENTS].sfx;
			if (s && s == &mediaaudio)
			{
				sc->channel[NUM_AMBIENTS].pos = 0;
				sc->channel[NUM_AMBIENTS].end = 0;
				sc->channel[NUM_AMBIENTS].sfx = NULL;
			}
		}
		break;

	case MFT_STATIC:
		BZ_Free(staticfilmimage);
		staticfilmimage = NULL;
		break;

#ifdef WINAVI
	case MFT_AVI:
		AVIStreamGetFrameClose(pgf);
		AVIStreamEndStreaming(pavivideo);
		AVIStreamRelease(pavivideo);
//		AVIFileRelease(pavi);

		for (sc = sndcardinfo; sc; sc=sc->next)
		{
			s = sc->channel[NUM_AMBIENTS].sfx;
			if (s && s == &mediaaudio)
			{
				sc->channel[NUM_AMBIENTS].pos = 0;
				sc->channel[NUM_AMBIENTS].end = 0;
				sc->channel[NUM_AMBIENTS].sfx = NULL;
			}
		}
		break;
#else
	case MFT_AVI:
		break;
#endif

	case MFT_CIN:
		CIN_FinishCinematic();
		break;
#if 0
	case MFT_CAPTURE:
		if (capturewindow)
		{
			capCaptureStop(capturewindow);
			capDriverDisconnect(capturewindow);
			DestroyWindow(capturewindow);
		}
		break;
#endif

	case MFT_NONE:
		break;
	}

	media_filmtype = MFT_NONE;


	if (!name || !*name)	//clear only.
		return false;

	dot = strchr(name, '.');	//q2 cinematics work like this.
	if (dot && (!strcmp(dot, ".pcx") || !strcmp(dot, ".tga") || !strcmp(dot, ".png") || !strcmp(dot, ".jpg")))
	{
		char fullname[MAX_QPATH];
		qbyte *file;
		qbyte *ReadPCXFile(qbyte *buf, int length, int *width, int *height);
		qbyte *ReadTargaFile(qbyte *buf, int length, int *width, int *height, int asgrey);
		qbyte *ReadJPEGFile(qbyte *infile, int length, int *width, int *height);
		qbyte *ReadPNGFile(qbyte *buf, int length, int *width, int *height);

		sprintf(fullname, "pics/%s", name);
		file = COM_LoadMallocFile(fullname);	//read file
		if (!file)
			return false;

		if ((staticfilmimage = ReadPCXFile(file, com_filesize, &imagewidth, &imageheight)) ||	//convert to 32 rgba if not corrupt
			(staticfilmimage = ReadTargaFile(file, com_filesize, &imagewidth, &imageheight, false)) ||
#ifdef AVAIL_JPEGLIB
			(staticfilmimage = ReadJPEGFile(file, com_filesize, &imagewidth, &imageheight)) ||
#endif
#ifdef AVAIL_PNGLIB
			(staticfilmimage = ReadPNGFile(file, com_filesize, &imagewidth, &imageheight)) ||
#endif
			0)
		{
			BZ_Free(file);	//got image data
		}
		else
		{
			Con_Printf("Static cinematic format not supported.\n");	//not supported format
			return false;
		}

		Con_ClearNotify();
		if (key_dest != key_console)
			scr_con_current=0;
		media_filmtype = MFT_STATIC;
		return true;
	}
	if (dot && (!strcmp(dot, ".cin")))
	{
		if (CIN_PlayCinematic(name))
			media_filmtype = MFT_CIN;
		return true;
	}
	if ((roqfilm = roq_open(name)))
	{
		Con_ClearNotify();
		if (key_dest != key_console)
			scr_con_current=0;
		media_filmtype = MFT_ROQ;
		return true;
	}
#ifdef WINAVI

#if 0
	if (dot && (!strcmp(dot, ".cap")))
	{
		char drivername[256];
		capturewindow = capCreateCaptureWindow("Capture Window", WS_OVERLAPPEDWINDOW, 0, 0, 512, 512, mainwindow, 0);
		ShowWindow(capturewindow, SW_NORMAL);



		capDriverConnect(capturewindow, atoi(name));
		drivername[0] = '\0';
		capDriverGetName(capturewindow, drivername, sizeof(drivername));
//		capDlgVideoSource(capturewindow);
		capCaptureSequenceNoFile(capturewindow);
		Con_Printf("%s", drivername);
		media_filmtype = MFT_CAPTURE;
		return false;
	}
#endif

	if (!aviinited)
	{
		aviinited=true;
		AVIFileInit();
	}
	if (!AVIFileOpen(&pavi, name, OF_READ, NULL))//!AVIStreamOpenFromFile(&pavi, name, streamtypeVIDEO, 0, OF_READ, NULL))
	{
		if (AVIFileGetStream(pavi, &pavivideo, streamtypeVIDEO, 0))	//retrieve video stream
		{
			AVIFileRelease(pavi);
			Con_Printf("%s contains no video stream\n", name);
			return false;
		}
		if (AVIFileGetStream(pavi, &pavisound, streamtypeAUDIO, 0))	//retrieve audio stream
		{
			Con_Printf("%s contains no audio stream\n", name);
			pavisound=NULL;
		}
		AVIFileRelease(pavi);

//play with video
		AVIStreamInfo(pavivideo, &psi, sizeof(psi));
		filmwidth=psi.rcFrame.right-psi.rcFrame.left;					// Width Is Right Side Of Frame Minus Left
		filmheight=psi.rcFrame.bottom-psi.rcFrame.top;					// Height Is Bottom Of Frame Minus Top

		num_frames=AVIStreamLength(pavivideo);							// The Last Frame Of The Stream
		filmfps=1000.0f*(float)num_frames/(float)AVIStreamSampleToTime(pavivideo,num_frames);		// Calculate Rough Milliseconds Per Frame

		for (filmnwidth = 1; filmnwidth<filmwidth; filmnwidth*=2)
			;
		for (filmnheight = 1; filmnheight<filmheight; filmnheight*=2)
			;


		AVIStreamBeginStreaming(pavivideo, 0, num_frames, 100);

		pgf=AVIStreamGetFrameOpen(pavivideo, NULL);

		currentframe=0;
		filmstarttime = Sys_DoubleTime();

soundpos=0;


//play with sound
		if (pavisound)
		{
			LONG lSize;
			LPBYTE pChunk;
			AVIStreamRead(pavisound, 0, AVISTREAMREAD_CONVENIENT, NULL, 0, &lSize, NULL);

			if (!lSize)
				pWaveFormat = NULL;
			else
			{

				pChunk = BZ_Malloc(sizeof(qbyte)*lSize);


				if(AVIStreamReadFormat(pavisound, AVIStreamStart(pavisound), pChunk, &lSize))
				{
				   // error
					Con_Printf("Failiure reading sound info\n");
				}
				pWaveFormat = (LPWAVEFORMAT)pChunk;
			}

			if (!pWaveFormat)
			{
				Con_Printf("VFW is broken\n");
				AVIStreamRelease(pavisound);
				pavisound=NULL;
			}
			else if (pWaveFormat->wFormatTag != 1)
			{
				Con_Printf("Audio stream is not PCM\n");	//FIXME: so that it no longer is...
				AVIStreamRelease(pavisound);
				pavisound=NULL;
			}

		}




		Con_ClearNotify();
		if (key_dest != key_console)
			scr_con_current=0;

		media_filmtype = MFT_AVI;
		return true;
	}
#endif
	
	Con_Printf("Failed to find file %s\n", name);
	return false;
}

qboolean Media_ShowFilm(void)
{
//	sfx_t *s;
	static qbyte framedata[1024*1024*4];
	static float lastframe=0;
//	soundcardinfo_t *sc;

	float curtime = Sys_DoubleTime();
	
	switch (media_filmtype)
	{
	case MFT_ROQ:
		if (curtime<lastframe || roq_read_frame(roqfilm)==1)	 //0 if end, -1 if error, 1 if success
		{			
		//#define LIMIT(x) ((x)<0xFFFF)?(x)>>16:0xFF;
#define LIMIT(x) ((((x) > 0xffffff) ? 0xff0000 : (((x) <= 0xffff) ? 0 : (x) & 0xff0000)) >> 16)
			unsigned char *pa=roqfilm->y[0];
			unsigned char *pb=roqfilm->u[0];
			unsigned char *pc=roqfilm->v[0];
			int pixel=0;
			int num_columns=(roqfilm->width)>>1;
			int y;
			int x;
			if (!(curtime<lastframe))	//roq file was read properly
			{
				lastframe += 1/30.0;	//add a little bit of extra speed so we cover up a little bit of glitchy sound... :o)

				if (lastframe < curtime)
					lastframe = curtime;

				for(y = 0; y < roqfilm->height; ++y)	//roq playing doesn't give nice data. It's still fairly raw.
				{										//convert it properly.
					for(x = 0; x < num_columns; ++x)
					{
						
						int r, g, b, y1, y2, u, v, t;
						y1 = *(pa++); y2 = *(pa++);
						u = pb[x] - 128;
						v = pc[x] - 128;

						y1 <<= 16;
						y2 <<= 16;
						r = 91881 * v;
						g = -22554 * u + -46802 * v;
						b = 116130 * u;

						t=r+y1;
						framedata[pixel] =(unsigned char) LIMIT(t);
						t=g+y1;
						framedata[pixel+1] =(unsigned char) LIMIT(t);
						t=b+y1;
						framedata[pixel+2] =(unsigned char) LIMIT(t);

						t=r+y2;
						framedata[pixel+4] =(unsigned char) LIMIT(t);
						t=g+y2;
						framedata[pixel+5] =(unsigned char) LIMIT(t);
						t=b+y2;
						framedata[pixel+6] =(unsigned char) LIMIT(t);
						pixel+=8;

					}
					if(y & 0x01) { pb += num_columns; pc += num_columns; }
				}	
			}
			else if (vid.numpages == 1)	//previous frame is still in page.
			{
				SCR_SetUpToDrawConsole();	//animate the console at the right speed, but don't bother drawing it.
				return true;
			}

			Media_ShowFrameRGBA_32(framedata, roqfilm->width, roqfilm->height);

			if (roqfilm->audio_channels && sndcardinfo && roqfilm->aud_pos < roqfilm->vid_pos)
			if (roq_read_audio(roqfilm)>0)
					S_RawAudio(-1, roqfilm->audio, 22050, roqfilm->audio_size/roqfilm->audio_channels/2, roqfilm->audio_channels, 2);

			return true;
		}

		Media_PlayFilm(NULL);

		return false;




	case MFT_STATIC:
		Media_ShowFrameRGBA_32(staticfilmimage, imagewidth, imageheight);
		return true;

#ifdef WINAVI
	case MFT_AVI:
		{
			LPBITMAPINFOHEADER lpbi;									// Holds The Bitmap Header Information

			currentframe = (curtime - filmstarttime)*filmfps;

			if (currentframe>=num_frames)
			{			
				Media_PlayFilm(NULL);
				return false;
			}

			lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(pgf, currentframe);	// Grab Data From The AVI Stream
			currentframe++;
			if (!lpbi || lpbi->biBitCount != 24)//oops
			{		
				SCR_SetUpToDrawConsole();
#ifdef SWQUAKE
				D_EnableBackBufferAccess ();	// of all overlay stuff if drawing directly
#endif
				Draw_ConsoleBackground(vid.height);
				Draw_String(0, 0, "Video stream is corrupt\n");			
			}
			else
			{
				Media_ShowFrameBGR_24_Flip((char*)lpbi+lpbi->biSize, lpbi->biWidth, lpbi->biHeight);
			}

			if (pavisound)
			{
				LONG lSize;
				LPBYTE pBuffer;
				AVIStreamRead(pavisound, soundpos, AVISTREAMREAD_CONVENIENT,
				   NULL, 0, &lSize, NULL);

				soundpos+=lSize;

				pBuffer = framedata;


				AVIStreamRead(pavisound, 0, AVISTREAMREAD_CONVENIENT,   pBuffer, lSize, NULL, NULL);

				S_RawAudio(-1, pBuffer, pWaveFormat->nSamplesPerSec, lSize, pWaveFormat->nChannels, 2);

			}
		}
		return true;
#else
	case MFT_AVI:
		break;
#endif
	case MFT_CIN:
		if (CIN_RunCinematic())
		{
			CIN_DrawCinematic();
			return true;
		}
		break;
#ifdef WINAVI
	case MFT_CAPTURE:

		return false;
#endif
	case MFT_NONE:
		break;
	}
	Media_PlayFilm(NULL);
	return false;
}

void Media_PlayFilm_f (void)
{
	if (Cmd_Argc() < 2)
	{
		Con_Printf("playfilm <filename>");
	}
	if (!strcmp(Cmd_Argv(0), "cinematic"))
		Media_PlayFilm(va("video/%s", Cmd_Argv(1)));
	else
		Media_PlayFilm(Cmd_Argv(1));
}


#if defined(RGLQUAKE) && defined(WINAVI)
#define WINAVIRECORDING
PAVIFILE recordavi_file;
#define recordavi_video_stream (recordavi_codec_fourcc?recordavi_compressed_video_stream:recordavi_uncompressed_video_stream)
PAVISTREAM recordavi_uncompressed_video_stream;
PAVISTREAM recordavi_compressed_video_stream;
PAVISTREAM recordavi_uncompressed_audio_stream;
WAVEFORMATEX recordavi_wave_format;
unsigned long recordavi_codec_fourcc;
int recordavi_video_frame_counter;
int recordavi_audio_frame_counter;
float recordavi_frametime;
float recordavi_videotime;
float recordavi_audiotime;
int capturesize;
int capturewidth;
char *capturevideomem;
short *captureaudiomem;
int captureaudiosamples;
captureframe;
cvar_t capturerate = {"capturerate", "15"};
cvar_t capturecodec = {"capturecodec", "divx"};
cvar_t capturesound = {"capturesound", "1"};
cvar_t capturemessage = {"capturemessage", ""};
qboolean recordingdemo;
enum {
	CT_NONE,
	CT_AVI,
	CT_SCREENSHOT
} capturetype;
char capturefilenameprefix[MAX_QPATH];
void Media_RecordFrame (void)
{
	HRESULT hr;
	char *framebuffer = capturevideomem;
	qbyte temp;
	int i, c;

	if (!capturetype)
		return;

	if (recordingdemo)
		if (scr_con_current > 0)
		{
			scr_con_current=0;
			key_dest = key_game;
			return;
		}

//overlay this on the screen, so it appears in the film
	if (*capturemessage.string)
	{
		int y = vid.height -32-16;
		if (y < scr_con_current) y = scr_con_current;
		if (y > vid.height-8)
			y = vid.height-8;
		Draw_String(0, y, capturemessage.string);
	}

	//time for annother frame?
	/*if (recordavi_uncompressed_audio_stream)	//sync video to the same frame as audio.
	{
		if (recordavi_video_frame_counter > recordavi_audio_frame_counter)
			goto skipframe;
	}
	else*/
	{
		if (recordavi_videotime > realtime)
			goto skipframe;
		recordavi_videotime += recordavi_frametime;
	}

	switch (capturetype)
	{
	case CT_AVI:
	//ask gl for it
		qglReadPixels (glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, framebuffer ); 

		// swap rgb to bgr
		c = glwidth*glheight*3;
		for (i=0 ; i<c ; i+=3)
		{
			temp = framebuffer[i];
			framebuffer[i] = framebuffer[i+2];
			framebuffer[i+2] = temp;
		}
		//write it
		hr = AVIStreamWrite(recordavi_video_stream, recordavi_video_frame_counter++, 1, framebuffer, glwidth*glheight * 3, AVIIF_KEYFRAME%15, NULL, NULL);
		if (FAILED(hr)) Con_Printf("Recoring error\n");	
		break;

	case CT_SCREENSHOT:
		{
			char filename[MAX_OSPATH];
			sprintf(filename, "%s%i.%s", capturefilenameprefix, captureframe++, capturecodec.string);
			SCR_ScreenShot(filename);
		}
		break;
	case CT_NONE:	//non issue.
		;
	}

	//this is drawn to the screen and not the film
skipframe:
{
	int y = vid.height -32-16;
	if (y < scr_con_current) y = scr_con_current;
	if (y > vid.height-8)
		y = vid.height-8;
	qglColor4f(1, 0, 0, sin(realtime*4)/4+0.75);
	qglEnable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	GL_TexEnv(GL_MODULATE);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Draw_String((strlen(capturemessage.string)+1)*8, y, "RECORDING");
}
}
void Media_RecordAudioFrame (short *sample_buffer, int samples)
{
	HRESULT hr;
	int samps;

	if (capturetype != CT_AVI)
		return;

	if (!recordavi_uncompressed_audio_stream)
		return;

	if (recordingdemo)
		if (scr_con_current > 0)
		{			
			return;
		}

	memcpy(captureaudiomem+captureaudiosamples*2, sample_buffer, samples*2);

	captureaudiosamples+=samples/2;

	samps = captureaudiosamples;//recordavi_wave_format.nSamplesPerSec*recordavi_frametime;

	if (samps < recordavi_wave_format.nSamplesPerSec*recordavi_frametime)
		return;	//not enough - wouldn't make a frame
	samps=recordavi_wave_format.nSamplesPerSec*recordavi_frametime;

	//time for annother frame?
	if (recordavi_audiotime > realtime)
		return;
	recordavi_audiotime += recordavi_frametime;

	captureaudiosamples-=samps;


    hr = AVIStreamWrite(recordavi_uncompressed_audio_stream, recordavi_audio_frame_counter++, 1, captureaudiomem, samps*recordavi_wave_format.nBlockAlign, AVIIF_KEYFRAME, NULL, NULL);
	if (FAILED(hr)) Con_Printf("Recoring error\n");	
//save excess for later.
	memmove(captureaudiomem, captureaudiomem+samps*2, captureaudiosamples*4);

}
void Media_StopRecordFilm_f (void)
{
    if (recordavi_uncompressed_video_stream)	AVIStreamRelease(recordavi_uncompressed_video_stream);
    if (recordavi_compressed_video_stream)		AVIStreamRelease(recordavi_compressed_video_stream);
    if (recordavi_uncompressed_audio_stream)	AVIStreamRelease(recordavi_uncompressed_audio_stream);
    if (recordavi_file)					AVIFileRelease(recordavi_file);	

	if (capturevideomem)	BZ_Free(capturevideomem);
	if (captureaudiomem)	BZ_Free(captureaudiomem);

	recordavi_uncompressed_video_stream=NULL;
	recordavi_compressed_video_stream = NULL;
	recordavi_uncompressed_audio_stream=NULL;
	recordavi_file = NULL;

	capturevideomem = NULL;
	captureaudiomem=NULL;

	recordingdemo=false;

	capturetype = CT_NONE;
}
void Media_RecordFilm_f (void)
{
	char *fourcc = capturecodec.string;
	char filename[256];
	HRESULT hr;
	BITMAPINFOHEADER bitmap_info_header;
	AVISTREAMINFO stream_header;
	FILE *f;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("capture <filename>\nRecords video output in an avi file.\nUse capturerate and capturecodec to configure.\n");
		return;
	}

	if (Cmd_FromServer())	//err... don't think so sony.
		return;

	if (!aviinited)
	{
		aviinited=true;
		AVIFileInit();
	}

	Media_StopRecordFilm_f();

	recordavi_video_frame_counter = 0;
	recordavi_audio_frame_counter = 0;

	if (capturerate.value<=0)
	{
		Con_Printf("Invalid capturerate\n");
		capturerate.value = 15;
	}

	recordavi_frametime = 1/capturerate.value;

	if (fourcc)
	{
		if (!strcmp(fourcc, "tga") ||
			!strcmp(fourcc, "png") ||
			!strcmp(fourcc, "jpg") ||
			!strcmp(fourcc, "pcx"))
		{
			capturetype = CT_SCREENSHOT;
			strcpy(capturefilenameprefix, Cmd_Argv(1));
			captureframe = 0;
		}
		else
		{
			capturetype = CT_AVI;
			recordavi_codec_fourcc = mmioFOURCC(*(fourcc+0), *(fourcc+1), *(fourcc+2), *(fourcc+3));
		}
	}
	else
	{
		recordavi_codec_fourcc = 0;
		capturetype = CT_AVI;	//uncompressed avi
	}


	if (capturetype == CT_AVI)
	{
		_snprintf(filename, 192, "%s%s", com_gamedir, Cmd_Argv(1));
		COM_StripExtension(filename, filename);
		COM_DefaultExtension (filename, ".avi");

		//wipe it.
		f = fopen(filename, "rb");
		if (f)
		{
			fclose(f);
			unlink(filename);
		}

		hr = AVIFileOpen(&recordavi_file, filename, OF_WRITE | OF_CREATE, NULL);
		if (FAILED(hr))
		{
			Con_Printf("Failed to open\n");
			return;
		}


		memset(&bitmap_info_header, 0, sizeof(BITMAPINFOHEADER));
		bitmap_info_header.biSize = 40;
		bitmap_info_header.biWidth = glwidth;
		bitmap_info_header.biHeight = glheight;
		bitmap_info_header.biPlanes = 1;
		bitmap_info_header.biBitCount = 24;
		bitmap_info_header.biCompression = BI_RGB;
		bitmap_info_header.biSizeImage = glwidth*glheight * 3;


		memset(&stream_header, 0, sizeof(stream_header));
		stream_header.fccType = streamtypeVIDEO;
		stream_header.fccHandler = recordavi_codec_fourcc;
		stream_header.dwScale = 100;
		stream_header.dwRate = (unsigned long)(0.5 + 100.0/recordavi_frametime);
		SetRect(&stream_header.rcFrame, 0, 0, glwidth, glheight);  

		hr = AVIFileCreateStream(recordavi_file, &recordavi_uncompressed_video_stream, &stream_header);
		if (FAILED(hr))
		{
			Con_Printf("Couldn't initialise the stream\n");
			Media_StopRecordFilm_f();
			return;
		}

		if (recordavi_codec_fourcc)
		{
			AVICOMPRESSOPTIONS opts;
			AVICOMPRESSOPTIONS* aopts[1] = { &opts };
			memset(&opts, 0, sizeof(opts));        
			opts.fccType = stream_header.fccType;
			opts.fccHandler = recordavi_codec_fourcc;
			// Make the stream according to compression
			hr = AVIMakeCompressedStream(&recordavi_compressed_video_stream, recordavi_uncompressed_video_stream, &opts, NULL);
			if (FAILED(hr))
			{
				Con_Printf("Failed to init compressor\n");
				Media_StopRecordFilm_f();
				return;
			}
		}
		

		hr = AVIStreamSetFormat(recordavi_video_stream, 0, &bitmap_info_header, sizeof(BITMAPINFOHEADER));
		if (FAILED(hr))
		{
			Con_Printf("Failed to set format\n");
			Media_StopRecordFilm_f();
			return;
		}

		if (capturesound.value)
		{
			memset(&recordavi_wave_format, 0, sizeof(WAVEFORMATEX));
			recordavi_wave_format.wFormatTag = WAVE_FORMAT_PCM; 
			recordavi_wave_format.nChannels = 2; // always stereo in Quake sound engine
			recordavi_wave_format.nSamplesPerSec = sndcardinfo->sn.speed;
			recordavi_wave_format.wBitsPerSample = 16; // always 16bit in Quake sound engine
			recordavi_wave_format.nBlockAlign = recordavi_wave_format.wBitsPerSample/8 * recordavi_wave_format.nChannels; 
			recordavi_wave_format.nAvgBytesPerSec = recordavi_wave_format.nSamplesPerSec * recordavi_wave_format.nBlockAlign; 
			recordavi_wave_format.cbSize = 0; 

			
			memset(&stream_header, 0, sizeof(stream_header));
			stream_header.fccType = streamtypeAUDIO;
			stream_header.dwScale = recordavi_wave_format.nBlockAlign;
			stream_header.dwRate = stream_header.dwScale * (unsigned long)recordavi_wave_format.nSamplesPerSec;
			stream_header.dwSampleSize = recordavi_wave_format.nBlockAlign;

			hr = AVIFileCreateStream(recordavi_file, &recordavi_uncompressed_audio_stream, &stream_header);
			if (FAILED(hr)) return;

			hr = AVIStreamSetFormat(recordavi_uncompressed_audio_stream, 0, &recordavi_wave_format, sizeof(WAVEFORMATEX));
			if (FAILED(hr)) return;
		}


		recordavi_videotime = realtime;
		recordavi_audiotime = realtime;

		captureaudiomem = BZ_Malloc(recordavi_wave_format.nSamplesPerSec*2);

		capturevideomem = BZ_Malloc(glwidth*glheight*3);
	}
}
void Media_CaptureDemoEnd(void)
{
	if (recordingdemo)
		Media_StopRecordFilm_f();
}
void Media_RecordDemo_f(void)
{
	CL_PlayDemo_f();
	Media_RecordFilm_f();
	scr_con_current=0;
	key_dest = key_game;

	if (recordavi_video_stream)
		recordingdemo = true;
}
#else
void Media_CaptureDemoEnd(void){}
void Media_RecordAudioFrame (short *sample_buffer, int samples){}
void Media_RecordFrame (void) {}
#endif
void Media_Init(void)
{
	Cmd_AddCommand("playfilm", Media_PlayFilm_f);
	Cmd_AddCommand("cinematic", Media_PlayFilm_f);
	Cmd_AddCommand("music_fforward", Media_FForward_f);
	Cmd_AddCommand("music_rewind", Media_Rewind_f);
	Cmd_AddCommand("music_next", Media_Next_f);

#ifdef WINAVIRECORDING
	Cmd_AddCommand("capture", Media_RecordFilm_f);
	Cmd_AddCommand("capturedemo", Media_RecordDemo_f);
	Cmd_AddCommand("capturestop", Media_StopRecordFilm_f);

	Cvar_Register(&capturemessage,	"AVI capture controls");
	Cvar_Register(&capturesound,	"AVI capture controls");
	Cvar_Register(&capturerate,	"AVI capture controls");
	Cvar_Register(&capturecodec,	"AVI capture controls");
#endif

#ifdef WINAMP
	Cvar_Register(&media_hijackwinamp,	"Media player things");
#endif
	Cvar_Register(&media_shuffle,	"Media player things");
	Cvar_Register(&media_repeat,	"Media player things");
}







#else
void Media_Init(void){}
void M_Media_Draw (void){}
void M_Media_Key (int key) {}
qboolean Media_ShowFilm(void){return false;}
qboolean Media_PlayFilm(char *name) {return false;}

void Media_RecordFrame (void) {}
void Media_CaptureDemoEnd(void) {}
void Media_RecordDemo_f(void) {}
void Media_RecordAudioFrame (short *sample_buffer, int samples) {}
void Media_StopRecordFilm_f (void) {}
void Media_RecordFilm_f (void){}
void M_Menu_Media_f (void) {}
char *Media_NextTrack(void) {return NULL;}

int filmtexture;
media_filmtype_t media_filmtype;

#endif
