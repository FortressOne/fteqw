/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef __GNUC__
	#define LittleLong(x) ({ typeof(x) _x = (x); _x = (((unsigned char *)&_x)[0]|(((unsigned char *)&_x)[1]<<8)|(((unsigned char *)&_x)[2]<<16)|(((unsigned char *)&_x)[3]<<24)); _x; })
	#define LittleShort(x) ({ typeof(x) _x = (x); _x = (((unsigned char *)&_x)[0]|(((unsigned char *)&_x)[1]<<8)); _x; })
#else
	#define LittleLong(x) (x)
	#define LittleShort(x) (x)
#endif

//this is for a future version
//#define COMMENTARY

//each server that we are connected to has it's own state.
//it should be easy enough to use one thread per server.

//mvd info is forwarded to other proxies instantly
//qwd stuff is buffered and delayed. :(

//this means that when a new proxy connects, we have to send initial state as well as a chunk of pending state, expect to need to send new data before the proxy even has all the init stuff. We may need to raise MAX_PROXY_BUFFER to be larger than on the server

#ifdef __GNUC__
	#define PRINTFWARNING(x) __attribute__((format(printf, x, (x+1))))
#else
	#define PRINTFWARNING(x) /*nothing*/
#endif

//how does multiple servers work
//each proxy acts as a cluster of connections to servers
//when a viewer connects, they are given a list of active server connections
//if there's only one server connection, they are given that one automatically.

#if defined(__APPLE__) && defined(__MACH__)
#define MACOSX
#endif

#include <stdio.h>
/*work around fucked MSVC functions. we use our own for these*/
#if _MSC_VER >= 1300
	#include <string.h>
	#ifndef _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#define vsnprintf q_vsnprintf /*msvc doesn't null terminate. its insecute and thus useless*/
	#define stricmp _stricmp /*msvc just doesn't work properly*/
	#define chdir _chdir
	#define gwtcwd _getcwd
#endif

#ifdef _WIN32
	#include <conio.h>
	#include <winsock2.h>	//this includes windows.h and is the reason for much compiling slowness with windows builds.
	#ifdef IPPROTO_IPV6
		#include <ws2tcpip.h>
	#else
		#define	IPPROTO_IPV6
		#define EAI_NONAME 8
		struct ip6_scope_id
		{
			union
			{
				struct
				{
					u_long  Zone : 28;
					u_long  Level : 4;
				};
				u_long  Value;
			};
		};
		struct in6_addr
		{
			u_char	s6_addr[16];	/* IPv6 address */
		};
		typedef struct sockaddr_in6
		{
			short  sin6_family;
			u_short  sin6_port;
			u_long  sin6_flowinfo;
			struct in6_addr  sin6_addr;
			union
			{
				u_long  sin6_scope_id;
				struct ip6_scope_id  sin6_scope_struct; 
			};
		};

		struct addrinfo
		{
		  int ai_flags;
		  int ai_family;
		  int ai_socktype;
		  int ai_protocol;
		  size_t ai_addrlen;
		  char* ai_canonname;
		  struct sockaddr * ai_addr;
		  struct addrinfo * ai_next;
		};
	#endif
	#ifdef _MSC_VER
		#pragma comment (lib, "wsock32.lib")
	#endif
	#define qerrno WSAGetLastError()
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#define EINPROGRESS WSAEINPROGRESS
	#define ECONNREFUSED WSAECONNREFUSED
	#define ENOTCONN WSAENOTCONN

	//we have special functions to properly terminate sprintf buffers in windows.
	//we assume other systems are designed with even a minor thought to security.
	#if !defined(__MINGW32_VERSION)
		#define unlink _unlink	//why do MS have to be so awkward?
		int snprintf(char *buffer, int buffersize, char *format, ...) PRINTFWARNING(3);
		int vsnprintf(char *buffer, int buffersize, const char *format, va_list argptr);
	#else
		#define unlink remove	//seems mingw misses something
	#endif

	#ifdef _MSC_VER
		//okay, so warnings are here to help... they're ugly though.
		#pragma warning(disable: 4761)	//integral size mismatch in argument
		#pragma warning(disable: 4244)	//conversion from float to short
		#pragma warning(disable: 4018)	//signed/unsigned mismatch
	#endif

