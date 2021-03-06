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
// client.h


typedef struct
{
	char		name[16];
	int			width;
	int			height;
	int cachedbpp;
	qboolean	failedload;		// the name isn't a valid skin
	cache_user_t	cache;
} skin_t;

// player_state_t is the information needed by a player entity
// to do move prediction and to generate a drawable entity
typedef struct
{
	int			messagenum;		// all player's won't be updated each frame

	double		state_time;		// not the same as the packet time,
								// because player commands come asyncronously
	usercmd_t	command;		// last command for prediction

	vec3_t		origin;
	vec3_t		viewangles;		// only for demos, not from server
	vec3_t		velocity;
	int			weaponframe;

	int			modelindex;
	int			frame;
	int			skinnum;
	int			effects;

#ifdef PEXT_SCALE
	float scale;
#endif
#ifdef PEXT_TRANS
	float trans;
#endif
#ifdef PEXT_FATNESS
	float fatness;
#endif

	int			flags;			// dead, gib, etc

	int			pm_type;
	float		waterjumptime;
	qboolean	onground;
	qboolean	jump_held;
	int			jump_msec;		// hack for fixing bunny-hop flickering on non-ZQuake servers
	int			hullnum;

	float lerpstarttime;
	int oldframe;
} player_state_t;


#if defined(Q2CLIENT) || defined(Q2SERVER)
typedef enum 
{
	// can accelerate and turn
	Q2PM_NORMAL,
	Q2PM_SPECTATOR,
	// no acceleration or turning
	Q2PM_DEAD,
	Q2PM_GIB,		// different bounding box
	Q2PM_FREEZE
} q2pmtype_t;
typedef struct
{	//shared with q2 dll

	q2pmtype_t	pm_type;

	short		origin[3];		// 12.3
	short		velocity[3];	// 12.3
	qbyte		pm_flags;		// ducked, jump_held, etc
	qbyte		pm_time;		// each unit = 8 ms
	short		gravity;
	short		delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters
} q2pmove_state_t;

typedef struct
{	//shared with q2 dll

	q2pmove_state_t	pmove;		// for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t		viewangles;		// for fixed views
	vec3_t		viewoffset;		// add to pmovestate->origin
	vec3_t		kick_angles;	// add to view direction to get render angles
								// set by weapon kicks, pain effects, etc

	vec3_t		gunangles;
	vec3_t		gunoffset;
	int			gunindex;
	int			gunframe;

	float		blend[4];		// rgba full screen effect
	
	float		fov;			// horizontal field of view

	int			rdflags;		// refdef flags

	short		stats[Q2MAX_STATS];		// fast status bar updates
} q2player_state_t;
#endif

#define	MAX_SCOREBOARDNAME	64
#define MAX_DISPLAYEDNAME	16
typedef struct player_info_s
{
	int		userid;
	char	userinfo[MAX_INFO_STRING];

	// scoreboard information
	char	name[MAX_SCOREBOARDNAME];
	char	team[MAX_INFO_KEY];
	float	entertime;
	int		frags;
	int		ping;
	qbyte	pl;

	// skin information
	int		topcolor;
	int		bottomcolor;

	int		_topcolor;
	int		_bottomcolor;

	int		spectator;
	qbyte	translations[VID_GRADES*256];
	skin_t	*skin;

	struct model_s	*model;

	unsigned short vweapindex;

	int prevcount;

	int stats[MAX_CL_STATS];
} player_info_t;


typedef struct
{
	// generated on client side
	usercmd_t	cmd[MAX_SPLITS];		// cmd that generated the frame
	double		senttime;	// time cmd was sent off
	int			delta_sequence;		// sequence number to delta from, -1 = full update
	int			cmd_sequence;

	// received from server
	double		receivedtime;	// time message was received, or -1
	player_state_t	playerstate[MAX_CLIENTS];	// message received that reflects performing
							// the usercmd
	packet_entities_t	packet_entities;
	qboolean	invalid;		// true if the packet_entities delta was invalid
} frame_t;

