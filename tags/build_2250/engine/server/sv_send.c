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
// sv_main.c -- server main program

#include "qwsvdef.h"

#ifndef CLIENTONLY

#define CHAN_AUTO   0
#define CHAN_WEAPON 1
#define CHAN_VOICE  2
#define CHAN_ITEM   3
#define CHAN_BODY   4

/*
=============================================================================

Con_Printf redirection

=============================================================================
*/

char	outputbuf[8000];

redirect_t	sv_redirected;
int sv_redirectedlang;

extern func_t getplayerstat[MAX_CL_STATS];
extern func_t getplayerstati[MAX_CL_STATS];

extern cvar_t sv_phs;

/*
==================
SV_FlushRedirect
==================
*/
void SV_FlushRedirect (void)
{
	int totallen;
	char	send[8000+6];

	if (!*outputbuf)
		return;

	if (sv_redirected == RD_PACKET)
	{
		send[0] = 0xff;
		send[1] = 0xff;
		send[2] = 0xff;
		send[3] = 0xff;
		send[4] = A2C_PRINT;
		memcpy (send+5, outputbuf, strlen(outputbuf)+1);

		NET_SendPacket (NS_SERVER, strlen(send)+1, send, net_from);
	}
	else if (sv_redirected == RD_CLIENT)
	{
		int chop;
		char spare;
		char *s = outputbuf;
		totallen = strlen(s)+3;
		while (sizeof(host_client->backbuf_data[0])/2 < totallen)
		{
			chop = sizeof(host_client->backbuf_data[0]) / 2;
			spare = s[chop];
			s[chop] = '\0';

			ClientReliableWrite_Begin (host_client, host_client->isq2client?svcq2_print:svc_print, chop+3);
			ClientReliableWrite_Byte (host_client, PRINT_HIGH);
			ClientReliableWrite_String (host_client, s);

			s += chop;
			totallen -= chop;
			s[0] = spare;
		}
		ClientReliableWrite_Begin (host_client, host_client->isq2client?svcq2_print:svc_print, strlen(s)+3);
		ClientReliableWrite_Byte (host_client, PRINT_HIGH);
		ClientReliableWrite_String (host_client, s);
	}

	// clear it
	outputbuf[0] = 0;
}


/*
==================
SV_BeginRedirect

  Send Con_Printf data to the remote client
  instead of the console
==================
*/
void SV_BeginRedirect (redirect_t rd, int lang)
{
	sv_redirected = rd;
	sv_redirectedlang = lang;
	outputbuf[0] = 0;
}

void SV_EndRedirect (void)
{
	SV_FlushRedirect ();
	sv_redirectedlang = 0;	//clenliness rather than functionality. Shouldn't be needed.
	sv_redirected = RD_NONE;
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
#define	MAXPRINTMSG	4096
// FIXME: make a buffer size safe vsprintf?
#ifdef SERVERONLY
qboolean con_debuglog;
void VARGS Con_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	_vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);
	
	// add to redirected message
	if (sv_redirected)
	{
		if (strlen (msg) + strlen(outputbuf) > sizeof(outputbuf) - 1)
			SV_FlushRedirect ();
		strcat (outputbuf, msg);
		if (sv_redirected != -1)
			return;
	}

	Sys_Printf ("%s", msg);	// also echo to debugging console

	if (con_debuglog)
		Sys_DebugLog(va("%s/qconsole.log",com_gamedir), "%s", msg);
}
void Con_TPrintf (translation_t stringnum, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	char *fmt;

	// add to redirected message
	if (sv_redirected)
	{
		fmt = languagetext[stringnum][sv_redirectedlang];
		va_start (argptr,stringnum);
		_vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
		va_end (argptr);
	
		if (strlen (msg) + strlen(outputbuf) > sizeof(outputbuf) - 1)
			SV_FlushRedirect ();
		strcat (outputbuf, msg);
		return;
	}

	fmt = languagetext[stringnum][svs.language];

	va_start (argptr,stringnum);
	_vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	Sys_Printf ("%s", msg);	// also echo to debugging console

	if (con_debuglog)
		Sys_DebugLog(va("%s/qconsole.log",com_gamedir), "%s", msg);
}
/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	if (!developer.value)
		return;

	va_start (argptr,fmt);
	_vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);
	
	Con_Printf ("%s", msg);
}
#endif

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