#elif defined(__CYGWIN__)

	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/errno.h>
	#include <arpa/inet.h>
	#include <stdarg.h>
	#include <netdb.h>
	#include <sys/ioctl.h>
	#include <unistd.h>

	#define ioctlsocket ioctl
	#define closesocket close

#elif defined(linux) || defined(ixemul) || defined(MACOSX) // I hope by adding MACOSX here it doesnt stop it from being natively built on macosx
	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <stdarg.h>
	#include <stdlib.h>
	#include <string.h>
	#include <errno.h>
	#include <sys/ioctl.h>
	#include <unistd.h>

	#define ioctlsocket ioctl
	#define closesocket close
#elif (defined(__MORPHOS__) && !defined(ixemul))
	#include <stdlib.h>
	#include <unistd.h>

	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <errno.h>

	#define qerrno Errno()
	#define ioctlsocket IoctlSocket
	#define closesocket CloseSocket
#else
#error "Please insert required headers here"
//try the cygwin ones
#endif

#ifndef EAGAIN
#define EAGAIN EWOULDBLOCK
#endif

#ifndef pgetaddrinfo
	#ifndef _WIN32
		#define pgetaddrinfo getaddrinfo
		#define pfreeaddrinfo freeaddrinfo
	#endif
#endif
#ifndef SOCKET
	#define SOCKET int
#endif
#ifndef INVALID_SOCKET
	#define INVALID_SOCKET -1
#endif
#ifndef qerrno
	#define qerrno errno
#endif


#include <stdio.h>
#include <string.h>

#ifndef _WIN32
//stricmp is ansi, strcasecmp is unix.
	#define stricmp strcasecmp
	#define strnicmp strncasecmp
#endif

#define ustrlen(s) strlen((char*)(s))
#define ustrcmp(s1,s2) strcmp((char*)(s1),(char*)(s2))
#define ustrncmp(s1,s2,l) strncmp((char*)(s1),(char*)(s2),l)




#define VERSION "0.01"	//this will be added to the serverinfo

#define PROX_DEFAULTSERVERPORT 27500
#define PROX_DEFAULTLISTENPORT 27501
#define PROX_DEFAULTSERVER "localhost:27500"

#define DEFAULT_HOSTNAME "FTEQTV"

#define MAX_ENTITY_LEAFS 32


#include "protocol.h"