#ifdef Q2CLIENT
typedef struct
{
	qboolean		valid;			// cleared if delta parsing was invalid
	int				serverframe;
	int				servertime;		// server time the message is valid for (in msec)
	int				deltaframe;
	qbyte			areabits[MAX_Q2MAP_AREAS/8];		// portalarea visibility bits
	q2player_state_t	playerstate;
	int				num_entities;
	int				parse_entities;	// non-masked index into cl_parse_entities array
} q2frame_t;
#endif

typedef struct
{
	int		destcolor[3];
	int		percent;		// 0-256
} cshift_t;

#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define CSHIFT_SERVER	4
#define	NUM_CSHIFTS		5


//
// client_state_t should hold all pieces of the client state
//
#define	MAX_DLIGHTS		256
typedef struct dlight_s
{
	int		key;				// so entities can reuse same entry
	qboolean	noppl, nodynamic, noflash, isstatic;
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	float   color[3];
	float	channelfade[3];

	struct	shadowmesh_s *worldshadowmesh;
	int style;	//multiply by style values if > 0
	float	dist;
	struct dlight_s *next;
} dlight_t;

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
	int colour;
} lightstyle_t;



#define	MAX_EFRAGS		512

#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef enum {
ca_disconnected, 	// full screen console with no connection
ca_demostart,		// starting up a demo
ca_connected,		// netchan_t established, waiting for svc_serverdata
ca_onserver,		// processing data lists, donwloading, etc
ca_active			// everything is in, so frames can be rendered
} cactive_t;

typedef enum {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_wad,
	dl_single,
	dl_singlestuffed
} dltype_t;		// download type

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
// connection information
	cactive_t	state;
#ifdef Q2CLIENT
	qboolean q2server;
#endif

	qboolean resendinfo;

	int framecount;

// network stuff
	netchan_t	netchan;
	float lastarbiatarypackettime;	//used to mark when packets were sent to prevent mvdsv servers from causing us to disconnect.

// private userinfo for sending to masterless servers
	char		userinfo[MAX_INFO_STRING];

	char		servername[MAX_OSPATH];	// name of server from original connect

	int			qport;
	int socketip;
	int socketip6;
	int socketipx;

	enum {DL_NONE, DL_QW, DL_QWCHUNKS, DL_QWPENDING, DL_HTTP, DL_FTP} downloadmethod;
	FILE		*downloadqw;		// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int			downloadnumber;
	dltype_t	downloadtype;
	int			downloadpercent;

// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
	qboolean	demorecording;
	enum{DPB_NONE,DPB_QUAKEWORLD,DPB_MVD,
#ifdef NQPROT
		DPB_NETQUAKE,
#endif
#ifdef Q2CLIENT
		DPB_QUAKE2
#endif
	}	demoplayback;
	qboolean	timedemo;
	FILE		*demofile;
	float		td_lastframe;		// to meter out one message a frame
	int			td_startframe;		// host_framecount at start
	float		td_starttime;		// realtime at second frame of timedemo

	int			challenge;

	float		latency;		// rolling average

	qboolean	allow_anyparticles;
	qboolean	allow_lightmapgamma;
	qboolean	allow_rearview;
	qboolean	allow_skyboxes;
	qboolean	allow_mirrors;
	qboolean	allow_watervis;
	qboolean	allow_shaders;
	qboolean	allow_luma;
	qboolean	allow_bump;
	float		allow_fbskins;	//fraction of allowance
#ifdef FISH
	qboolean	allow_fish;
#endif
	qboolean	allow_cheats;
	qboolean	allow_semicheats;	//defaults to true, but this allows a server to enforce a strict ruleset (smackdown type rules).
	float		maxfps;	//server capped
	enum {GAME_DEATHMATCH, GAME_COOP} gamemode;