static void SV_PrintToClient(client_t *cl, int level, char *string)
{
	ClientReliableWrite_Begin (cl, cl->isq2client?svcq2_print:svc_print, strlen(string)+3);
#ifdef NQPROT
	if (cl->nqprot)
	{
		if (level == PRINT_CHAT)
			ClientReliableWrite_Byte (cl, 1);
	}
	else
#endif
		ClientReliableWrite_Byte (cl, level);
	ClientReliableWrite_String (cl, string);
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed if the level passes
=================
*/
void VARGS SV_ClientPrintf (client_t *cl, int level, char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	
	if (level < cl->messagelevel)
		return;
	
	va_start (argptr,fmt);
	_vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	if(strlen(string) >= sizeof(string))
		Sys_Error("SV_ClientPrintf: Buffer stomped\n");

	if (sv.mvdrecording)
	{
		MVDWrite_Begin (dem_single, cl - svs.clients, strlen(string)+3);
		MSG_WriteByte ((sizebuf_t *)demo.dbuf, svc_print);
		MSG_WriteByte ((sizebuf_t *)demo.dbuf, level);
		MSG_WriteString ((sizebuf_t *)demo.dbuf, string);
	}

	if (cl->controller)
		SV_PrintToClient(cl->controller, level, string);
	else
		SV_PrintToClient(cl, level, string);
}

void VARGS SV_ClientTPrintf (client_t *cl, int level, translation_t stringnum, ...)
{
	va_list		argptr;
	char		string[1024];
	char *fmt = languagetext[stringnum][cl->language];
	
	if (level < cl->messagelevel)
		return;
	
	va_start (argptr,stringnum);
	_vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	if(strlen(string) >= sizeof(string))
		Sys_Error("SV_ClientTPrintf: Buffer stomped\n");

	if (sv.mvdrecording)
	{
		MVDWrite_Begin (dem_single, cl - svs.clients, strlen(string)+3);
		MSG_WriteByte ((sizebuf_t*)demo.dbuf, svc_print);
		MSG_WriteByte ((sizebuf_t*)demo.dbuf, level);
		MSG_WriteString ((sizebuf_t*)demo.dbuf, string);
	}

	SV_PrintToClient(cl, level, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void VARGS SV_BroadcastPrintf (int level, char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	client_t	*cl;
	int			i;

	va_start (argptr,fmt);
	_vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	if(strlen(string) >= sizeof(string))
		Sys_Error("SV_BroadcastPrintf: Buffer stomped\n");

	Sys_Printf ("%s", string);	// print to the console

	for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
	{
		if (level < cl->messagelevel)
			continue;
		if (!cl->state)
			continue;

		if (cl->controller)
			continue;

		SV_PrintToClient(cl, level, string);
	}

	if (sv.mvdrecording)
	{
		MVDWrite_Begin (dem_all, 0, strlen(string)+3);
		MSG_WriteByte ((sizebuf_t*)demo.dbuf, svc_print);
		MSG_WriteByte ((sizebuf_t*)demo.dbuf, level);
		MSG_WriteString ((sizebuf_t*)demo.dbuf, string);
	}
}


void VARGS SV_BroadcastTPrintf (int level, translation_t stringnum, ...)
{
	va_list		argptr;
	char		string[1024];
	client_t	*cl;
	int			i;
	int oldlang=-1;
	char *fmt = languagetext[stringnum][oldlang=svs.language];

	va_start (argptr,stringnum);
	_vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	if(strlen(string) >= sizeof(string))
		Sys_Error("SV_BroadcastPrintf: Buffer stomped\n");
	
	Sys_Printf ("%s", string);	// print to the console

	for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
	{
		if (level < cl->messagelevel)
			continue;
		if (!cl->state)
			continue;
		if (cl->controller)
			continue;

		if (oldlang!=cl->language)
		{
			fmt = languagetext[stringnum][oldlang=cl->language];

			va_start (argptr,stringnum);
			_vsnprintf (string,sizeof(string)-1, fmt,argptr);
			va_end (argptr);

			if(strlen(string) >= sizeof(string))
				Sys_Error("SV_BroadcastPrintf: Buffer stomped\n");
		}

		SV_PrintToClient(cl, level, string);
	}
}


/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void VARGS SV_BroadcastCommand (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int i;
	client_t *cl;
	
	if (!sv.state)
		return;
	va_start (argptr,fmt);
	_vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
	{
		if (cl->controller)
			continue;
		if (cl->state>=cs_connected)
		{
			ClientReliableWrite_Begin(cl, cl->isq2client?svcq2_stufftext:svc_stufftext, strlen(string)+2);
			ClientReliableWrite_String (cl, string);
		}
	}
}


/*
=================
SV_Multicast

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.

MULTICAST_ALL	same as broadcast
MULTICAST_PVS	send to clients potentially visible from org
MULTICAST_PHS	send to clients potentially hearable from org
=================
*/
void SV_MulticastProtExt(vec3_t origin, multicast_t to, int dimension_mask, int with, int without)
{
	client_t	*client;
	qbyte		*mask;
	mleaf_t		*leaf;
	int			leafnum;
	int			j;
	qboolean	reliable;

//	to = MULTICAST_ALL;
#ifdef Q2BSPS
	if (sv.worldmodel->fromgame == fg_quake2 || sv.worldmodel->fromgame == fg_quake3)
	{
		int			area1, area2, cluster;

		reliable = false;

		if (to != MULTICAST_ALL_R && to != MULTICAST_ALL)
		{
			leafnum = CM_PointLeafnum (origin);
			area1 = CM_LeafArea (leafnum);
		}
		else
		{
			leafnum = 0;	// just to avoid compiler warnings
			area1 = 0;
		}

		switch (to)
		{
		case MULTICAST_ALL_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_ALL:
			leafnum = 0;
			mask = NULL;
			break;

		case MULTICAST_PHS_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_PHS:
			leafnum = CM_PointLeafnum (origin);
			cluster = CM_LeafCluster (leafnum);
			mask = CM_ClusterPHS (cluster);
			break;

		case MULTICAST_PVS_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_PVS:
			leafnum = CM_PointLeafnum (origin);
			cluster = CM_LeafCluster (leafnum);
			mask = CM_ClusterPVS (cluster, NULL);
			break;

		default:
			mask = NULL;
			SV_Error ("SV_Multicast: bad to:%i", to);
		}

		// send the data to all relevent clients
		for (j = 0, client = svs.clients; j < MAX_CLIENTS; j++, client++)
		{
			if (client->state != cs_spawned)
				continue;

			if (client->fteprotocolextensions & without)
			{
	//			Con_Printf ("Version supressed multicast - without pext\n");
				continue;
			}
			if (!(~client->fteprotocolextensions & ~with))
			{
	//			Con_Printf ("Version supressed multicast - with pext\n");
				continue;
			}

			if (mask)
			{
#ifdef Q2SERVER
				if (ge)
					leafnum = CM_PointLeafnum (client->q2edict->s.origin);
				else
#endif
					leafnum = CM_PointLeafnum (client->edict->v.origin);
				cluster = CM_LeafCluster (leafnum);
				area2 = CM_LeafArea (leafnum);
				if (!CM_AreasConnected (area1, area2))
					continue;
				if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
					continue;
			}

#ifdef NQPROT
			if (client->nqprot)
			{
				if (reliable)
				{
					ClientReliableCheckBlock(client, sv.nqmulticast.cursize);
					ClientReliableWrite_SZ(client, sv.nqmulticast.data, sv.nqmulticast.cursize);
				}
				else
					SZ_Write (&client->datagram, sv.nqmulticast.data, sv.nqmulticast.cursize);
			}
			else
#endif
			     if (reliable)
			{
				ClientReliableCheckBlock(client, sv.multicast.cursize);
				ClientReliableWrite_SZ(client, sv.multicast.data, sv.multicast.cursize);
			}
			else
				SZ_Write (&client->datagram, sv.multicast.data, sv.multicast.cursize);
		}
	}
	else
#endif
	{
		leaf = Mod_PointInLeaf (origin, sv.worldmodel);
		if (!leaf)
			leafnum = 0;
		else
			leafnum = leaf - sv.worldmodel->leafs;

		reliable = false;

		switch (to)
		{
		case MULTICAST_ALL_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_ALL:
			mask = sv.pvs;		// leaf 0 is everything;
			break;

		case MULTICAST_PHS_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_PHS:
			mask = sv.phs + leafnum * 4*((sv.worldmodel->numleafs+31)>>5);
			break;

		case MULTICAST_PVS_R:
			reliable = true;	// intentional fallthrough
		case MULTICAST_PVS:
			mask = sv.pvs + leafnum * 4*((sv.worldmodel->numleafs+31)>>5);
			break;

		default:
			mask = NULL;
			SV_Error ("SV_Multicast: bad to:%i", to);
		}

		// send the data to all relevent clients
		for (j = 0, client = svs.clients; j < MAX_CLIENTS; j++, client++)
		{
			if (client->state != cs_spawned)
				continue;

			if (client->controller)
				continue;	//FIXME: send if at least one of the players is near enough.

			if (client->fteprotocolextensions & without)
			{
	//			Con_Printf ("Version supressed multicast - without pext\n");
				continue;
			}
			if (!(client->fteprotocolextensions & with) && with)
			{
	//			Con_Printf ("Version supressed multicast - with pext\n");
				continue;
			}

			if (svprogfuncs)
				if (!((int)client->edict->v.dimension_see & dimension_mask))
					continue;

			if (to == MULTICAST_PHS_R || to == MULTICAST_PHS) {
				vec3_t delta;
				VectorSubtract(origin, client->edict->v.origin, delta);
				if (Length(delta) <= 1024)
					goto inrange;
			}

			leaf = Mod_PointInLeaf (client->edict->v.origin, sv.worldmodel);
			if (leaf)
			{
				// -1 is because pvs rows are 1 based, not 0 based like leafs
				leafnum = leaf - sv.worldmodel->leafs - 1;
				if ( !(mask[leafnum>>3] & (1<<(leafnum&7)) ) )
				{
	//				Con_Printf ("supressed multicast\n");
					continue;
				}
			}

	inrange:
	#ifdef NQPROT
			if (client->nqprot)
			{
				if (reliable) {
					ClientReliableCheckBlock(client, sv.nqmulticast.cursize);
					ClientReliableWrite_SZ(client, sv.nqmulticast.data, sv.nqmulticast.cursize);
				} else
					SZ_Write (&client->datagram, sv.nqmulticast.data, sv.nqmulticast.cursize);
			}
			else
	#endif
			{
				if (reliable) {
					ClientReliableCheckBlock(client, sv.multicast.cursize);
					ClientReliableWrite_SZ(client, sv.multicast.data, sv.multicast.cursize);
				} else
					SZ_Write (&client->datagram, sv.multicast.data, sv.multicast.cursize);
			}
		}
	}

	if (sv.mvdrecording && !with)	//mvds don't get the pext stuff
	{
		if (reliable)
		{
			MVDWrite_Begin(dem_all, 0, sv.multicast.cursize);
			SZ_Write((sizebuf_t*)demo.dbuf, sv.multicast.data, sv.multicast.cursize);
		} else 
			SZ_Write(&demo.datagram, sv.multicast.data, sv.multicast.cursize);
	}

#ifdef NQPROT
	SZ_Clear (&sv.nqmulticast);
#endif
	SZ_Clear (&sv.multicast);
}

//version does all the work now
void SV_Multicast (vec3_t origin, multicast_t to)
{
	SV_MulticastProtExt(origin, to, FULLDIMENSIONMASK, 0, 0);
}

/*  
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/  
void SV_StartSound (edict_t *entity, int channel, char *sample, int volume,
    float attenuation)
{   
#define	NQSND_VOLUME		(1<<0)		// a qbyte
#define	NQSND_ATTENUATION	(1<<1)		// a qbyte

    int         sound_num;
#ifdef NQPROT
    int			field_mask;
	int nqchan;
#endif
    int			i;
	int			ent;
	vec3_t		origin;
	qboolean	use_phs;
	qboolean	reliable = false;

	if (volume < 0 || volume > 255)
	{
		Con_Printf ("SV_StartSound: volume = %i", volume);
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		Con_Printf ("SV_StartSound: attenuation = %f", attenuation);
		return;
	}

	if (channel < 0 || channel > 15)
	{
		Con_Printf ("SV_StartSound: channel = %i", channel);
		return;
	}

// find precache number for sound
    for (sound_num=1 ; sound_num<MAX_SOUNDS
        && sv.sound_precache[sound_num] ; sound_num++)
        if (!strcmp(sample, sv.sound_precache[sound_num]))
            break;
    
    if ( sound_num == MAX_SOUNDS || !sv.sound_precache[sound_num] )
    {
        Con_DPrintf ("SV_StartSound: %s not precacheed\n", sample);
        return;
    }
    
	ent = NUM_FOR_EDICT(svprogfuncs, entity);

	if ((channel & 8) || !sv_phs.value)	// no PHS flag
	{
		if (channel & 8)
			reliable = true; // sounds that break the phs are reliable
		use_phs = false;
		channel &= 7;
	}
	else
		use_phs = true;

//	if (channel == CHAN_BODY || channel == CHAN_VOICE)
//		reliable = true;

	channel = (ent<<3) | channel;
	
#ifdef NQPROT
	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= NQSND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= NQSND_ATTENUATION;

	nqchan = channel;
#endif
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		channel |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		channel |= SND_ATTENUATION;

	// use the entity origin unless it is a bmodel
	if (entity->v.solid == SOLID_BSP)
	{
		for (i=0 ; i<3 ; i++)
			origin[i] = entity->v.origin[i]+0.5*(entity->v.mins[i]+entity->v.maxs[i]);
	}
	else
	{
		VectorCopy (entity->v.origin, origin);
	}

	MSG_WriteByte (&sv.multicast, svc_sound);
	MSG_WriteShort (&sv.multicast, channel);
	if (channel & SND_VOLUME)
		MSG_WriteByte (&sv.multicast, volume);
	if (channel & SND_ATTENUATION)
		MSG_WriteByte (&sv.multicast, attenuation*64);
	MSG_WriteByte (&sv.multicast, sound_num);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord (&sv.multicast, origin[i]);

#ifdef NQPROT
	MSG_WriteByte (&sv.nqmulticast, svc_sound);
	MSG_WriteByte (&sv.nqmulticast, field_mask);
	if (field_mask & NQSND_VOLUME)
		MSG_WriteByte (&sv.nqmulticast, volume);
	if (field_mask & NQSND_ATTENUATION)
		MSG_WriteByte (&sv.nqmulticast, attenuation*64);
	MSG_WriteShort (&sv.nqmulticast, nqchan);
	MSG_WriteByte (&sv.nqmulticast, sound_num);
	for (i=0 ; i<3 ; i++)
		MSG_WriteCoord (&sv.nqmulticast, origin[i]);
#endif
	if (use_phs)
		SV_MulticastProtExt(origin, reliable ? MULTICAST_PHS_R : MULTICAST_PHS, entity->v.dimension_seen, 0, 0);
	else
		SV_MulticastProtExt(origin, reliable ? MULTICAST_ALL_R : MULTICAST_ALL, entity->v.dimension_seen, 0, 0);
}

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

int		sv_nailmodel, sv_supernailmodel, sv_playermodel;
#ifdef PEXT_LIGHTUPDATES
int		sv_lightningmodel;
#endif

void SV_FindModelNumbers (void)
{
	int		i;

	sv_nailmodel = -1;
	sv_supernailmodel = -1;
	sv_playermodel = -1;
#ifdef PEXT_LIGHTUPDATES
	sv_lightningmodel = -1;
#endif
	
	for (i=0 ; i<MAX_MODELS ; i++)
	{
		if (!*sv.model_precache[i])
			break;
		if (!strcmp(sv.model_precache[i],"progs/spike.mdl"))
			sv_nailmodel = i;
		if (!strcmp(sv.model_precache[i],"progs/s_spike.mdl"))
			sv_supernailmodel = i;
#ifdef PEXT_LIGHTUPDATES
		if (!strcmp(sv.model_precache[i],"progs/zap.mdl"))
			sv_lightningmodel = i;
#endif
		if (!strcmp(sv.model_precache[i],"progs/player.mdl"))
			sv_playermodel = i;
	}
}

void SV_WriteEntityDataToMessage (client_t *client, sizebuf_t *msg, int pnum)
{
	edict_t	*other;
	edict_t	*ent;
	int i;

	ent = client->edict;

	// send a damage message if the player got hit this frame
	if (ent->v.dmg_take || ent->v.dmg_save)
	{
		other = PROG_TO_EDICT(svprogfuncs, ent->v.dmg_inflictor);
		if (pnum)
		{
			MSG_WriteByte(msg, svc_choosesplitclient);
			MSG_WriteByte(msg, pnum);
		}
		MSG_WriteByte (msg, svc_damage);
		MSG_WriteByte (msg, ent->v.dmg_save);
		MSG_WriteByte (msg, ent->v.dmg_take);
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (msg, other->v.origin[i] + 0.5*(other->v.mins[i] + other->v.maxs[i]));
	
		ent->v.dmg_take = 0;
		ent->v.dmg_save = 0;
	}

	// a fixangle might get lost in a dropped packet.  Oh well.
	if ( ent->v.fixangle )
	{
		if (pnum)
		{
			MSG_WriteByte(msg, svc_choosesplitclient);
			MSG_WriteByte(msg, pnum);
		}
		MSG_WriteByte (msg, svc_setangle);
		for (i=0 ; i < 3 ; i++)
			MSG_WriteAngle (msg, ent->v.angles[i] );
		ent->v.fixangle = 0;
	}
}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage (client_t *client, sizebuf_t *msg)
{
#ifdef NQPROT
	int		i;
	int bits, items;
	edict_t	*ent;
#endif
	client_t *split;
	int pnum=0;



	// send the chokecount for r_netgraph
	if (client->chokecount)
	{
		MSG_WriteByte (msg, svc_chokecount);
		MSG_WriteByte (msg, client->chokecount);
		client->chokecount = 0;
	}

	for (split = client; split; split=split->controlled, pnum++)
		SV_WriteEntityDataToMessage(split, msg, pnum);

	// Z_EXT_TIME protocol extension
	// every now and then, send an update so that extrapolation
	// on client side doesn't stray too far off
#ifdef NQPROT
	if (!client->nqprot)
#endif
	if (client->zquake_extensions & Z_EXT_SERVERTIME && sv.time - client->nextservertimeupdate > 0)
	{
		MSG_WriteByte (msg, svc_updatestatlong);
		MSG_WriteByte (msg, STAT_TIME);
		MSG_WriteLong (msg, (int)(sv.time * 1000));

		client->nextservertimeupdate = sv.time+10;
	}


#ifdef NQPROT
	if (!client->nqprot)
		return;

	ent = client->edict;


	MSG_WriteByte (msg, svc_time);
	MSG_WriteFloat(msg, sv.time);
	client->nextservertimeupdate = sv.time+10;


	bits = 0;
#define	SU_VIEWHEIGHT	(1<<0)
#define	SU_IDEALPITCH	(1<<1)
#define	SU_PUNCH1		(1<<2)
#define	SU_PUNCH2		(1<<3)
#define	SU_PUNCH3		(1<<4)
#define	SU_VELOCITY1	(1<<5)
#define	SU_VELOCITY2	(1<<6)
#define	SU_VELOCITY3	(1<<7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define	SU_ITEMS		(1<<9)
#define	SU_ONGROUND		(1<<10)		// no data follows, the bit is it
#define	SU_INWATER		(1<<11)		// no data follows, the bit is it
#define	SU_WEAPONFRAME	(1<<12)
#define	SU_ARMOR		(1<<13)
#define	SU_WEAPON		(1<<14)

	if (ent->v.view_ofs[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;
		
//	if (ent->v.idealpitch)
//		bits |= SU_IDEALPITCH;

// stuff the sigil bits into the high bits of items for sbar, or else
// mix in items2
//	val = GetEdictFieldValue(ent, "items2", &items2cache);

//	if (val)
//		items = (int)ent->v.items | ((int)val->_float << 23);
//	else
		items = (int)ent->v.items | ((int)pr_global_struct->serverflags << 28);
	

	bits |= SU_ITEMS;
	
	if ( (int)ent->v.flags & FL_ONGROUND)
		bits |= SU_ONGROUND;
	
	if ( ent->v.waterlevel >= 2)
		bits |= SU_INWATER;
	
	for (i=0 ; i<3 ; i++)
	{
//		if (ent->v.punchangle[i])
//			bits |= (SU_PUNCH1<<i);
		if (ent->v.velocity[i])
			bits |= (SU_VELOCITY1<<i);
	}
	
	if (ent->v.weaponframe)
		bits |= SU_WEAPONFRAME;

	if (ent->v.armorvalue)
		bits |= SU_ARMOR;

//	if (ent->v.weapon)
		bits |= SU_WEAPON;

// send the data

	MSG_WriteByte (msg, svc_clientdata);
	MSG_WriteShort (msg, bits);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar (msg, ent->v.view_ofs[2]);

//	if (bits & SU_IDEALPITCH)
//		MSG_WriteChar (msg, ent->v.idealpitch);

	for (i=0 ; i<3 ; i++)
	{
//		if (bits & (SU_PUNCH1<<i))
//			MSG_WriteChar (msg, ent->v.punchangle[i]);
		if (bits & (SU_VELOCITY1<<i))
			MSG_WriteChar (msg, ent->v.velocity[i]/16);
	}

// [always sent]	if (bits & SU_ITEMS)
	MSG_WriteLong (msg, items);

	if (bits & SU_WEAPONFRAME)
		MSG_WriteByte (msg, ent->v.weaponframe);
	if (bits & SU_ARMOR)
	{
		if (ent->v.armorvalue>255)
			MSG_WriteByte (msg, 255);
		else
			MSG_WriteByte (msg, ent->v.armorvalue);
	}
	if (bits & SU_WEAPON)
		MSG_WriteByte (msg, SV_ModelIndex(ent->v.weaponmodel + svprogfuncs->stringtable));
	
	MSG_WriteShort (msg, ent->v.health);
	MSG_WriteByte (msg, ent->v.currentammo);
	MSG_WriteByte (msg, ent->v.ammo_shells);
	MSG_WriteByte (msg, ent->v.ammo_nails);
	MSG_WriteByte (msg, ent->v.ammo_rockets);
	MSG_WriteByte (msg, ent->v.ammo_cells);
	
	//if (other && other->v.weapon)		
		//MSG_WriteByte (msg, other->v.weapon);
	//else
	//{

	if (standard_quake)
	{
		MSG_WriteByte (msg, ent->v.weapon);
	}
	else
	{
		for(i=0;i<32;i++)
		{
			if ( ((int)ent->v.weapon) & (1<<i) )
			{
				MSG_WriteByte (msg, i);
				break;
			}
		}
	}
//	}
#endif
}

typedef struct {
	int type;
	char name[64];
	evalc_t evalc;
	int statnum;
} qcstat_t;
qcstat_t qcstats[MAX_CL_STATS-32];
int numqcstats;
void SV_QCStat(int type, char *name, int statnum)
{
	if (numqcstats == sizeof(qcstats)/sizeof(qcstats[0]))
	{
		Con_Printf("Too many stat types\n");
		return;
	}

	qcstats[numqcstats].type = type;
	qcstats[numqcstats].statnum = statnum;
	Q_strncpyz(qcstats[numqcstats].name, name, sizeof(qcstats[numqcstats].name));
	memset(&qcstats[numqcstats].evalc, 0, sizeof(evalc_t));
	numqcstats++;
}

void SV_ClearQCStats(void)
{
	numqcstats = 0;
}

void SV_UpdateQCStats(edict_t	*ent, int *stats)
{
	char *s;
	int i;

	for (i = 0; i < numqcstats; i++)
	{
		eval_t *eval;
		eval = svprogfuncs->GetEdictFieldValue(svprogfuncs, ent, qcstats[i].name, &qcstats[i].evalc);

		switch(qcstats[i].type)
		{
		case ev_float:
			stats[qcstats[i].statnum] = eval->_float;
			break;
		case ev_integer:
			stats[qcstats[i].statnum] = eval->_int;
			break;
		case ev_entity:
			stats[qcstats[i].statnum] = NUM_FOR_EDICT(svprogfuncs, PROG_TO_EDICT(svprogfuncs, eval->edict));
			break;
		case ev_string:
			s = PR_GetString(svprogfuncs, eval->string);
			stats[qcstats[i].statnum+0] = LittleLong(((int*)s)[0]);	//so the network is sent out correctly as a string.
			stats[qcstats[i].statnum+1] = LittleLong(((int*)s)[1]);
			stats[qcstats[i].statnum+2] = LittleLong(((int*)s)[2]);
			stats[qcstats[i].statnum+3] = LittleLong(((int*)s)[3]);
			break;
		}
	}
}

/*
=======================
SV_UpdateClientStats

Performs a delta update of the stats array.  This should only be performed
when a reliable message can be delivered this frame.
=======================
*/
void SV_UpdateClientStats (client_t *client, int pnum)
{
	edict_t	*ent;
	int		stats[MAX_CL_STATS];
	int		i, m;
	globalvars_t *pr_globals;
	extern qboolean pr_items2;

	ent = client->edict;
	memset (stats, 0, sizeof(stats));
	
	// if we are a spectator and we are tracking a player, we get his stats
	// so our status bar reflects his
	if (client->spectator && client->spec_track > 0)
		ent = svs.clients[client->spec_track - 1].edict;

	stats[STAT_HEALTH] = ent->v.health;
	stats[STAT_WEAPON] = SV_ModelIndex(PR_GetString(svprogfuncs, ent->v.weaponmodel));
	stats[STAT_AMMO] = ent->v.currentammo;
	stats[STAT_ARMOR] = ent->v.armorvalue;
	stats[STAT_SHELLS] = ent->v.ammo_shells;
	stats[STAT_NAILS] = ent->v.ammo_nails;
	stats[STAT_ROCKETS] = ent->v.ammo_rockets;
	stats[STAT_CELLS] = ent->v.ammo_cells;
	if (!client->spectator)
		stats[STAT_ACTIVEWEAPON] = ent->v.weapon;

	// stuff the sigil bits into the high bits of items for sbar
	if (pr_items2)
		stats[STAT_ITEMS] = (int)ent->v.items | ((int)ent->v.items2 << 23);
	else
		stats[STAT_ITEMS] = (int)ent->v.items | ((int)pr_global_struct->serverflags << 28);

	stats[STAT_VIEWHEIGHT] = ent->v.view_ofs[2];
#ifdef PEXT_VIEW2	
	if (ent->v.view2)
		stats[STAT_VIEW2] = NUM_FOR_EDICT(svprogfuncs, PROG_TO_EDICT(svprogfuncs, ent->v.view2));
	else
		stats[STAT_VIEW2] = 0;
#endif

	SV_UpdateQCStats(ent, stats);

	//dmw tweek for stats
	pr_globals = PR_globals(svprogfuncs, PR_CURRENT);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, ent);

	m = MAX_QW_STATS;
	if (client->fteprotocolextensions & PEXT_HEXEN2)
		m = MAX_CL_STATS;

	for (i=0 ; i<m ; i++)
	{
		//dmw tweek for stats
		if (getplayerstati[i])
		{
			G_INT(OFS_PARM0) = stats[i];
			PR_ExecuteProgram(svprogfuncs, getplayerstati[i]);
			stats[i] = G_INT(OFS_RETURN);
		}
		else if (getplayerstat[i])
		{
			G_FLOAT(OFS_PARM0) = stats[i];
			PR_ExecuteProgram(svprogfuncs, getplayerstat[i]);
			stats[i] = G_FLOAT(OFS_RETURN);
		}
		if (sv.demofile)
		{
			if (!client->spec_track)
			{
				stats[i] = 0;
				if (i == STAT_HEALTH)
					stats[i] = 100;
			}
			else
				stats[i] = sv.recordedplayer[client->spec_track - 1].stats[i];
		}
		if (stats[i] != client->stats[i])
		{
			client->stats[i] = stats[i];
#ifdef NQPROT
			if (client->nqprot)
			{
				ClientReliableWrite_Begin(client, svc_updatestat, 3);
				ClientReliableWrite_Byte(client, i);
				ClientReliableWrite_Long(client, stats[i]);
			} else
#endif
			
			if (pnum)
			{
				if (stats[i] >=0 && stats[i] <= 255)
				{
					ClientReliableWrite_Begin(client->controller, svc_choosesplitclient, 5);
					ClientReliableWrite_Byte(client->controller, pnum);
					ClientReliableWrite_Byte(client->controller, svc_updatestat);
					ClientReliableWrite_Byte(client->controller, i);
					ClientReliableWrite_Byte(client->controller, stats[i]);
				}
				else
				{
					ClientReliableWrite_Begin(client->controller, svc_choosesplitclient, 8);
					ClientReliableWrite_Byte(client->controller, pnum);
					ClientReliableWrite_Byte(client->controller, svc_updatestatlong);
					ClientReliableWrite_Byte(client->controller, i);
					ClientReliableWrite_Long(client->controller, stats[i]);
				}
			}
			else if (stats[i] >=0 && stats[i] <= 255)
			{
				ClientReliableWrite_Begin(client, svc_updatestat, 3);
				ClientReliableWrite_Byte(client, i);
				ClientReliableWrite_Byte(client, stats[i]);
			}
			else
			{
				ClientReliableWrite_Begin(client, svc_updatestatlong, 6);
				ClientReliableWrite_Byte(client, i);
				ClientReliableWrite_Long(client, stats[i]);
			}
		}
	}
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	qbyte		buf[MAX_DATAGRAM];
	sizebuf_t	msg;

	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;
	msg.allowoverflow = true;
	msg.overflowed = false;

	if (sv.worldmodel && !client->controller)
	{
		if (client->isq2client)
		{
			SV_BuildClientFrame (client);

			// send over all the relevant entity_state_t
			// and the player_state_t
			SV_WriteFrameToClient (client, &msg);
		}
		else
		{
			// add the client specific data to the datagram
			SV_WriteClientdataToMessage (client, &msg);

			// send over all the objects that are in the PVS
			// this will include clients, a packetentities, and
			// possibly a nails update
			SV_WriteEntitiesToClient (client, &msg, false);
		}
	}

	// copy the accumulated multicast datagram
	// for this client out to the message
	if (client->datagram.overflowed)
		Con_Printf ("WARNING: datagram overflowed for %s\n", client->name);
	else
		SZ_Write (&msg, client->datagram.data, client->datagram.cursize);
	SZ_Clear (&client->datagram);

	// send deltas over reliable stream
	if (sv.worldmodel)
		if (!client->isq2client && Netchan_CanReliable (&client->netchan, SV_RateForClient(client)))
		{
			int pnum=1;
			client_t *c;
			SV_UpdateClientStats (client, 0);

			for (c = client->controlled; c; c = c->controlled,pnum++)
				SV_UpdateClientStats(c, pnum);
		}

	if (msg.overflowed)
	{
		Con_Printf ("WARNING: msg overflowed for %s\n", client->name);
		SZ_Clear (&msg);
	}

	// send the datagram
	Netchan_Transmit (&client->netchan, msg.cursize, buf, SV_RateForClient(client));

	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages (void)
{
	float newval;
	int			i, j;
	client_t *client;
	edict_t *ent;

// check for changes to be sent over the reliable streams to all clients
	for (i=0, host_client = svs.clients ; i<MAX_CLIENTS ; i++, host_client++)
	{
		if (host_client->state != cs_spawned)
		{
			if (!host_client->state && host_client->name[0])
			{
				if (host_client->old_frags != (int)host_client->edict->v.frags)
				{
					for (j=0, client = svs.clients ; j<MAX_CLIENTS ; j++, client++)
					{
						if (client->state < cs_connected)
							continue;
						ClientReliableWrite_Begin(client, svc_updatefrags, 4);
						ClientReliableWrite_Byte(client, i);
						ClientReliableWrite_Short(client, host_client->edict->v.frags);
					}

					host_client->old_frags = host_client->edict->v.frags;
				}
			}
			continue;
		}
		if (!host_client->isq2client)
		{
			if (host_client->sendinfo)
			{
				host_client->sendinfo = false;
				SV_FullClientUpdate (host_client, &sv.reliable_datagram);
			}
			if (host_client->old_frags != (int)host_client->edict->v.frags)
			{
				for (j=0, client = svs.clients ; j<MAX_CLIENTS ; j++, client++)
				{
					if (client->state < cs_connected)
						continue;
					if (client->controller)
						continue;
					ClientReliableWrite_Begin(client, svc_updatefrags, 4);
					ClientReliableWrite_Byte(client, i);
					ClientReliableWrite_Short(client, host_client->edict->v.frags);
				}

				host_client->old_frags = host_client->edict->v.frags;
			}
		}

		if (svprogfuncs)
		{
			extern cvar_t sv_gravity;
			// maxspeed/entgravity changes
			ent = host_client->edict;
			
			newval = ent->v.gravity*sv_gravity.value;
			if (progstype == PROG_NQ)
			{
				if (!newval)
					newval = 1;
			}

		/*	if (host_client->viewent)
			{
#define VIEWENT_GRAVITY_MAGIC 0.83217	//we use so the gravity is properly reset when viewent is cleared
				if (host_client->entgravity != newval+VIEWENT_GRAVITY_MAGIC)
				{
					ClientReliableWrite_Begin(host_client, svc_entgravity, 5);
					ClientReliableWrite_Float(host_client, 0);
					host_client->entgravity = newval+VIEWENT_GRAVITY_MAGIC;
				}
#undef VIEWENT_GRAVITY_MAGIC
			}
			else */if (host_client->entgravity != newval)
			{
				ClientReliableWrite_Begin(host_client, svc_entgravity, 5);
				ClientReliableWrite_Float(host_client, newval/movevars.gravity);	//lie to the client in a cunning way
				host_client->entgravity = newval;
			}
			newval = ent->v.maxspeed;
			if (progstype == PROG_NQ)
			{
				if (!newval)
					newval = sv_maxspeed.value;
			}
			if (ent->v.hasted)
				newval*=ent->v.hasted;
	#ifdef SVCHAT	//enforce a no moving time when chatting. Prevent client prediction going mad.
			if (host_client->chat.active)
				newval = 0;
	#endif
			if (host_client->maxspeed != newval)
			{	//MSVC can really suck at times (optimiser bug)

				if (host_client->controller)
				{	//this is a slave client.
					//find the right number and send.
					int pnum = 0;
					client_t *sp;
					for (sp = host_client->controller; sp; sp = sp->controlled)
					{
						if (sp == host_client)
							break;
						pnum++;
					}
					sp = host_client->controller;

					ClientReliableWrite_Begin (sp, svc_choosesplitclient, 7);
					ClientReliableWrite_Byte (sp, pnum);
					ClientReliableWrite_Byte (sp, svc_maxspeed);
					ClientReliableWrite_Float(sp, newval);
				}
				else
				{
					ClientReliableWrite_Begin(host_client, svc_maxspeed, 5);
					ClientReliableWrite_Float(host_client, newval);
				}
				host_client->maxspeed = newval;
			}
		}
	}

	if (sv.reliable_datagram.overflowed)
	{
		Con_Printf("WARNING: Reliable datagram overflowed\n");
		SZ_Clear (&sv.reliable_datagram);
	}

	if (sv.datagram.overflowed)
		SZ_Clear (&sv.datagram);

#ifdef NQPROT
	if (sv.nqdatagram.overflowed)
		SZ_Clear (&sv.nqdatagram);
#endif
	if (sv.q2datagram.overflowed)
		SZ_Clear (&sv.q2datagram);

	// append the broadcast messages to each client messages
	for (j=0, client = svs.clients ; j<MAX_CLIENTS ; j++, client++)
	{
		if (client->state < cs_connected)
			continue;	// reliables go to all connected or spawned
		if (client->controller)
			continue;	//splitscreen
#ifdef NQPROT
		if (client->nqprot)
		{
			ClientReliableCheckBlock(client, sv.nqreliable_datagram.cursize);
			ClientReliableWrite_SZ(client, sv.nqreliable_datagram.data, sv.nqreliable_datagram.cursize);

			if (client->state != cs_spawned)
				continue;	// datagrams only go to spawned
			SZ_Write (&client->datagram
				, sv.nqdatagram.data
				, sv.nqdatagram.cursize);
		}
		else
#endif
			 if (client->isq2client)
		{
			ClientReliableCheckBlock(client, sv.q2reliable_datagram.cursize);
			ClientReliableWrite_SZ(client, sv.q2reliable_datagram.data, sv.q2reliable_datagram.cursize);

			if (client->state != cs_spawned)
				continue;	// datagrams only go to spawned
			SZ_Write (&client->datagram
				, sv.q2datagram.data
				, sv.q2datagram.cursize);
		}
		else
		{
			ClientReliableCheckBlock(client, sv.reliable_datagram.cursize);
			ClientReliableWrite_SZ(client, sv.reliable_datagram.data, sv.reliable_datagram.cursize);

			if (client->state != cs_spawned)
				continue;	// datagrams only go to spawned
			SZ_Write (&client->datagram
				, sv.datagram.data
				, sv.datagram.cursize);
		}
	}

	SZ_Clear (&sv.reliable_datagram);	
	SZ_Clear (&sv.datagram);
#ifdef NQPROT
	SZ_Clear (&sv.nqreliable_datagram);
	SZ_Clear (&sv.nqdatagram);
#endif
	SZ_Clear (&sv.q2reliable_datagram);	
	SZ_Clear (&sv.q2datagram);
}

#ifdef _MSC_VER
#pragma optimize( "", off )
#endif



/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages (void)
{
	int			i, j;
	client_t	*c;

// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, c = svs.clients ; i<MAX_CLIENTS ; i++, c++)
	{
		if (!c->state)
			continue;

		if (c->drop)
		{
			SV_DropClient(c);
			c->drop = false;
			continue;
		}

#ifdef SVCHAT
		SV_ChatThink(c);
#endif

		if (c->wasrecorded)
		{
			c->netchan.message.cursize = 0;
			c->datagram.cursize = 0;
			continue;
		}

		if (c->istobeloaded && c->state == cs_zombie)
		{	//not yet present.
			c->netchan.message.cursize = 0;
			c->datagram.cursize = 0;
			continue;
		}

		// check to see if we have a backbuf to stick in the reliable
		if (c->num_backbuf)
		{
			// will it fit?
			if (c->netchan.message.cursize + c->backbuf_size[0] <
				c->netchan.message.maxsize)
			{

				Con_DPrintf("%s: backbuf %d bytes\n",
					c->name, c->backbuf_size[0]);

				// it'll fit
				SZ_Write(&c->netchan.message, c->backbuf_data[0],
					c->backbuf_size[0]);
				
				//move along, move along
				for (j = 1; j < c->num_backbuf; j++)
				{
					memcpy(c->backbuf_data[j - 1], c->backbuf_data[j],
						c->backbuf_size[j]);
					c->backbuf_size[j - 1] = c->backbuf_size[j];
				}

				c->num_backbuf--;
				if (c->num_backbuf)
				{
					memset(&c->backbuf, 0, sizeof(c->backbuf));
					c->backbuf.data = c->backbuf_data[c->num_backbuf - 1];
					c->backbuf.cursize = c->backbuf_size[c->num_backbuf - 1];
					c->backbuf.maxsize = sizeof(c->backbuf_data[c->num_backbuf - 1]);
				}
			}
		}

		// if the reliable message overflowed,
		// drop the client
		if (c->netchan.message.overflowed)
		{
			SZ_Clear (&c->netchan.message);
			SZ_Clear (&c->datagram);
			SV_BroadcastPrintf (PRINT_HIGH, "%s overflowed\n", c->name);
			Con_Printf ("WARNING: reliable overflow for %s\n",c->name);
			SV_DropClient (c);
			c->send_message = true;
			c->netchan.cleartime = 0;	// don't choke this message
		}

		// only send messages if the client has sent one
		// and the bandwidth is not choked
		if (!c->send_message)
			continue;
		c->send_message = false;	// try putting this after choke?
		if (!sv.paused && !Netchan_CanPacket (&c->netchan, SV_RateForClient(c)))
		{
			c->chokecount++;
			continue;		// bandwidth choke
		}

		if (c->state == cs_spawned)
			SV_SendClientDatagram (c);
		else
			Netchan_Transmit (&c->netchan, 0, NULL, SV_RateForClient(c));	// just update reliable
			
	}

	SV_CleanupEnts();
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif



#define Max(a, b) ((a>b)?a:b)
void SV_SendMVDMessage(void)
{
	int			i, j, cls = 0;
	client_t	*c;
	qbyte		buf[MAX_DATAGRAM];
	sizebuf_t	msg;
	edict_t		*ent;
	int			stats[MAX_QW_STATS];
	float		min_fps;
	extern		cvar_t sv_demofps;
	extern		cvar_t sv_demoPings;
//	extern		cvar_t	sv_demoMaxSize;

	if (!sv.mvdrecording)
		return;

	if (sv_demoPings.value)
	{
		if (sv.time - demo.pingtime > sv_demoPings.value)
		{
			SV_MVDPings();
			demo.pingtime = sv.time;
		}
	}


	if (!sv_demofps.value)
		min_fps = 30.0;
	else
		min_fps = sv_demofps.value;

	min_fps = Max(4, min_fps);
	if (sv.time - demo.time < 1.0/min_fps)
		return;

	for (i=0, c = svs.clients ; i<MAX_CLIENTS ; i++, c++)
	{
		if (c->state != cs_spawned)
			continue;	// datagrams only go to spawned

		cls |= 1 << i;
	}

	if (!cls) {
		SZ_Clear (&demo.datagram);
		return;
	}

	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;
	msg.allowoverflow = true;
	msg.overflowed = false;
	
	for (i=0, c = svs.clients ; i<MAX_CLIENTS ; i++, c++)
	{
		if (c->state != cs_spawned)
			continue;	// datagrams only go to spawned

		if (c->spectator)
			continue;

		ent = c->edict;
		memset (stats, 0, sizeof(stats));

		stats[STAT_HEALTH] = ent->v.health;
		stats[STAT_WEAPON] = SV_ModelIndex(PR_GetString(svprogfuncs, ent->v.weaponmodel));
		stats[STAT_AMMO] = ent->v.currentammo;
		stats[STAT_ARMOR] = ent->v.armorvalue;
		stats[STAT_SHELLS] = ent->v.ammo_shells;
		stats[STAT_NAILS] = ent->v.ammo_nails;
		stats[STAT_ROCKETS] = ent->v.ammo_rockets;
		stats[STAT_CELLS] = ent->v.ammo_cells;
		stats[STAT_ACTIVEWEAPON] = ent->v.weapon;
		

		// stuff the sigil bits into the high bits of items for sbar
		stats[STAT_ITEMS] = (int)ent->v.items | ((int)pr_global_struct->serverflags << 28);

		for (j=0 ; j<MAX_QW_STATS ; j++)
			if (stats[j] != demo.stats[i][j])
			{
				demo.stats[i][j] = stats[j];
				if (stats[j] >=0 && stats[j] <= 255)
				{
					MVDWrite_Begin(dem_stats, i, 3);
					MSG_WriteByte((sizebuf_t*)demo.dbuf, svc_updatestat);
					MSG_WriteByte((sizebuf_t*)demo.dbuf, j);
					MSG_WriteByte((sizebuf_t*)demo.dbuf, stats[j]);
				}
				else
				{
					MVDWrite_Begin(dem_stats, i, 6);
					MSG_WriteByte((sizebuf_t*)demo.dbuf, svc_updatestatlong);
					MSG_WriteByte((sizebuf_t*)demo.dbuf, j);
					MSG_WriteLong((sizebuf_t*)demo.dbuf, stats[j]);
				}
			}
	}

	// send over all the objects that are in the PVS
	// this will include clients, a packetentities, and
	// possibly a nails update
	msg.cursize = 0;
	if (!demo.recorder.delta_sequence)
		demo.recorder.delta_sequence = -1;
	SV_WriteEntitiesToClient (&demo.recorder, &msg, true);
	MVDWrite_Begin(dem_all, 0, msg.cursize);
	SZ_Write ((sizebuf_t*)demo.dbuf, msg.data, msg.cursize);
	// copy the accumulated multicast datagram
	// for this client out to the message
	if (demo.datagram.cursize) {
		MVDWrite_Begin(dem_all, 0, demo.datagram.cursize);
		SZ_Write ((sizebuf_t*)demo.dbuf, demo.datagram.data, demo.datagram.cursize);
		SZ_Clear (&demo.datagram);
	}

	demo.recorder.delta_sequence = demo.recorder.netchan.incoming_sequence&255;
	demo.recorder.netchan.incoming_sequence++;
	demo.frames[demo.parsecount&DEMO_FRAMES_MASK].time = demo.time = sv.time;

	if (demo.parsecount - demo.lastwritten > 60) // that's a backup of 3sec in 20fps, should be enough
	{
		SV_MVDWritePackets(1);
	}

	demo.parsecount++;
	MVDSetMsgBuf(demo.dbuf,&demo.frames[demo.parsecount&DEMO_FRAMES_MASK].buf);
}



/*
=======================
SV_SendMessagesToAll

FIXME: does this sequence right?
=======================
*/
void SV_SendMessagesToAll (void)
{
	int			i;
	client_t	*c;

	for (i=0, c = svs.clients ; i<MAX_CLIENTS ; i++, c++)
		if (c->state)		// FIXME: should this only send to active?
			c->send_message = true;
	
	SV_SendClientMessages ();
}

#endif