#ifndef __cplusplus
typedef enum {false, true} qboolean;
#else
typedef int qboolean;
extern "C" {
#endif

typedef struct{
	char blob[64];
} netadr_t;


#ifdef COMMENTARY
typedef struct soundcapt_s {
	int (*update)(struct soundcapt_s *ghnd, int samplechunks, char *buffer);
	void (*close)(struct soundcapt_s *ptr);
} soundcapt_t;
typedef struct soundplay_s {
	int (*update)(struct soundplay_s *ghnd, int samplechunks, char *buffer);
	void (*close)(struct soundplay_s *ptr);
} soundplay_t;
#endif

typedef struct {
	unsigned int readpos;
	unsigned int cursize;
	unsigned int maxsize;
	void *data;
	unsigned int startpos;
	qboolean overflowed;
	qboolean allowoverflow;
} netmsg_t;

typedef struct {
	SOCKET sock;
	netadr_t remote_address;
	unsigned short qport;
	unsigned int last_received;
	unsigned int cleartime;

	int maxdatagramlen;
	int maxreliablelen;

	int reliable_length;
	qboolean drop;
	qboolean isclient;
	qboolean isnqprotocol;

	netmsg_t message;
	char message_buf[MAX_MSGLEN];	//reliable message being built
	char reliable_buf[MAX_MSGLEN];	//reliable message that we're making sure arrives.
	float rate;


	unsigned int incoming_acknowledged;
	unsigned int last_reliable_sequence;
	unsigned int incoming_reliable_acknowledged;
	unsigned int incoming_reliable_sequence;
	unsigned int reliable_sequence;

	unsigned int incoming_sequence;
	unsigned int outgoing_sequence;



	unsigned int reliable_start;
	unsigned int outgoing_unreliable;
	unsigned int incoming_unreliable;
	unsigned int in_fragment_length;

	char in_fragment_buf[MAX_NQMSGLEN];
} netchan_t;

typedef struct {
#define MAX_QPATH 64
	char name[MAX_QPATH];
} filename_t;

typedef struct {
	unsigned char frame;
	unsigned char modelindex;
	unsigned char colormap;
	unsigned char skinnum;
	short origin[3];
	char angles[3];
	unsigned char effects;
} entity_state_t;
typedef struct {
	unsigned char frame;
	unsigned char modelindex;
	unsigned char skinnum;
	short origin[3];
	short velocity[3];
	short angles[3];
	unsigned char effects;
	unsigned char weaponframe;
} player_state_t;
typedef struct {
	unsigned int stats[MAX_STATS];
	char userinfo[MAX_USERINFO];

	int ping;
	int packetloss;
	int frags;
	float entertime;

	int leafcount;
	unsigned short leafs[MAX_ENTITY_LEAFS];

	qboolean active:1;
	qboolean gibbed:1;
	qboolean dead:1;
	player_state_t current;
	player_state_t old;
} playerinfo_t;

typedef struct {
	entity_state_t ents[ENTS_PER_FRAME];	//ouchie ouchie!
	unsigned short entnum[ENTS_PER_FRAME];
	int numents;
} packet_entities_t;

typedef struct {
	unsigned char msec;
	unsigned short angles[3];
	short forwardmove, sidemove, upmove;
	unsigned char buttons;
	unsigned char impulse;
} usercmd_t;
extern const usercmd_t nullcmd;


typedef float vec3_t[3];
typedef struct {
	float gravity;
	float maxspeed;
	float spectatormaxspeed;
	float accelerate;
	float airaccelerate;
	float waterfriction;
	float entgrav;
	float stopspeed;
	float wateraccelerate;
	float friction;
} movevars_t;
typedef struct {
	//in / out
	vec3_t origin;
	vec3_t velocity;

	//in
	usercmd_t cmd;
	movevars_t movevars;

	//internal
	vec3_t angles;
	float frametime;
	vec3_t forward, right, up;
} pmove_t;


#define MAX_BACK_BUFFERS	16
typedef struct sv_s sv_t;
typedef struct cluster_s cluster_t;
typedef struct viewer_s {
	qboolean drop;
	unsigned int timeout;
	unsigned int nextpacket;	//for nq clients
	netchan_t netchan;
	qboolean maysend;
	qboolean chokeme;
	qboolean thinksitsconnected;
	qboolean conmenussupported;
	qboolean isproxy;

	int servercount;

	netmsg_t backbuf[MAX_BACK_BUFFERS];	//note data is malloced!
	int backbuffered;

	unsigned int currentstats[MAX_STATS];
	int trackplayer;
	int thisplayer;
	int userid;

	packet_entities_t frame[ENTITY_FRAMES];
	int delta_frames[ENTITY_FRAMES];

	struct viewer_s *next;
	struct viewer_s *commentator;

	char name[32];
	char userinfo[1024];

	int lost;	//packets
	usercmd_t ucmds[3];


	int settime;	//the time that we last told the client.

	vec3_t velocity;
	vec3_t origin;

	int isadmin;
	char expectcommand[16];

	sv_t *server;

	int menuspamtime;
	int menunum;
	int menuop;
	int fwdval;	//for scrolling up/down the menu using +forward/+back :)
	int firstconnect;
} viewer_t;

//'other proxy', these are mvd stream clients.
typedef struct oproxy_s {
	int authkey;
	unsigned int droptime;

	qboolean flushing;
	qboolean drop;

	sv_t *defaultstream;
	sv_t *stream;

	FILE *srcfile;	//buffer is padded with data from this file when its empty
	FILE *file;		//recording a demo (written to)
	SOCKET sock;	//playing to a proxy

	unsigned char inbuffer[MAX_PROXY_INBUFFER];
	unsigned int inbuffersize;	//amount of data available.

	unsigned char buffer[MAX_PROXY_BUFFER];
	unsigned int buffersize;	//use cyclic buffering.
	unsigned int bufferpos;
	struct oproxy_s *next;
} oproxy_t;

typedef struct {
	short origin[3];
	unsigned char soundindex;
	unsigned char volume;
	unsigned char attenuation;
} staticsound_t;

typedef struct bsp_s bsp_t;

typedef struct {
	entity_state_t baseline;
//	entity_state_t current;
//	entity_state_t old;
//	unsigned int updatetime;	//to stop lerping when it's an old entity (bodies, stationary grenades, ...)

	int leafcount;
	unsigned short leafs[MAX_ENTITY_LEAFS];
} entity_t;

#define MAX_ENTITY_FRAMES 64
typedef struct {
	int oldframe;
	int numents;
	int maxents;
	entity_state_t *ents;		//dynamically allocated
	unsigned short *entnums;	//dynamically allocated
} frame_t;

typedef struct {
	unsigned char number;
	char bits[6];
} nail_t;

typedef struct {
	float pos[3];
	float angle[3];
} intermission_t;

typedef enum {
	SRC_BAD,
	SRC_DEMO,
	SRC_UDP,
	SRC_TCP
} sourcetype_t;

typedef enum {
	ERR_NONE,	//stream is fine
	ERR_PAUSED,
	ERR_RECONNECT,	//stream needs to reconnect
	ERR_PERMANENT,	//permanent error, transitioning to disabled next frame
	ERR_DISABLED,	//stream is disabled, can be set to reconnect by admin
	ERR_DROP	//stream _will_ be forgotten about next frame
} errorstate_t;

struct sv_s {	//details about a server connection (also known as stream)
	char connectpassword[64];	//password given to server
	netadr_t serveraddress;
	netchan_t netchan;
	qboolean serverquery;

	sourcetype_t sourcetype;

	//proxy chaining
	qboolean serverisproxy;
	qboolean proxyisselected;
	qboolean upstreamacceptschat;
	qboolean upstreamacceptsdownload;
	//

	unsigned char buffer[MAX_PROXY_BUFFER];	//this doesn't cycle.
	int buffersize;	//it memmoves down
	int forwardpoint;	//the point in the stream that we've forwarded up to.
	qboolean parsingqtvheader;

	unsigned char upstreambuffer[2048];
	int upstreambuffersize;

	unsigned int parsetime;
	unsigned int parsespeed;

	FILE *downloadfile;
	char downloadname[256];

	char status[64];

	qboolean silentstream;

	qboolean usequakeworldprotocols;
	int challenge;
	unsigned short qport;
	int isconnected;
	int clservercount;
	unsigned int nextsendpings;

	unsigned int timeout;
	unsigned int packetratelimiter;

	viewer_t *controller;
	int controllersquencebias;

	qboolean proxyplayer;	//a player is actually playing on the proxy.
	usercmd_t proxyplayerucmds[3];
	int proxyplayerucmdnum;
	int proxyplayerbuttons;
	float proxyplayerangles[3];
	float proxyplayerimpulse;

	qboolean maysend;

	FILE *sourcefile;
	unsigned int filelength;
	SOCKET sourcesock;

//	SOCKET tcpsocket;	//tcp + mvd protocol
//	int tcplistenportnum;

	oproxy_t *proxies;

	qboolean parsingconnectiondata;	//so reject any new connects for now

	unsigned int physicstime;	//the last time all the ents moved.
	unsigned int simtime;
	unsigned int curtime;
	unsigned int oldpackettime;
	unsigned int nextpackettime;
	unsigned int nextconnectattempt;



	errorstate_t errored;
	enum autodisconnect_e {
		AD_NO,
		AD_WHENEMPTY,
		AD_REVERSECONNECT,
		AD_STATUSPOLL
	} autodisconnect;
	unsigned int numviewers;

	cluster_t *cluster;
	sv_t *next;	//next proxy->server connection

#ifdef COMMENTARY
	//audio stuff
	soundcapt_t *comentrycapture;
#endif

	//options:
	char server[MAX_QPATH];
	int streamid;

	struct mapstate_s
	{
		//this structure is freed+memset in QTV_CleanupMap

		bsp_t *bsp;
		int numinlines;

		nail_t nails[32];
		int nailcount;

		char gamedir[MAX_QPATH];
		char mapname[256];
		movevars_t movevars;
		int cdtrack;
		entity_t entity[MAX_ENTITIES];
		frame_t frame[MAX_ENTITY_FRAMES];
	//	int maxents;
		staticsound_t staticsound[MAX_STATICSOUNDS];
		int staticsound_count;
		entity_state_t spawnstatic[MAX_STATICENTITIES];
		int spawnstatic_count;
		filename_t lightstyle[MAX_LIGHTSTYLES];

		char serverinfo[MAX_SERVERINFO_STRING];
		char hostname[MAX_QPATH];
		playerinfo_t players[MAX_CLIENTS];

		filename_t modellist[MAX_MODELS];
		filename_t soundlist[MAX_SOUNDS];
		int modelindex_spike;	// qw is wierd.
		int modelindex_player;	// qw is wierd.

		int trackplayer;
		int thisplayer;
		qboolean ispaused;
	} map;
};

typedef struct {
	char name[64];
	int size;
	int time, smalltime;
} availdemo_t;

#define SOCKETGROUPS 2
struct cluster_s {
	SOCKET qwdsocket[2];	//udp + quakeworld protocols
	SOCKET tcpsocket[2];	//tcp listening socket (for mvd and listings and stuff)

	char commandinput[512];
	int inputlength;

	unsigned int mastersendtime;
	unsigned int mastersequence;
	unsigned int curtime;

	viewer_t *viewers;
	int numviewers;
	sv_t *servers;
	int numservers;
	int nextstreamid;
	int nextuserid;

	sv_t *viewserver;

	//options
	int qwlistenportnum;
	int tcplistenportnum;
	char adminpassword[256];//password required for rcon etc
	char qtvpassword[256];	//password required to connect a proxy
	char hostname[256];
	char master[MAX_QPATH];
	char demodir[MAX_QPATH];
	char downloaddir[MAX_QPATH];	//must be slash terminated, or empty.
	char plugindatasource[256];	//sued by the http server for use with npfte
	char mapsource[256];	//sued by the http server for use with npfte
	qboolean chokeonnotupdated;
	qboolean lateforward;
	qboolean notalking;
	qboolean nobsp;
	qboolean allownqclients;	//nq clients require no challenge
	qboolean nouserconnects;	//prohibit users from connecting to new streams.

	int maxviewers;

	int buildnumber;

	int numproxies;
	int maxproxies;

	qboolean wanttoexit;

	oproxy_t *pendingproxies;

	availdemo_t availdemos[2048];
	int availdemoscount;
};

#define MENU_NONE			0
#define	MENU_MAIN			1//MENU_SERVERS
#define MENU_SERVERS		2
#define MENU_CLIENTS		3
#define MENU_ADMIN			4
#define MENU_ADMINSERVER	5
#define MENU_DEMOS			6
#define MENU_FORWARDING		7
#define MENU_SERVERBROWSER	8
#define MENU_HELP			9

#define	MENU_DEFAULT		MENU_MAIN


enum {
	MENU_MAIN_STREAMS,
	MENU_MAIN_NEWSTREAM,
	MENU_MAIN_SERVERBROWSER,
	MENU_MAIN_PREVPROX,
	MENU_MAIN_HELP,

	MENU_MAIN_CLIENTLIST,
	MENU_MAIN_DEMOS,
	MENU_MAIN_ADMIN,
	MENU_MAIN_NEXTPROX,

	MENU_MAIN_ITEMCOUNT
};



unsigned char ReadByte(netmsg_t *b);
unsigned short ReadShort(netmsg_t *b);
unsigned int ReadLong(netmsg_t *b);
float ReadFloat(netmsg_t *b);
void ReadString(netmsg_t *b, char *string, int maxlen);

unsigned int SwapLong(unsigned int val);
unsigned int BigLong(unsigned int val);






//flags for where a message can be sent, for easy broadcasting
#define Q1 (NQ|QW)
#define QW 1
#define NQ 2
#define CONNECTING 4
#include "cmd.h"


void InitNetMsg(netmsg_t *b, void *buffer, int bufferlength);
unsigned char ReadByte(netmsg_t *b);
unsigned short ReadShort(netmsg_t *b);
unsigned int ReadLong(netmsg_t *b);
float ReadFloat(netmsg_t *b);
void ReadString(netmsg_t *b, char *string, int maxlen);
void WriteByte(netmsg_t *b, unsigned char c);
void WriteShort(netmsg_t *b, unsigned short l);
void WriteLong(netmsg_t *b, unsigned int l);
void WriteFloat(netmsg_t *b, float f);
void WriteString2(netmsg_t *b, const char *str);
void WriteString(netmsg_t *b, const char *str);
void WriteData(netmsg_t *b, const void *data, int length);

void Multicast(sv_t *tv, void *buffer, int length, int to, unsigned int playermask,int suitablefor);
void Broadcast(cluster_t *cluster, void *buffer, int length, int suitablefor);
void ParseMessage(sv_t *tv, void *buffer, int length, int to, int mask);
void BuildServerData(sv_t *tv, netmsg_t *msg, int servercount, viewer_t *spectatorflag);
void BuildNQServerData(sv_t *tv, netmsg_t *msg, qboolean mvd, int servercount);
SOCKET QW_InitUDPSocket(int port, qboolean ipv6);
void QW_UpdateUDPStuff(cluster_t *qtv);
unsigned int Sys_Milliseconds(void);
void Prox_SendInitialEnts(sv_t *qtv, oproxy_t *prox, netmsg_t *msg);
qboolean QTV_Connect(sv_t *qtv, char *serverurl);
void QTV_Shutdown(sv_t *qtv);
qboolean	NET_StringToAddr (char *s, netadr_t *sadr, int defaultport);
void QTV_Printf(sv_t *qtv, char *format, ...) PRINTFWARNING(2);

void SendBufferToViewer(viewer_t *v, const char *buffer, int length, qboolean reliable);
void QW_PrintfToViewer(viewer_t *v, char *format, ...) PRINTFWARNING(2);
void QW_StuffcmdToViewer(viewer_t *v, char *format, ...) PRINTFWARNING(2);
void QW_StreamPrint(cluster_t *cluster, sv_t *server, viewer_t *allbut, char *message);
void QW_StreamStuffcmd(cluster_t *cluster, sv_t *server, char *fmt, ...) PRINTFWARNING(3);
void QTV_SayCommand(cluster_t *cluster, sv_t *qtv, viewer_t *v, char *fullcommand);	//execute a command from a view

void PM_PlayerMove (pmove_t *pmove);

void Netchan_Setup (SOCKET sock, netchan_t *chan, netadr_t adr, int qport, qboolean isclient);
void Netchan_OutOfBandPrint (cluster_t *cluster, SOCKET sock[], netadr_t adr, char *format, ...) PRINTFWARNING(4);
int Netchan_IsLocal (netadr_t adr);
void NET_SendPacket(cluster_t *cluster, SOCKET sock, int length, void *data, netadr_t adr);
SOCKET NET_ChooseSocket(SOCKET sock[], netadr_t *adr);
qboolean Net_CompareAddress(netadr_t *s1, netadr_t *s2, int qp1, int qp2);
qboolean Netchan_Process (netchan_t *chan, netmsg_t *msg);
qboolean NQNetchan_Process(cluster_t *cluster, netchan_t *chan, netmsg_t *msg);
void Netchan_Transmit (cluster_t *cluster, netchan_t *chan, int length, const void *data);
void Netchan_OutOfBand (cluster_t *cluster, SOCKET sock, netadr_t adr, int length, void *data);
qboolean Netchan_CanPacket (netchan_t *chan);

int SendList(sv_t *qtv, int first, const filename_t *list, int svc, netmsg_t *msg);
int Prespawn(sv_t *qtv, int curmsgsize, netmsg_t *msg, int bufnum, int thisplayer);

bsp_t *BSP_LoadModel(cluster_t *cluster, char *gamedir, char *bspname);
void BSP_Free(bsp_t *bsp);
int BSP_LeafNum(bsp_t *bsp, float x, float y, float z);
unsigned int BSP_Checksum(bsp_t *bsp);
int BSP_SphereLeafNums(bsp_t *bsp, int maxleafs, unsigned short *list, float x, float y, float z, float radius);
qboolean BSP_Visible(bsp_t *bsp, int leafcount, unsigned short *list);
void BSP_SetupForPosition(bsp_t *bsp, float x, float y, float z);
const intermission_t *BSP_IntermissionSpot(bsp_t *bsp);

void QW_SetViewersServer(cluster_t *cluster, viewer_t *viewer, sv_t *sv);
unsigned short QCRC_Block (void *start, int count);
unsigned short QCRC_Value(unsigned short crcvalue);
void WriteDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move);
void SendClientCommand(sv_t *qtv, char *fmt, ...) PRINTFWARNING(2);
void QTV_Run(sv_t *qtv);
void QW_FreeViewer(cluster_t *cluster, viewer_t *viewer);
void QW_SetMenu(viewer_t *v, int menunum);

