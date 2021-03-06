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

#ifndef __BOTHDEFS_H
#define __BOTHDEFS_H


#if defined(__MINGW32_VERSION) || defined(__MINGW__)
	#define MINGW
#endif
#if !defined(MINGW) && defined(__GNUC__) && defined(_WIN32)
	#define MINGW	//Erm, why is this happening?
#endif

#ifdef HAVE_CONFIG_H	//if it was configured properly, then we have a more correct list of features we want to use.
	#include "config.h"
#else

	//#define AVAIL_OGGVORBIS
	#if !defined(__CYGWIN__) && !defined(MINGW)
		#define AVAIL_PNGLIB
		#define AVAIL_JPEGLIB
		#define AVAIL_ZLIB

//		#define AVAIL_OGGVORBIS
	#endif
	#define AVAIL_MASM

	//#define AVAIL_DX7


//set any additional defines or libs in win32
	#ifndef AVAIL_MASM
		#ifdef _WIN32
			#define NOASM
		#else
			#undef AVAIL_MASM	//fixme
		#endif
	#endif

	#define SVRANKING

	#ifdef MINIMAL
		#define CL_MASTER		//this is useful

		#undef AVAIL_MP3		//no mp3 support
		#undef AVAIL_JPEGLIB	//no jpeg support
		#undef AVAIL_PNGLIB		//no png support
		#undef USE_MADLIB		//no internal mp3 playing
		#undef AVAIL_DX7		//no d3d support
		#define NOMEDIA			//NO playing of avis/cins/roqs
		#define NOVOICECHAT		//NO sound recording, tcp streaming and playback on a remote client. not finalised.

		#define MD3MODELS		//we DO want to use quake3 alias models. This might be a minimal build, but we still want this.

#ifndef SERVERONLY	//don't be stupid, stupid.
		#define CLIENTONLY
#endif
	#else

		#define SIDEVIEWS	4	//enable secondary/reverse views.
		#define SP2MODELS		//quake2 sprite models
		#define MD2MODELS		//quake2 alias models
		#define MD3MODELS		//quake3 alias models
		#define ZYMOTICMODELS	//zymotic skeletal models.
		#define HUFFNETWORK		//huffman network compression
		#define HALFLIFEMODELS	//halflife model support (experimental)
		#define DOOMWADS		//doom wad/map/sprite support
		//#define WOLF3DSUPPORT	//wolfenstein3d map support (not started yet)
		#define Q2BSPS			//quake 2 bsp support
		#define Q3BSPS			//quake 3 bsp support
		#define SV_MASTER		//starts up a master server
		#define SVCHAT			//serverside npc chatting. see sv_chat.c
		#define Q2SERVER		//server can run a q2 game dll and switches to q2 network and everything else.
		#define Q2CLIENT		//client can connect to q2 servers
		#define NQPROT			//server and client are capable of using quake1/netquake protocols. (qw is still prefered. uses the command 'nqconnect')
		#define FISH			//sw rendering only
		#define VM_UI			//support userinterfaces within Q3 Virtual Machines
		#define ZLIB			//zip/pk3 support
		#define WEBSERVER		//http/ftp servers
		#define WEBCLIENT		//http/ftp clients.
		#define EMAILSERVER		//smtp/pop3 server should you feel a need
		#define EMAILCLIENT		//smtp/pop3 clients (email notifications)
		#define RUNTIMELIGHTING	//calculate lit/lux files the first time the map is loaded and doesn't have a loadable lit.
		#define QTERM			//qterm... adds a console command that allows running programs from within quake - bit like xterm.
		#define CL_MASTER		//query master servers and stuff for a dynamic server listing.
		#define SERIALMOUSE		//means that the engine talks to a serial mouse directly via the com/serial port. Thus allowing duel mice with seperate inputs.
		#define R_XFLIP			//allow view to be flipped horizontally
		#define IN_XFLIP		//allow input to be flipped horizontally.
		#define TEXTEDITOR
		#define PPL				//per pixel lighting (stencil shadowing)

		#define PLUGINS

#ifdef _DEBUG
		#define CSQC_DAT	//support for csqc
#endif
		#define MENU_DAT	//support for menu.dat

		#define Q3SHADERS

//		#define VOICECHAT	//experimental

//these things were moved to plugins.
//#define IRCCLIENT		//connects to irc servers.

	#endif

#endif

//fix things a little...

#ifdef MINGW
	#undef ZLIB
	#undef AVAIL_ZLIB
#endif

#ifdef USE_MADLIB	//global option. Specify on compiler command line.
	#define AVAIL_MP3	//suposedly anti-gpl. don't use in a distributed binary
#endif

#ifndef ZLIB
	#undef AVAIL_ZLIB		//no zip/pk3 support
#endif

#ifndef _WIN32
	#undef QTERM
#endif

#if (defined(Q2CLIENT) || defined(Q2SERVER))
	#ifndef Q2BSPS
		#error "Q2 game support without Q2BSP support. doesn't make sense"
	#endif
	#if !defined(MD2MODELS) || !defined(SP2MODELS)
		#error "Q2 game support without full Q2 model support. doesn't make sense"
	#endif