#ifdef PROTOCOLEXTENSIONS
	unsigned long fteprotocolextensions;
#endif
	unsigned long z_ext;
#ifdef NQPROT
	struct qsocket_s *netcon;
	int signon;
#endif
	translation_t language;
} client_static_t;

extern client_static_t	cls;

extern int nq_dp_protocol;

typedef struct downloadlist_s {
	char name[128];
	struct downloadlist_s *next;
} downloadlist_t;


typedef struct {
	float lerptime;
#ifdef HALFLIFEMODELS
	float framechange;	//marks time of last frame change - for halflife model sequencing.
#endif
	float lerprate;	//inverse rate...
	vec3_t origin;
	vec3_t angles;
	trailstate_t trailstate;	//when to next throw out a trail
	unsigned short frame;
	unsigned short tagent;
	unsigned short tagindex;
} lerpents_t;
//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
	int			fpd;
	int			servercount;	// server identification for prespawns

	char		serverinfo[MAX_SERVERINFO_STRING];

	int			parsecount;		// server message counter
	int			oldparsecount;
	int			oldvalidsequence;
	int			validsequence;	// this is the sequence number of the last good
								// packetentity_t we got.  If this is 0, we can't
								// render a frame yet
	int			movemessages;	// since connecting to this server
								// throw out the first couple, so the player
								// doesn't accidentally do something the 
								// first frame

	int			spectator;

	double		last_ping_request;	// while showing scoreboard
	double		last_servermessage;

#ifdef Q2CLIENT
	q2frame_t	q2frame;
	q2frame_t	q2frames[Q2UPDATE_BACKUP];
#endif

// sentcmds[cl.netchan.outgoing_sequence & UPDATE_MASK] = cmd
	frame_t		frames[UPDATE_BACKUP];
	lerpents_t	lerpents[MAX_EDICTS];

// information for local display
	int			stats[MAX_SPLITS][MAX_CL_STATS];	// health, etc
	float		item_gettime[MAX_SPLITS][32];	// cl.time of aquiring item, for blinking
	float		faceanimtime[MAX_SPLITS];		// use anim frame if cl.time < this

	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups
	cshift_t	prev_cshifts[NUM_CSHIFTS];	// and content types

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  And only reset at level change
// and teleport times
	vec3_t		viewangles[MAX_SPLITS];

// the client simulates or interpolates movement to get these values
	double		time;			// this is the time value that the client
								// is rendering at.  allways <= realtime
	float gametime;
	float gametimemark;

	vec3_t		simorg[MAX_SPLITS];
	vec3_t		simvel[MAX_SPLITS];
	vec3_t		simangles[MAX_SPLITS];
	float		rollangle[MAX_SPLITS];

	float minpitch;
	float maxpitch;

// pitch drifting vars
	float		pitchvel[MAX_SPLITS];
	qboolean	nodrift[MAX_SPLITS];
	float		driftmove[MAX_SPLITS];
	double		laststop[MAX_SPLITS];


	float		crouch[MAX_SPLITS];			// local amount for smoothing stepups
	qboolean	onground[MAX_SPLITS];
	float		viewheight[MAX_SPLITS];

	qboolean	paused;			// send over by server

	float		punchangle[MAX_SPLITS];		// temporar yview kick from weapon firing
	
	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched ffrom time at intermission start
	
//
// information that is static for the entire time connected to a server
//
	char		model_name[MAX_MODELS][MAX_QPATH];
	char		sound_name[MAX_SOUNDS][MAX_QPATH];
	char		image_name[Q2MAX_IMAGES][MAX_QPATH];

	struct model_s		*model_precache[MAX_MODELS];
	struct sfx_s		*sound_precache[MAX_SOUNDS];

	char				model_csqcname[MAX_CSQCMODELS][MAX_QPATH];
	struct model_s		*model_csqcprecache[MAX_CSQCMODELS];

	char skyname[MAX_QPATH];
	char		levelname[40];	// for display on solo scoreboard
	int			playernum[MAX_SPLITS];
	int			splitclients;	//we are running this many clients split screen.