char *COM_ParseToken (char *data, char *out, int outsize, const char *punctuation);
char *Info_ValueForKey (char *s, const char *key, char *buffer, int buffersize);
void Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize);
void ReadDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move);
unsigned Com_BlockChecksum (void *buffer, int length);
void Com_BlockFullChecksum (void *buffer, int len, unsigned char *outbuf);
void Cluster_BuildAvailableDemoList(cluster_t *cluster);

void Sys_Printf(cluster_t *cluster, char *fmt, ...) PRINTFWARNING(2);

void Net_ProxySend(cluster_t *cluster, oproxy_t *prox, void *buffer, int length);
oproxy_t *Net_FileProxy(sv_t *qtv, char *filename);
sv_t *QTV_NewServerConnection(cluster_t *cluster, int streamid, char *server, char *password, qboolean force, enum autodisconnect_e autodisconnect, qboolean noduplicates, qboolean query);
SOCKET Net_TCPListen(int port, qboolean ipv6);
qboolean Net_StopFileProxy(sv_t *qtv);


void SV_FindProxies(SOCKET sock, cluster_t *cluster, sv_t *defaultqtv);
qboolean SV_ReadPendingProxy(cluster_t *cluster, oproxy_t *pend);
void SV_ForwardStream(sv_t *qtv, void *buffer, int length);
int SV_SayToUpstream(sv_t *qtv, char *message);

unsigned char *FS_ReadFile(char *gamedir, char *filename, unsigned int *size);

void ChooseFavoriteTrack(sv_t *tv);

void DemoViewer_Update(sv_t *svtest);


//httpsv.c
void HTTPSV_GetMethod(cluster_t *cluster, oproxy_t *pend);
void HTTPSV_PostMethod(cluster_t *cluster, oproxy_t *pend, char *postdata);


#ifdef __cplusplus
}
#endif