#endif

#ifdef NODIRECTX
	#undef AVAIL_DX7
#endif


#ifdef SERVERONLY	//remove options that don't make sense on only a server
	#undef Q2CLIENT
	#undef Q3CLIENT
	#undef WEBCLIENT
	#undef IRCCLIENT
	#undef EMAILCLIENT
	#undef TEXTEDITOR
	#undef RUNTIMELIGHTING
	#undef PLUGINS	//we don't have any server side stuff.
	#undef Q3SHADERS
#endif
#ifdef CLIENTONLY	//remove optional server componants that make no sence on a client only build.
	#undef Q2SERVER
	#undef WEBSERVER
	#undef EMAILSERVER
#endif

//remove any options that depend upon GL.
#ifndef SERVERONLY
	#if defined(SWQUAKE) && !defined(GLQUAKE)
		#undef DOOMWADS
		#undef HALFLIFEMODELS
		#undef Q3BSPS
		#undef R_XFLIP
		#undef RUNTIMELIGHTING
	#endif
#endif

#if !defined(GLQUAKE)
	#undef Q3BSPS
#endif
#if !defined(Q3BSPS)
	#undef Q3SHADERS
	#undef Q3CLIENT //reconsider this (later)
#endif

#ifndef Q3CLIENT
	#undef VM_CG	// :(
#else
	#define VM_CG
#endif

#if defined(VM_UI) || defined(VM_CG)
	#define VM_ANY
#endif

#define PROTOCOLEXTENSIONS

#define PRE_SAYONE	2.487	//FIXME: remove.

// defs common to client and server

#define	VERSION		2.56

//#define VERSION3PART	//add the 3rd decimal point of a more precise version.

#define DISTRIBUTION "FTE"
#define DISTRIBUTIONLONG "Forethought Entertainment"

#define ENGINEWEBSITE "http://fte.quakesrc.org/"

#ifdef _WIN32
#define PLATFORM	"Win32"
#else
#define PLATFORM	"Linux"
#endif


#if (defined(_M_IX86) || defined(__i386__)) && !defined(id386)
#define id386	1
#else
#define id386	0
#endif

#if defined(NOASM)		// no asm in dedicated server
#undef id386
#endif

#if id386
#define UNALIGNED_OK	1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK	0
#endif

#ifdef _MSC_VER
#define VARGS __cdecl
#endif
#ifndef VARGS
#define VARGS
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

#define	MINIMUM_MEMORY	0x550000

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


#define	MAX_SCOREBOARD		16		// max numbers of players

#define	SOUND_CHANNELS		8


#define	MAX_QPATH		64			// max length of a quake game pathname
#define	MAX_OSPATH		256			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon

#define	MAX_NQMSGLEN	8000		// max length of a reliable message
#define MAX_Q2MSGLEN	1400
#define MAX_QWMSGLEN	1450
#define MAX_OVERALLMSGLEN	MAX_NQMSGLEN
#define	MAX_DATAGRAM	1450		// max length of unreliable message
#define MAX_Q2DATAGRAM	MAX_Q2MSGLEN
#define	MAX_NQDATAGRAM	1024		// max length of unreliable message
#define MAX_OVERALLDATAGRAM MAX_DATAGRAM

#define MAX_BACKBUFLEN	1200

//
// per-level limits
//
#define	MAX_EDICTS		2048			// FIXME: ouch! ouch! ouch!
#define	MAX_LIGHTSTYLES	64
#define	MAX_MODELS		512			// these are sent over the net as bytes
#define	MAX_SOUNDS		256			// so they cannot be blindly increased

#define	MAX_CSQCMODELS		256			// these live entirly clientside

#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