// refresh related state
	struct model_s	*worldmodel;	// cl_entitites[0].model
	struct efrag_s	*free_efrags;
	int			num_entities;	// stored bottom up in cl_entities array
	int			num_statics;	// stored top down in cl_entitiers

	int			cdtrack;		// cd audio

	entity_t	viewent[MAX_SPLITS];		// weapon model

// all player information
	player_info_t	players[MAX_CLIENTS];

#ifdef CLPROGS
	struct edict_s *edicts;
	int num_edicts;
#endif

	downloadlist_t *downloadlist;
	downloadlist_t *faileddownloads;

#ifdef PEXT_SETVIEW
	int viewentity[MAX_SPLITS];
#endif
	qboolean gamedirchanged;


	char		q2statusbar[1024];
	char		q2layout[1024];
	int parse_entities;
	int surpressCount;
	float lerpfrac;
	vec3_t predicted_origin;
	vec3_t predicted_angles;
	vec3_t prediction_error;
	float predicted_step_time;
	float predicted_step;

	// localized movement vars
	float		entgravity[MAX_SPLITS];
	float		maxspeed[MAX_SPLITS];
	float		bunnyspeedcap;
	qboolean	fixangle;	//received a fixangle - so disable prediction till the next packet.

	int teamplay;
	int deathmatch;

	qboolean teamfortress;	//*sigh*. This is used for teamplay stuff. This sucks.
} client_state_t;

extern int		cl_teamtopcolor;
extern int		cl_teambottomcolor;
extern int		cl_enemytopcolor;
extern int		cl_enemybottomcolor;

//FPD values
//(commented out ones are ones that we don't support)
#define FPD_NO_FORCE_SKIN	256
#define FPD_NO_FORCE_COLOR	512
#define FPD_LIMIT_PITCH		(1 << 14)	//limit scripted pitch changes
#define FPD_LIMIT_YAW		(1 << 15)	//limit scripted yaw changes

//
// cvars
//
extern  cvar_t	cl_warncmd;
extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_shownet;
extern	cvar_t	cl_sbar;
extern	cvar_t	cl_hudswap;

extern	cvar_t	cl_pitchdriftspeed;
extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;

extern cvar_t		_windowed_mouse;

extern	cvar_t	name;


#define	MAX_STATIC_ENTITIES	256			// torches, etc

extern	client_state_t	cl;

// FIXME, allocate dynamically
extern	entity_state_t	cl_baselines[MAX_EDICTS];
extern	efrag_t			cl_efrags[MAX_EFRAGS];
extern	entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t		cl_dlights[MAX_DLIGHTS];

extern	qboolean	nomaster;
extern float	server_version;	// version of server we connected to

//=============================================================================


//
// cl_main
//
dlight_t *CL_AllocDlight (int key);
void	CL_DecayLights (void);
void CL_ParseDelta (struct entity_state_s *from, struct entity_state_s *to, int bits, qboolean);

void CL_Init (void);
void Host_WriteConfiguration (void);
void CL_CheckServerInfo(void);

void CL_EstablishConnection (char *host);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);
qboolean CL_DemoBehind(void);

void CL_BeginServerConnect(void);
void CLNQ_BeginServerConnect(void);

#define			MAX_VISEDICTS	256
extern	int				cl_numvisedicts, cl_oldnumvisedicts;
extern	entity_t		*cl_visedicts, *cl_oldvisedicts;
extern	entity_t		cl_visedicts_list[2][MAX_VISEDICTS];

extern char emodel_name[], pmodel_name[], prespawn_name[], modellist_name[], soundlist_name[];

//
// cl_input
//
typedef struct
{
	int		down[MAX_SPLITS][2];		// key nums holding it down
	int		state[MAX_SPLITS];			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (usercmd_t *cmd);
#ifdef NQPROT
void CL_ParseTEnt (qboolean nqprot);
#else
void CL_ParseTEnt (void);
#endif
void CL_UpdateTEnts (void);

void CL_ClearState (void);

void CL_ReadPackets (void);
void CL_ClampPitch (int pnum);

int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd, int pnum);


float CL_KeyState (kbutton_t *key, int pnum);
char *Key_KeynumToString (int keynum);
int Key_StringToKeynum (char *str, int *modifier);

//
// cl_demo.c
//
void CL_StopPlayback (void);
qboolean CL_GetMessage (void);
void CL_WriteDemoCmd (usercmd_t *pcmd);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_ReRecord_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

//
// cl_parse.c
//
#define NET_TIMINGS 256
#define NET_TIMINGSMASK 255
extern int	packet_latency[NET_TIMINGS];
int CL_CalcNet (void);
void CL_ParseServerMessage (void);
void CLNQ_ParseServerMessage (void);
#ifdef Q2CLIENT
void CLQ2_ParseServerMessage (void);
#endif
void CL_NewTranslation (int slot);
qboolean	CL_CheckOrDownloadFile (char *filename, int nodelay);
qboolean CL_IsUploading(void);
void CL_NextUpload(void);
void CL_StartUpload (qbyte *data, int size);
void CL_StopUpload(void);

void CL_RequestNextDownload (void);
void CL_ParseAttachment(void);

//
// view.c
//
void V_StartPitchDrift (int pnum);
void V_StopPitchDrift (int pnum);

void V_RenderView (void);
void V_UpdatePalette (void);
void V_Register (void);
void V_ParseDamage (int pnum);
void V_SetContentsColor (int contents);
void GLV_CalcBlend (void);


//
// cl_tent
//
void CL_InitTEnts (void);
void CL_ClearTEnts (void);
void CL_ClearCustomTEnts(void);
void CL_ParseCustomTEnt(void);
void CL_ParseEffect (qboolean effect2);

//
// cl_ents.c
//
void CL_SetSolidPlayers (int playernum);
void CL_SetUpPlayerPrediction(qboolean dopred);
void CL_EmitEntities (void);
void CL_ClearProjectiles (void);
void CL_ParseProjectiles (int modelindex, qboolean nails2);
void CL_ParsePacketEntities (qboolean delta);
void CL_SetSolidEntities (void);
void CL_ParsePlayerinfo (void);
void CL_ParseClientPersist(void);
//these last ones are needed for csqc handling of engine-bound ents.
void CL_SwapEntityLists(void);
void CL_LinkViewModel(void);
void CL_LinkPlayers (void);
void CL_LinkPacketEntities (void);
void CL_LinkProjectiles (void);

//
//clq3_parse.c
//
#ifdef Q3CLIENT
void CLQ3_SendClientCommand(const char *fmt, ...);
void CLQ3_SendConnectPacket(netadr_t to);
void CLQ3_SendCmd(usercmd_t *cmd);
qboolean CLQ3_Netchan_Process(void);
void CLQ3_ParseServerMessage (void);
qboolean CG_FillQ3Snapshot(int snapnum, struct snapshot_s *snapshot);

void CG_InsertIntoGameState(int num, char *str);
void CG_Restart_f(void);
#endif

//
//pr_csqc.c
//
#ifdef CSQC_DAT
void CSQC_Init (void);
qboolean CSQC_DrawView(void);
void CSQC_Shutdown(void);
qboolean CSQC_StuffCmd(char *cmd);
qboolean CSQC_CenterPrint(char *cmd);
#endif

//
// cl_pred.c
//
void CL_InitPrediction (void);
void CL_PredictMove (void);
void CL_PredictUsercmd (int pnum, player_state_t *from, player_state_t *to, usercmd_t *u);
#ifdef Q2CLIENT
void CLQ2_CheckPredictionError (void);
#endif