//
// stats are integers communicated to the client by the server
//
#define MAX_QW_STATS 32
enum {
STAT_HEALTH			= 0,
//STAT_FRAGS		= 1,
STAT_WEAPON			= 2,
STAT_AMMO			= 3,
STAT_ARMOR			= 4,
STAT_WEAPONFRAME	= 5,
STAT_SHELLS			= 6,
STAT_NAILS			= 7,
STAT_ROCKETS		= 8,
STAT_CELLS			= 9,
STAT_ACTIVEWEAPON	= 10,
STAT_TOTALSECRETS	= 11,
STAT_TOTALMONSTERS	= 12,
STAT_SECRETS		= 13,		// bumped on client side by svc_foundsecret
STAT_MONSTERS		= 14,		// bumped by svc_killedmonster
STAT_ITEMS			= 15,
STAT_VIEWHEIGHT		= 16,	//same as zquake
STAT_TIME			= 17,	//zquake
#ifdef SIDEVIEWS
STAT_VIEW2			= 20,
#endif
STAT_VIEWZOOM		= 21, // DP

STAT_H2_LEVEL	= 32,				// changes stat bar
STAT_H2_INTELLIGENCE,				// changes stat bar
STAT_H2_WISDOM,						// changes stat bar
STAT_H2_STRENGTH,					// changes stat bar
STAT_H2_DEXTERITY,					// changes stat bar
STAT_H2_BLUEMANA,					// changes stat bar
STAT_H2_GREENMANA,					// changes stat bar
STAT_H2_EXPERIENCE,					// changes stat bar
STAT_H2_CNT_TORCH,					// changes stat bar
STAT_H2_CNT_H_BOOST,				// changes stat bar
STAT_H2_CNT_SH_BOOST,				// changes stat bar
STAT_H2_CNT_MANA_BOOST,				// changes stat bar
STAT_H2_CNT_TELEPORT,				// changes stat bar
STAT_H2_CNT_TOME,					// changes stat bar
STAT_H2_CNT_SUMMON,					// changes stat bar
STAT_H2_CNT_INVISIBILITY,			// changes stat bar
STAT_H2_CNT_GLYPH,					// changes stat bar
STAT_H2_CNT_HASTE,					// changes stat bar
STAT_H2_CNT_BLAST,					// changes stat bar
STAT_H2_CNT_POLYMORPH,				// changes stat bar
STAT_H2_CNT_FLIGHT,					// changes stat bar
STAT_H2_CNT_CUBEOFFORCE,			// changes stat bar
STAT_H2_CNT_INVINCIBILITY,			// changes stat bar
STAT_H2_ARTIFACT_ACTIVE,
STAT_H2_ARTIFACT_LOW,
STAT_H2_MOVETYPE,
STAT_H2_CAMERAMODE,
STAT_H2_HASTED,
STAT_H2_INVENTORY,
STAT_H2_RINGS_ACTIVE,

STAT_H2_RINGS_LOW,
STAT_H2_AMULET,
STAT_H2_BRACER,
STAT_H2_BREASTPLATE,
STAT_H2_HELMET,
STAT_H2_FLIGHT_T,
STAT_H2_WATER_T,
STAT_H2_TURNING_T,
STAT_H2_REGEN_T,
STAT_H2_PUZZLE1A,
STAT_H2_PUZZLE1B,
STAT_H2_PUZZLE1C,
STAT_H2_PUZZLE1D,
STAT_H2_PUZZLE2A,
STAT_H2_PUZZLE2B,
STAT_H2_PUZZLE2C,
STAT_H2_PUZZLE2D,
STAT_H2_PUZZLE3A,
STAT_H2_PUZZLE3B,
STAT_H2_PUZZLE3C,
STAT_H2_PUZZLE3D,
STAT_H2_PUZZLE4A,
STAT_H2_PUZZLE4B,
STAT_H2_PUZZLE4C,
STAT_H2_PUZZLE4D,
STAT_H2_PUZZLE5A,
STAT_H2_PUZZLE5B,
STAT_H2_PUZZLE5C,
STAT_H2_PUZZLE5D,
STAT_H2_PUZZLE6A,
STAT_H2_PUZZLE6B,
STAT_H2_PUZZLE6C,
STAT_H2_PUZZLE6D,
STAT_H2_PUZZLE7A,
STAT_H2_PUZZLE7B,
STAT_H2_PUZZLE7C,
STAT_H2_PUZZLE7D,
STAT_H2_PUZZLE8A,
STAT_H2_PUZZLE8B,
STAT_H2_PUZZLE8C,
STAT_H2_PUZZLE8D,
STAT_H2_MAXHEALTH,
STAT_H2_MAXMANA,
STAT_H2_FLAGS,

MAX_CL_STATS = 128
};

//
// item flags
//
#define	IT_SHOTGUN				1
#define	IT_SUPER_SHOTGUN		2
#define	IT_NAILGUN				4
#define	IT_SUPER_NAILGUN		8

#define	IT_GRENADE_LAUNCHER		16
#define	IT_ROCKET_LAUNCHER		32
#define	IT_LIGHTNING			64
#define	IT_SUPER_LIGHTNING		128

#define	IT_SHELLS				256
#define	IT_NAILS				512
#define	IT_ROCKETS				1024
#define	IT_CELLS				2048

#define	IT_AXE					4096

#define	IT_ARMOR1				8192
#define	IT_ARMOR2				16384
#define	IT_ARMOR3				32768

#define	IT_SUPERHEALTH			65536

#define	IT_KEY1					131072
#define	IT_KEY2					262144

#define	IT_INVISIBILITY			524288

#define	IT_INVULNERABILITY		1048576
#define	IT_SUIT					2097152
#define	IT_QUAD					4194304

#define	IT_SIGIL1				(1<<28)

#define	IT_SIGIL2				(1<<29)
#define	IT_SIGIL3				(1<<30)
#define	IT_SIGIL4				(1<<31)

//
// print flags
//
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages



//split screen stuff
#define MAX_SPLITS 4




//savegame vars
#define	SAVEGAME_COMMENT_LENGTH	39
#define	SAVEGAME_VERSION	667





#define dem_cmd			0
#define dem_read		1
#define dem_set			2
#define dem_multiple	3
#define	dem_single		4
#define dem_stats		5
#define dem_all			6


#endif	//ifndef __BOTHDEFS_H