//
// cl_cam.c
//
#define CAM_NONE	0
#define CAM_TRACK	1

extern	int		autocam[MAX_SPLITS];
extern int spec_track[MAX_SPLITS]; // player# of who we are tracking

qboolean Cam_DrawViewModel(int pnum);
qboolean Cam_DrawPlayer(int pnum, int playernum);
void Cam_Track(int pnum, usercmd_t *cmd);
int Cam_TrackNum(int pnum);
void Cam_FinishMove(int pnum, usercmd_t *cmd);
void Cam_Reset(void);
void CL_InitCam(void);

//
//zqtp.c
//
qboolean TP_SoundTrigger(char *message);
char *TP_PlayerName (void);
char *TP_MapName (void);
int	TP_CountPlayers (void);
char *TP_PlayerTeam (void);
char *TP_EnemyTeam (void);
char *TP_EnemyName (void);
void TP_StatChanged (int stat, int value);
int TP_CategorizeMessage (char *s, int *offset);
void TP_NewMap (void);
qboolean TP_FilterMessage (char *s);
qboolean TP_CheckSoundTrigger (char *str);
void TP_SearchForMsgTriggers (char *s, int level);

//
// skin.c
//

typedef struct
{
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
//    unsigned char	data;			// unbounded
} pcx_t;


char *Skin_FindName (player_info_t *sc);
void	Skin_Find (player_info_t *sc);
qbyte	*Skin_Cache8 (skin_t *skin);
qbyte	*Skin_Cache32 (skin_t *skin);
void	Skin_Skins_f (void);
void	Skin_FlushSkin(char *name);
void	Skin_AllSkins_f (void);
void	Skin_NextDownload (void);
void Skin_FlushPlayers(void);

#define RSSHOT_WIDTH 320
#define RSSHOT_HEIGHT 200





//valid.c
void	Validation_FlushFileList(void);
void	ValidationPrintVersion(char *f_query_string);
void	Validation_Server(void);
void	Validation_FilesModified (void);
void	Validation_Skins(void);
void	Validation_CheckIfResponse(char *text);
void	InitValidation(void);

extern	qboolean f_modified_particles;
extern	qboolean care_f_modified;


//random files (fixme: clean up)

#ifdef Q2CLIENT
void CLQ2_ParseTEnt (void);
void CLQ2_AddEntities (void);
void CLQ2_ParseBaseline (void);
void CLQ2_ParseFrame (void);
void CLNQ_ParseEntity(unsigned int bits);
int CLQ2_RegisterTEntModels (void);
#endif

void NQ_R_ParseParticleEffect (void);
void CLNQ_SignonReply (void);
void NQ_BeginConnect(char *to);
void NQ_ContinueConnect(char *to);
int CLNQ_GetMessage (void);

void CL_BeginServerReconnect(void);

void SV_User_f (void);	//called by client version of the function
void SV_Serverinfo_f (void);



#ifdef TEXTEDITOR
extern qboolean editoractive;
extern qboolean editormodal;
void Editor_Draw(void);
void Editor_Init(void);
#endif



void CL_AddVWeapModel(entity_t *player, int model);


typedef enum {
	MFT_NONE,
	MFT_STATIC,	//non-moving, PCX, no sound
	MFT_ROQ,
	MFT_AVI,
	MFT_CIN
} media_filmtype_t;
extern media_filmtype_t media_filmtype;
void Media_Init(void);
qboolean Media_PlayFilm(char *name);
void CIN_FinishCinematic (void);
qboolean CIN_PlayCinematic (char *arg);
qboolean CIN_DrawCinematic (void);
qboolean CIN_RunCinematic (void);

void MVD_Interpolate(void);

void TP_Init(void);
void TP_CheckVars(void);
void TP_CheckPickupSound(char *s, vec3_t org);

void Stats_NewMap(void);
void Stats_ParsePrintLine(char *line);
