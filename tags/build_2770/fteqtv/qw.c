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


#include "qtv.h"

#define MENU_NONE 0
#define MENU_SERVERS 1
#define MENU_ADMIN 2
#define MENU_ADMINSERVER 3

void Menu_Enter(cluster_t *cluster, viewer_t *viewer, int buttonnum);
void QW_SetMenu(viewer_t *v, int menunum);

#ifdef _WIN32
int snprintf(char *buffer, int buffersize, char *format, ...)
{
	va_list		argptr;
	int ret;
	va_start (argptr, format);
	ret = _vsnprintf (buffer, buffersize, format, argptr);
	buffer[buffersize - 1] = '\0';
	va_end (argptr);

	return ret;
}
#endif
#if (defined(_WIN32) && !defined(_VC80_UPGRADE))
int vsnprintf(char *buffer, int buffersize, char *format, va_list argptr)
{
	int ret;
	ret = _vsnprintf (buffer, buffersize, format, argptr);
	buffer[buffersize - 1] = '\0';

	return ret;
}
#endif

const usercmd_t nullcmd;

#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE3 	(1<<1)
#define	CM_FORWARD	(1<<2)
#define	CM_SIDE		(1<<3)
#define	CM_UP		(1<<4)
#define	CM_BUTTONS	(1<<5)
#define	CM_IMPULSE	(1<<6)
#define	CM_ANGLE2 	(1<<7)
void ReadDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move)
{
	int bits;

	memcpy (move, from, sizeof(*move));

	bits = ReadByte (m);

// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = ReadShort (m);
	if (bits & CM_ANGLE2)
		move->angles[1] = ReadShort (m);
	if (bits & CM_ANGLE3)
		move->angles[2] = ReadShort (m);

// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = ReadShort(m);
	if (bits & CM_SIDE)
		move->sidemove = ReadShort(m);
	if (bits & CM_UP)
		move->upmove = ReadShort(m);

// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = ReadByte (m);

	if (bits & CM_IMPULSE)
		move->impulse = ReadByte (m);

// read time to run command
	move->msec = ReadByte (m);		// always sent
}

void WriteDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move)
{
	int bits = 0;

	if (move->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (move->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (move->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;

	if (move->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (move->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (move->upmove != from->upmove)
		bits |= CM_UP;

	if (move->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (move->impulse != from->impulse)
		bits |= CM_IMPULSE;


	WriteByte (m, bits);

// read current angles
	if (bits & CM_ANGLE1)
		WriteShort (m, move->angles[0]);
	if (bits & CM_ANGLE2)
		WriteShort (m, move->angles[1]);
	if (bits & CM_ANGLE3)
		WriteShort (m, move->angles[2]);

// read movement
	if (bits & CM_FORWARD)
		WriteShort(m, move->forwardmove);
	if (bits & CM_SIDE)
		WriteShort(m, move->sidemove);
	if (bits & CM_UP)
		WriteShort(m, move->upmove);

// read buttons
	if (bits & CM_BUTTONS)
		WriteByte (m, move->buttons);

	if (bits & CM_IMPULSE)
		WriteByte (m, move->impulse);

// read time to run command
	WriteByte (m, move->msec);		// always sent
}










SOCKET QW_InitUDPSocket(int port)
{
	int sock;

	struct sockaddr_in	address;
//	int fromlen;

	unsigned long nonblocking = true;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((u_short)port);



	if ((sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}

	if (ioctlsocket (sock, FIONBIO, &nonblocking) == -1)
	{
		closesocket(sock);
		return INVALID_SOCKET;
	}

	if( bind (sock, (void *)&address, sizeof(address)) == -1)
	{
		closesocket(sock);
		return INVALID_SOCKET;
	}
	return sock;
}

void BuildServerData(sv_t *tv, netmsg_t *msg, qboolean mvd, int servercount)
{
	WriteByte(msg, svc_serverdata);
	WriteLong(msg, PROTOCOL_VERSION);
	WriteLong(msg, servercount);

	if (!tv)
	{
		//dummy connection, for choosing a game to watch.
		WriteString(msg, "qw");

		if (mvd)
			WriteFloat(msg, 0);
		else
			WriteByte(msg, MAX_CLIENTS-1);
		WriteString(msg, "FTEQTV Proxy");


		// get the movevars
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);
		WriteFloat(msg, 0);



		WriteByte(msg, svc_stufftext);
		WriteString2(msg, "fullserverinfo \"");
		WriteString2(msg, "\\*QTV\\"VERSION);
		WriteString(msg, "\"\n");

	}
	else
	{
		WriteString(msg, tv->gamedir);

		if (mvd)
			WriteFloat(msg, 0);
		else
			WriteByte(msg, tv->thisplayer);
		WriteString(msg, tv->mapname);


		// get the movevars
		WriteFloat(msg, tv->movevars.gravity);
		WriteFloat(msg, tv->movevars.stopspeed);
		WriteFloat(msg, tv->movevars.maxspeed);
		WriteFloat(msg, tv->movevars.spectatormaxspeed);
		WriteFloat(msg, tv->movevars.accelerate);
		WriteFloat(msg, tv->movevars.airaccelerate);
		WriteFloat(msg, tv->movevars.wateraccelerate);
		WriteFloat(msg, tv->movevars.friction);
		WriteFloat(msg, tv->movevars.waterfriction);
		WriteFloat(msg, tv->movevars.entgrav);



		WriteByte(msg, svc_stufftext);
		WriteString2(msg, "fullserverinfo \"");
		WriteString2(msg, tv->serverinfo);
		WriteString(msg, "\"\n");
	}
}

void SendServerData(sv_t *tv, viewer_t *viewer)
{
	netmsg_t msg;
	char buffer[1024];
	InitNetMsg(&msg, buffer, sizeof(buffer));

	if (tv && (tv->controller == viewer || !tv->controller))
		viewer->thisplayer = tv->thisplayer;
	else
		viewer->thisplayer = MAX_CLIENTS-1;
	BuildServerData(tv, &msg, false, viewer->servercount);

	SendBufferToViewer(viewer, msg.data, msg.cursize, true);

	viewer->thinksitsconnected = false;

	memset(viewer->currentstats, 0, sizeof(viewer->currentstats));
}

int SendCurrentUserinfos(sv_t *tv, int cursize, netmsg_t *msg, int i, int thisplayer)
{
	if (i < 0)
		return i;
	if (i >= MAX_CLIENTS)
		return i;

	for (; i < MAX_CLIENTS; i++)
	{
		if (i == thisplayer && (!tv || !(tv->controller)))
		{
			WriteByte(msg, svc_updateuserinfo);
			WriteByte(msg, i);
			WriteLong(msg, i);
			WriteString2(msg, "\\*spectator\\1\\name\\");

			if (tv && tv->hostname && tv->hostname[0])
				WriteString(msg, tv->hostname);
			else
				WriteString(msg, "FTEQTV");


			WriteByte(msg, svc_updatefrags);
			WriteByte(msg, i);
			WriteShort(msg, 9999);

			WriteByte(msg, svc_updateping);
			WriteByte(msg, i);
			WriteShort(msg, 0);

			WriteByte(msg, svc_updatepl);
			WriteByte(msg, i);
			WriteByte(msg, 0);

			continue;
		}
		if (!tv)
			continue;
		if (msg->cursize+cursize+strlen(tv->players[i].userinfo) > 768)
		{
			return i;
		}
		WriteByte(msg, svc_updateuserinfo);
		WriteByte(msg, i);
		WriteLong(msg, i);
		WriteString(msg, tv->players[i].userinfo);

		WriteByte(msg, svc_updatefrags);
		WriteByte(msg, i);
		WriteShort(msg, tv->players[i].frags);

		WriteByte(msg, svc_updateping);
		WriteByte(msg, i);
		WriteShort(msg, tv->players[i].ping);

		WriteByte(msg, svc_updatepl);
		WriteByte(msg, i);
		WriteByte(msg, tv->players[i].packetloss);
	}

	i++;

	return i;
}
void WriteEntityState(netmsg_t *msg, entity_state_t *es)
{
	int i;
	WriteByte(msg, es->modelindex);
	WriteByte(msg, es->frame);
	WriteByte(msg, es->colormap);
	WriteByte(msg, es->skinnum);
	for (i = 0; i < 3; i++)
	{
		WriteShort(msg, es->origin[i]);
		WriteByte(msg, es->angles[i]);
	}
}
int SendCurrentBaselines(sv_t *tv, int cursize, netmsg_t *msg, int i)
{

	if (i < 0 || i >= MAX_ENTITIES)
		return i;

	for (; i < MAX_ENTITIES; i++)
	{
		if (msg->cursize+cursize+16 > 768)
		{
			return i;
		}
		WriteByte(msg, svc_spawnbaseline);
		WriteShort(msg, i);
		WriteEntityState(msg, &tv->entity[i].baseline);
	}

	return i;
}
int SendCurrentLightmaps(sv_t *tv, int cursize, netmsg_t *msg, int i)
{
	if (i < 0 || i >= MAX_LIGHTSTYLES)
		return i;

	for (; i < MAX_LIGHTSTYLES; i++)
	{
		if (msg->cursize+cursize+strlen(tv->lightstyle[i].name) > 768)
		{
			return i;
		}
		WriteByte(msg, svc_lightstyle);
		WriteByte(msg, i);
		WriteString(msg, tv->lightstyle[i].name);
	}
	return i;
}
int SendStaticSounds(sv_t *tv, int cursize, netmsg_t *msg, int i)
{
	if (i < 0 || i >= MAX_STATICSOUNDS)
		return i;

	for (; i < MAX_STATICSOUNDS; i++)
	{
		if (msg->cursize+cursize+16 > 768)
		{
			return i;
		}
		if (!tv->staticsound[i].soundindex)
			continue;

		WriteByte(msg, svc_spawnstaticsound);
		WriteShort(msg, tv->staticsound[i].origin[0]);
		WriteShort(msg, tv->staticsound[i].origin[1]);
		WriteShort(msg, tv->staticsound[i].origin[2]);
		WriteByte(msg, tv->staticsound[i].soundindex);
		WriteByte(msg, tv->staticsound[i].volume);
		WriteByte(msg, tv->staticsound[i].attenuation);
	}

	return i;
}
int SendStaticEntities(sv_t *tv, int cursize, netmsg_t *msg, int i)
{
	if (i < 0 || i >= MAX_STATICENTITIES)
		return i;

	for (; i < MAX_STATICENTITIES; i++)
	{
		if (msg->cursize+cursize+16 > 768)
		{
			return i;
		}
		if (!tv->spawnstatic[i].modelindex)
			continue;

		WriteByte(msg, svc_spawnstatic);
		WriteEntityState(msg, &tv->spawnstatic[i]);
	}

	return i;
}

int SendList(sv_t *qtv, int first, const filename_t *list, int svc, netmsg_t *msg)
{
	int i;

	WriteByte(msg, svc);
	WriteByte(msg, first);
	for (i = first+1; i < 256; i++)
	{
//		printf("write %i: %s\n", i, list[i].name);
		WriteString(msg, list[i].name);
		if (!*list[i].name)	//fixme: this probably needs testing for where we are close to the limit
		{	//no more
			WriteByte(msg, 0);
			return -1;
		}

		if (msg->cursize > 768)
		{	//truncate
			i--;
			break;
		}
	}
	WriteByte(msg, 0);
	WriteByte(msg, i);

	return i;
}

void QW_SetViewersServer(viewer_t *viewer, sv_t *sv)
{
	if (viewer->server)
		viewer->server->numviewers--;
	viewer->server = sv;
	if (viewer->server)
		viewer->server->numviewers++;
	if (!sv || !sv->parsingconnectiondata)
	{
		QW_StuffcmdToViewer(viewer, "cmd new\n");
	}
	viewer->servercount++;
	viewer->origin[0] = 0;
	viewer->origin[1] = 0;
	viewer->origin[2] = 0;
}

//fixme: will these want to have state?..
int NewChallenge(netadr_t *addr)
{
	return 4;
}
qboolean ChallengePasses(netadr_t *addr, int challenge)
{
	if (challenge == 4)
		return true;
	return false;
}

void NewQWClient(cluster_t *cluster, netadr_t *addr, char *connectmessage)
{
	sv_t *initialserver;
	viewer_t *viewer;

	char qport[32];
	char challenge[32];
	char infostring[256];

	connectmessage+=11;

	connectmessage = COM_ParseToken(connectmessage, qport, sizeof(qport), "");
	connectmessage = COM_ParseToken(connectmessage, challenge, sizeof(challenge), "");
	connectmessage = COM_ParseToken(connectmessage, infostring, sizeof(infostring), "");

	if (!ChallengePasses(addr, atoi(challenge)))
	{
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *addr, "n" "Bad challenge");
		return;
	}


	viewer = malloc(sizeof(viewer_t));
	if (!viewer)
	{
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *addr, "n" "Out of memory");
		return;
	}
	memset(viewer, 0, sizeof(viewer_t));

	viewer->trackplayer = -1;
	Netchan_Setup (cluster->qwdsocket, &viewer->netchan, *addr, atoi(qport), false);

	viewer->next = cluster->viewers;
	cluster->viewers = viewer;
	viewer->delta_frame = -1;

	initialserver = NULL;
	if (cluster->numservers == 1)
	{
		initialserver = cluster->servers;
		if (!initialserver->modellist[1].name[0])
			initialserver = NULL;	//damn, that server isn't ready
	}

	viewer->server = initialserver;
	if (viewer->server)
		viewer->server->numviewers++;

	if (!initialserver)
	{
		QW_SetMenu(viewer, MENU_SERVERS);
	}
	else
	{
		viewer->menunum = -1;
		QW_SetMenu(viewer, MENU_NONE);
	}

	cluster->numviewers++;

	Info_ValueForKey(infostring, "name", viewer->name, sizeof(viewer->name));
	strncpy(viewer->userinfo, infostring, sizeof(viewer->userinfo)-1);

	Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *addr, "j");

	QW_PrintfToViewer(viewer, "Welcome to FTEQTV\n");
	QW_StuffcmdToViewer(viewer, "alias admin \"cmd admin\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:up\" \"say proxy:menu up\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:down\" \"say proxy:menu down\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:right\" \"say proxy:menu right\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:left\" \"say proxy:menu left\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:select\" \"say proxy:menu select\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:home\" \"say proxy:menu home\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:end\" \"say proxy:menu end\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:menu\" \"say proxy:menu\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:backspace\" \"say proxy:menu backspace\"\n");

	QW_PrintfToViewer(viewer, "Type admin for the admin menu\n");

//	if 
}

void QW_SetMenu(viewer_t *v, int menunum)
{
	if ((v->menunum==MENU_NONE) != (menunum==MENU_NONE))
	{
		if (menunum != MENU_NONE)
		{
			QW_StuffcmdToViewer(v, "alias \"+proxfwd\" \"proxy:up\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxback\" \"proxy:down\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxleft\" \"proxy:left\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxright\" \"proxy:right\"\n");

			QW_StuffcmdToViewer(v, "alias \"-proxfwd\" \" \"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxback\" \" \"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxleft\" \" \"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxright\" \" \"\n");
		}
		else
		{
			QW_StuffcmdToViewer(v, "alias \"+proxfwd\" \"+forward\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxback\" \"+back\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxleft\" \"+moveleft\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxright\" \"+moveright\"\n");

			QW_StuffcmdToViewer(v, "alias \"-proxfwd\" \"-forward\"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxback\" \"-back\"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxleft\" \"-moveleft\"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxright\" \"-moveright\"\n");
		}
	}

	v->menunum = menunum;
	v->menuop = 0;
}

void QTV_Rcon(cluster_t *cluster, char *message, netadr_t *from)
{
	char buffer[8192];

	char *command;
	int passlen;

	if (!*cluster->password)
	{
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "n" "Bad rcon_password.\n");
		return;
	}

	while(*message > '\0' && *message <= ' ')
		message++;

	command = strchr(message, ' ');
	passlen = command-message;
	if (passlen != strlen(cluster->password) || strncmp(message, cluster->password, passlen))
	{
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "n" "Bad rcon_password.\n");
		return;
	}

	Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "n%s", Rcon_Command(cluster, NULL, command, buffer, sizeof(buffer), false));
}

void QTV_Status(cluster_t *cluster, netadr_t *from)
{
	int i;
	char buffer[8192];
	sv_t *sv;

	netmsg_t msg;
	char elem[256];
	InitNetMsg(&msg, buffer, sizeof(buffer));
	WriteLong(&msg, -1);
	WriteByte(&msg, 'n');

	if (cluster->numservers==1)
	{	//show this server's info
		sv = cluster->servers;

		WriteString2(&msg, sv->serverinfo);
		WriteString2(&msg, "\n");

		for (i = 0;i < MAX_CLIENTS; i++)
		{
			if (i == sv->thisplayer)
				continue;
			if (!sv->players[i].active)
				continue;
			//userid
			sprintf(elem, "%i", i);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//frags
			sprintf(elem, "%i", sv->players[i].frags);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//time (minuites)
			sprintf(elem, "%i", 0);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//ping
			sprintf(elem, "%i", sv->players[i].ping);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//name
			Info_ValueForKey(sv->players[i].userinfo, "name", elem, sizeof(elem));
			WriteString2(&msg, "\"");
			WriteString2(&msg, elem);
			WriteString2(&msg, "\" ");

			//skin
			Info_ValueForKey(sv->players[i].userinfo, "skin", elem, sizeof(elem));
			WriteString2(&msg, "\"");
			WriteString2(&msg, elem);
			WriteString2(&msg, "\" ");
			WriteString2(&msg, " ");

			//tc
			Info_ValueForKey(sv->players[i].userinfo, "topcolor", elem, sizeof(elem));
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//bc
			Info_ValueForKey(sv->players[i].userinfo, "bottomcolor", elem, sizeof(elem));
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			WriteString2(&msg, "\n");
		}
	}
	else
	{
		WriteString2(&msg, "\\hostname\\");
		WriteString2(&msg, cluster->hostname);

		for (sv = cluster->servers, i = 0; sv; sv = sv->next, i++)
		{
			sprintf(elem, "\\%i\\", i);
			WriteString2(&msg, elem);
			WriteString2(&msg, sv->serveraddress);
			sprintf(elem, " (%s)", sv->serveraddress);
			WriteString2(&msg, sv->serveraddress);
		}
	}

	WriteByte(&msg, 0);
	NET_SendPacket(cluster, cluster->qwdsocket, msg.cursize, msg.data, *from);
}


void ConnectionlessPacket(cluster_t *cluster, netadr_t *from, netmsg_t *m)
{
	char buffer[MAX_MSGLEN];
	int i;

	ReadLong(m);
	ReadString(m, buffer, sizeof(buffer));

	if (!strncmp(buffer, "rcon ", 5))
	{
		QTV_Rcon(cluster, buffer+5, from);
		return;
	}
	if (!strncmp(buffer, "ping", 4))
	{	//ack
		NET_SendPacket (cluster, cluster->qwdsocket, 1, "l", *from);
		return;
	}
	if (!strncmp(buffer, "status", 6))
	{
		QTV_Status(cluster, from);
		return;
	}
	if (!strncmp(buffer, "getchallenge", 12))
	{
		i = NewChallenge(from);
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "c%i", i);
		return;
	}
	if (!strncmp(buffer, "connect 28 ", 11))
	{
		if (cluster->numviewers >= cluster->maxviewers && cluster->maxviewers)
			Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "n" "Sorry, proxy is full.\n");
		else
			NewQWClient(cluster, from, buffer);
		return;
	}
	if (!strncmp(buffer, "l\n", 2))
	{
		Sys_Printf(cluster, "Ack\n");
	}
}


void SV_WriteDelta(int entnum, const entity_state_t *from, const entity_state_t *to, netmsg_t *msg, qboolean force)
{
	unsigned int i;
	unsigned int bits;

	bits = 0;
	if (from->angles[0] != to->angles[0])
		bits |= U_ANGLE1;
	if (from->angles[1] != to->angles[1])
		bits |= U_ANGLE2;
	if (from->angles[2] != to->angles[2])
		bits |= U_ANGLE3;

	if (from->origin[0] != to->origin[0])
		bits |= U_ORIGIN1;
	if (from->origin[1] != to->origin[1])
		bits |= U_ORIGIN2;
	if (from->origin[2] != to->origin[2])
		bits |= U_ORIGIN3;

	if (from->colormap != to->colormap)
		bits |= U_COLORMAP;
	if (from->skinnum != to->skinnum)
		bits |= U_SKIN;
	if (from->modelindex != to->modelindex)
		bits |= U_MODEL;
	if (from->frame != to->frame)
		bits |= U_FRAME;
	if (from->effects != to->effects)
		bits |= U_EFFECTS;

	if (bits & 255)
		bits |= U_MOREBITS;



	if (!bits && !force)
		return;

	i = (entnum&511) | (bits&~511);
	WriteShort (msg, i);

	if (bits & U_MOREBITS)
		WriteByte (msg, bits&255);
/*
#ifdef PROTOCOLEXTENSIONS
	if (bits & U_EVENMORE)
		WriteByte (msg, evenmorebits&255);
	if (evenmorebits & U_YETMORE)
		WriteByte (msg, (evenmorebits>>8)&255);
#endif
*/
	if (bits & U_MODEL)
		WriteByte (msg,	to->modelindex&255);
	if (bits & U_FRAME)
		WriteByte (msg, to->frame);
	if (bits & U_COLORMAP)
		WriteByte (msg, to->colormap);
	if (bits & U_SKIN)
		WriteByte (msg, to->skinnum);
	if (bits & U_EFFECTS)
		WriteByte (msg, to->effects&0x00ff);
	if (bits & U_ORIGIN1)
		WriteShort (msg, to->origin[0]);
	if (bits & U_ANGLE1)
		WriteByte(msg, to->angles[0]);
	if (bits & U_ORIGIN2)
		WriteShort (msg, to->origin[1]);
	if (bits & U_ANGLE2)
		WriteByte(msg, to->angles[1]);
	if (bits & U_ORIGIN3)
		WriteShort (msg, to->origin[2]);
	if (bits & U_ANGLE3)
		WriteByte(msg, to->angles[2]);
}

const entity_state_t nullentstate;
void SV_EmitPacketEntities (const sv_t *qtv, const viewer_t *v, const packet_entities_t *to, netmsg_t *msg)
{
	const entity_state_t *baseline;
	const packet_entities_t *from;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		oldmax;

	// this is the frame that we are going to delta update from
	if (v->delta_frame != -1)
	{
		from = &v->frame[v->delta_frame & (ENTITY_FRAMES-1)];
		oldmax = from->numents;

		WriteByte (msg, svc_deltapacketentities);
		WriteByte (msg, v->delta_frame);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		WriteByte (msg, svc_packetentities);
	}

	newindex = 0;
	oldindex = 0;
//Con_Printf ("---%i to %i ----\n", client->delta_sequence & UPDATE_MASK
//			, client->netchan.outgoing_sequence & UPDATE_MASK);
	while (newindex < to->numents || oldindex < oldmax)
	{
		newnum = newindex >= to->numents ? 9999 : to->entnum[newindex];
		oldnum = oldindex >= oldmax ? 9999 : from->entnum[oldindex];

		if (newnum == oldnum)
		{	// delta update from old position
//Con_Printf ("delta %i\n", newnum);
			SV_WriteDelta (newnum, &from->ents[oldindex], &to->ents[newindex], msg, false);

			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
			baseline = &qtv->entity[newnum].baseline;
//Con_Printf ("baseline %i\n", newnum);
			SV_WriteDelta (newnum, baseline, &to->ents[newindex], msg, true);

			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
//Con_Printf ("remove %i\n", oldnum);
			WriteShort (msg, oldnum | U_REMOVE);
			oldindex++;
			continue;
		}
	}

	WriteShort (msg, 0);	// end of packetentities
}

void Prox_SendInitialEnts(sv_t *qtv, oproxy_t *prox, netmsg_t *msg)
{
	int i;
	WriteByte(msg, svc_packetentities);
	for (i = 0; i < qtv->maxents; i++)
	{
		if (qtv->entity[i].current.modelindex)
			SV_WriteDelta(i, &nullentstate, &qtv->entity[i].current, msg, true);
	}
	WriteShort(msg, 0);
}

static float InterpolateAngle(float current, float ideal, float fraction)
{
	float move;

	move = ideal - current;
	if (move >= 32767)
		move -= 65535;
	else if (move <= -32767)
		move += 65535;

	return current + fraction * move;
}

void SendLocalPlayerState(sv_t *tv, viewer_t *v, int playernum, netmsg_t *msg)
{
	int flags;
	int j;

	WriteByte(msg, svc_playerinfo);
	WriteByte(msg, playernum);

	if (tv && tv->controller == v)
	{
		v->trackplayer = tv->thisplayer;
		flags = 0;
		if (tv->players[tv->thisplayer].current.weaponframe)
			flags |= PF_WEAPONFRAME;
		if (tv->players[tv->thisplayer].current.effects)
			flags |= PF_EFFECTS;
		for (j=0 ; j<3 ; j++)
			if (tv->players[tv->thisplayer].current.velocity[j])
				flags |= (PF_VELOCITY1<<j);

		WriteShort(msg, flags);
		WriteShort(msg, tv->players[tv->thisplayer].current.origin[0]);
		WriteShort(msg, tv->players[tv->thisplayer].current.origin[1]);
		WriteShort(msg, tv->players[tv->thisplayer].current.origin[2]);
		WriteByte(msg, tv->players[tv->thisplayer].current.frame);

		for (j=0 ; j<3 ; j++)
			if (flags & (PF_VELOCITY1<<j) )
				WriteShort (msg, tv->players[tv->thisplayer].current.velocity[j]);

		if (flags & PF_MODEL)
			WriteByte(msg, tv->players[tv->thisplayer].current.modelindex);
		if (flags & PF_SKINNUM)
			WriteByte(msg, tv->players[tv->thisplayer].current.skinnum);
		if (flags & PF_EFFECTS)
			WriteByte(msg, tv->players[tv->thisplayer].current.effects);
		if (flags & PF_WEAPONFRAME)
			WriteByte(msg, tv->players[tv->thisplayer].current.weaponframe);
	}
	else
	{
		WriteShort(msg, 0);
		WriteShort(msg, v->origin[0]*8);
		WriteShort(msg, v->origin[1]*8);
		WriteShort(msg, v->origin[2]*8);
		WriteByte(msg, 0);
	}
}

void SendPlayerStates(sv_t *tv, viewer_t *v, netmsg_t *msg)
{
	packet_entities_t *e;
	int i;
	usercmd_t to;
	unsigned short flags;
	short interp;
	float lerp;


	memset(&to, 0, sizeof(to));

	if (tv)
	{
		if (tv->physicstime != v->settime && tv->cluster->chokeonnotupdated)
		{
			WriteByte(msg, svc_updatestatlong);
			WriteByte(msg, STAT_TIME);
			WriteLong(msg, v->settime);

			v->settime = tv->physicstime;
		}

		BSP_SetupForPosition(tv->bsp, v->origin[0], v->origin[1], v->origin[2]);

		lerp = ((tv->simtime - tv->oldpackettime)/1000.0f) / ((tv->nextpackettime - tv->oldpackettime)/1000.0f);
		if (lerp < 0)
			lerp = 0;
		if (lerp > 1)
			lerp = 1;

		if (tv->controller == v)
			lerp = 1;

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (i == v->thisplayer)
			{
				SendLocalPlayerState(tv, v, i, msg);
				continue;
			}
			if (!tv->players[i].active)
				continue;

			if (v->trackplayer != i && !BSP_Visible(tv->bsp, tv->players[i].leafcount, tv->players[i].leafs))
				continue;

			flags = PF_COMMAND;
			if (v->trackplayer == i && tv->players[i].current.weaponframe)
				flags |= PF_WEAPONFRAME;

			WriteByte(msg, svc_playerinfo);
			WriteByte(msg, i);
			WriteShort(msg, flags);

			interp = (lerp)*tv->players[i].current.origin[0] + (1-lerp)*tv->players[i].old.origin[0];
			WriteShort(msg, interp);
			interp = (lerp)*tv->players[i].current.origin[1] + (1-lerp)*tv->players[i].old.origin[1];
			WriteShort(msg, interp);
			interp = (lerp)*tv->players[i].current.origin[2] + (1-lerp)*tv->players[i].old.origin[2];
			WriteShort(msg, interp);

			WriteByte(msg, tv->players[i].current.frame);

			if (flags & PF_MSEC)
			{
				WriteByte(msg, 0);
			}
			if (flags & PF_COMMAND)
			{
	//			to.angles[0] = tv->players[i].current.angles[0];
	//			to.angles[1] = tv->players[i].current.angles[1];
	//			to.angles[2] = tv->players[i].current.angles[2];

				to.angles[0] = InterpolateAngle(tv->players[i].old.angles[0], tv->players[i].current.angles[0], lerp);
				to.angles[1] = InterpolateAngle(tv->players[i].old.angles[1], tv->players[i].current.angles[1], lerp);
				to.angles[2] = InterpolateAngle(tv->players[i].old.angles[2], tv->players[i].current.angles[2], lerp);
				WriteDeltaUsercmd(msg, &nullcmd, &to);
			}
			//vel
			//model
			//skin
			//effects
			//weaponframe
			if (flags & PF_WEAPONFRAME)
				WriteByte(msg, tv->players[i].current.weaponframe);
		}
	}
	else
	{
		lerp = 1;

		SendLocalPlayerState(tv, v, v->thisplayer, msg);
	}


	e = &v->frame[v->netchan.outgoing_sequence&(ENTITY_FRAMES-1)];
	e->numents = 0;
	if (tv)
		for (i = 0; i < tv->maxents; i++)
		{
			if (!tv->entity[i].current.modelindex)
				continue;

			if (tv->entity[i].current.modelindex >= tv->numinlines && !BSP_Visible(tv->bsp, tv->entity[i].leafcount, tv->entity[i].leafs))
				continue;

			e->entnum[e->numents] = i;
			memcpy(&e->ents[e->numents], &tv->entity[i].current, sizeof(entity_state_t));

			if (tv->entity[i].updatetime == tv->oldpackettime)
			{
				e->ents[e->numents].origin[0] = (lerp)*tv->entity[i].current.origin[0] + (1-lerp)*tv->entity[i].old.origin[0];
				e->ents[e->numents].origin[1] = (lerp)*tv->entity[i].current.origin[1] + (1-lerp)*tv->entity[i].old.origin[1];
				e->ents[e->numents].origin[2] = (lerp)*tv->entity[i].current.origin[2] + (1-lerp)*tv->entity[i].old.origin[2];
			}

			e->numents++;

			if (e->numents == ENTS_PER_FRAME)
				break;
		}

	SV_EmitPacketEntities(tv, v, e, msg);

	if (tv && tv->nailcount)
	{
		WriteByte(msg, svc_nails);
		WriteByte(msg, tv->nailcount);
		for (i = 0; i < tv->nailcount; i++)
		{
			WriteByte(msg, tv->nails[i].bits[0]);
			WriteByte(msg, tv->nails[i].bits[1]);
			WriteByte(msg, tv->nails[i].bits[2]);
			WriteByte(msg, tv->nails[i].bits[3]);
			WriteByte(msg, tv->nails[i].bits[4]);
			WriteByte(msg, tv->nails[i].bits[5]);
		}
	}
}

void UpdateStats(sv_t *qtv, viewer_t *v)
{
	netmsg_t msg;
	char buf[6];
	int i;
	const static unsigned int nullstats[MAX_STATS] = {1000};

	const unsigned int *stats;

	InitNetMsg(&msg, buf, sizeof(buf));

	if (qtv && qtv->controller == v)
		stats = qtv->players[qtv->thisplayer].stats;
	else if (v->trackplayer != -1 || !qtv)
		stats = nullstats;
	else
		stats = qtv->players[v->trackplayer].stats;

	for (i = 0; i < MAX_STATS; i++)
	{
		if (v->currentstats[i] != stats[i])
		{
			if (stats[i] < 256)
			{
				WriteByte(&msg, svc_updatestat);
				WriteByte(&msg, i);
				WriteByte(&msg, stats[i]);
			}
			else
			{
				WriteByte(&msg, svc_updatestatlong);
				WriteByte(&msg, i);
				WriteLong(&msg, stats[i]);
			}
			SendBufferToViewer(v, msg.data, msg.cursize, true);
			msg.cursize = 0;
			v->currentstats[i] = stats[i];
		}
	}
}

//returns the next prespawn 'buffer' number to use, or -1 if no more
int Prespawn(sv_t *qtv, int curmsgsize, netmsg_t *msg, int bufnum, int thisplayer)
{
	int r, ni;
	r = bufnum;

	ni = SendCurrentUserinfos(qtv, curmsgsize, msg, bufnum, thisplayer);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_CLIENTS;

	ni = SendCurrentBaselines(qtv, curmsgsize, msg, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_ENTITIES;

	ni = SendCurrentLightmaps(qtv, curmsgsize, msg, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_LIGHTSTYLES;

	ni = SendStaticSounds(qtv, curmsgsize, msg, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_STATICSOUNDS;

	ni = SendStaticEntities(qtv, curmsgsize, msg, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_STATICENTITIES;

	if (bufnum == 0)
		return -1;

	return r;
}

#define M_PI 3.1415926535897932384626433832795
#include <math.h>
void AngleVectors (short angles[3], float *forward, float *right, float *up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[1] * (M_PI*2 / 65535);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[0] * (M_PI*2 / 65535);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[2] * (M_PI*2 / 65535);
	sr = sin(angle);
	cr = cos(angle);

	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;
	right[0] = (-1*sr*sp*cy+-1*cr*-sy);
	right[1] = (-1*sr*sp*sy+-1*cr*cy);
	right[2] = -1*sr*cp;
	up[0] = (cr*sp*cy+-sr*-sy);
	up[1] = (cr*sp*sy+-sr*cy);
	up[2] = cr*cp;
}

void PMove(viewer_t *v, usercmd_t *cmd)
{
	int i;
	float fwd[3], rgt[3], up[3];
	if (v->server && v->server->controller == v)
	{
		v->origin[0] = v->server->players[v->server->thisplayer].current.origin[0]/8.0f;
		v->origin[1] = v->server->players[v->server->thisplayer].current.origin[1]/8.0f;
		v->origin[2] = v->server->players[v->server->thisplayer].current.origin[2]/8.0f;
		return;
	}
	AngleVectors(cmd->angles, fwd, rgt, up);

	for (i = 0; i < 3; i++)
		v->origin[i] += (cmd->forwardmove*fwd[i] + cmd->sidemove*rgt[i] + cmd->upmove*up[i])*(cmd->msec/1000.0f);
}

void QTV_Say(cluster_t *cluster, sv_t *qtv, viewer_t *v, char *message, qboolean noupwards)
{
	char buf[1024];
	netmsg_t msg;

	if (message[strlen(message)-1] == '\"')
		message[strlen(message)-1] = '\0';

	if (*v->expectcommand)
	{
		buf[sizeof(buf)-1] = '\0';
		if (!strcmp(v->expectcommand, "hostname"))
		{
			strncpy(cluster->hostname, message, sizeof(cluster->hostname));
			cluster->hostname[sizeof(cluster->hostname)-1] = '\0';
		}
		else if (!strcmp(v->expectcommand, "master"))
		{
			strncpy(cluster->master, message, sizeof(cluster->master));
			cluster->master[sizeof(cluster->master)-1] = '\0';
			if (!strcmp(cluster->master, "."))
				*cluster->master = '\0';
			cluster->mastersendtime = cluster->curtime;
		}
		else if (!strcmp(v->expectcommand, "addserver"))
		{
			snprintf(buf, sizeof(buf), "tcp:%s", message);
			qtv = QTV_NewServerConnection(cluster, buf, false, false, false);
			if (qtv)
			{
				QW_SetViewersServer(v, qtv);
				QW_PrintfToViewer(v, "Connected\n", message);
			}
			else
				QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
		}
		else if (!strcmp(v->expectcommand, "admin"))
		{
			if (!strcmp(message, cluster->password))
			{
				QW_SetMenu(v, MENU_ADMIN);
				v->isadmin = true;
				Sys_Printf(cluster, "Player %s logs in as admin\n", v->name);
			}
			else
			{
				QW_PrintfToViewer(v, "Admin password incorrect\n");
				Sys_Printf(cluster, "Player %s gets incorrect admin password\n", v->name);
			}
		}
		else if (!strcmp(v->expectcommand, "adddemo"))
		{
			snprintf(buf, sizeof(buf), "file:%s", message);
			qtv = QTV_NewServerConnection(cluster, buf, false, false, false);
			if (!qtv)
				QW_PrintfToViewer(v, "Failed to play demo \"%s\"\n", message);
			else
			{
				QW_SetViewersServer(v, qtv);
				QW_PrintfToViewer(v, "Opened demo file.\n", message);
			}
		}
		else if (!strcmp(v->expectcommand, "setmvdport"))
		{
			int newp;
			int news;
			if (qtv)
			{
				newp = atoi(message);

				if (newp)
				{
					news = Net_MVDListen(newp);

					if (news != INVALID_SOCKET)
					{
						if (qtv->listenmvd != INVALID_SOCKET)
							closesocket(qtv->listenmvd);
						qtv->listenmvd = news;
						qtv->tcplistenportnum = newp;
						qtv->disconnectwhennooneiswatching = false;
					}
				}
				else if (qtv->listenmvd != INVALID_SOCKET)
				{
					closesocket(qtv->listenmvd);
					qtv->listenmvd = INVALID_SOCKET;
				}
			}
			else
				QW_PrintfToViewer(v, "You were disconnected from that stream\n");
		}
		else
		{
			QW_PrintfToViewer(v, "Command %s was not recognised\n", v->expectcommand);
		}

		*v->expectcommand = '\0';
		return;
	}
	if (!strncmp(message, ".help", 5))
	{
		QW_PrintfToViewer(v,	"Website: http://www.fteqw.com/\n"
								"Commands:\n"
								".qw qwserver:port\n"
								".qtv tcpserver:port\n"
								".demo gamedir/demoname.mvd\n"
								".disconnect\n");
	}
	else if (!strncmp(message, ".connect ", 9) || !strncmp(message, ".qw ", 4))
	{
		if (!strncmp(message, ".qw ", 4))
			message += 4;
		else
			message += 9;
		snprintf(buf, sizeof(buf), "udp:%s", message);
		qtv = QTV_NewServerConnection(cluster, buf, false, true, true);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(v, qtv);
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
	}
	else if (!strncmp(message, ".join ", 6))
	{
		message += 6;
		snprintf(buf, sizeof(buf), "udp:%s", message);
		qtv = QTV_NewServerConnection(cluster, buf, false, true, false);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(v, qtv);
			qtv->controller = v;
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
	}
	else if (!strncmp(message, ".qtv ", 5))
	{
		message += 5;
		snprintf(buf, sizeof(buf), "tcp:%s", message);
		qtv = QTV_NewServerConnection(cluster, buf, false, true, true);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(v, qtv);
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
	}
	else if (!strncmp(message, ".demo ", 6))
	{
		message += 6;
		snprintf(buf, sizeof(buf), "file:%s", message);
		qtv = QTV_NewServerConnection(cluster, buf, false, true, true);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(v, qtv);
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
	}
	else if (!strncmp(message, ".disconnect", 11))
	{
		QW_SetMenu(v, MENU_SERVERS);
		QW_SetViewersServer(v, NULL);
		QW_PrintfToViewer(v, "Connected\n", message);
	}
	else if (!strncmp(message, ".admin", 11))
	{
		QW_StuffcmdToViewer(v, "cmd admin\n");
	}
	else if (!strncmp(message, "proxy:menu up", 13))
	{
		v->menuop -= 1;
	}
	else if (!strncmp(message, "proxy:menu down", 15))
	{
		v->menuop += 1;
	}
	else if (!strncmp(message, "proxy:menu right", 16))
	{
		Menu_Enter(cluster, v, 1);
	}
	else if (!strncmp(message, "proxy:menu left", 15))
	{
		Menu_Enter(cluster, v, -1);
	}
	else if (!strncmp(message, "proxy:menu select", 17))
	{
		Menu_Enter(cluster, v, 0);
	}
	else if (!strncmp(message, "proxy:menu home", 15))
	{
		v->menuop -= 100000;
	}
	else if (!strncmp(message, "proxy:menu end", 14))
	{
		v->menuop += 100000;
	}
	else if (!strncmp(message, "proxy:menu back", 15))
	{
	}
	else if (!strncmp(message, "proxy:menu", 10))
	{
		if (v->menunum)
			Menu_Enter(cluster, v, 0);
		else
			QW_SetMenu(v, MENU_SERVERS);
	}
	else if (!strncmp(message, ".menu bind", 10) || !strncmp(message, "proxy:menu bindstd", 18))
	{
		QW_StuffcmdToViewer(v, "bind uparrow proxy:menu up\n");
		QW_StuffcmdToViewer(v, "bind downarrow proxy:menu down\n");
		QW_StuffcmdToViewer(v, "bind rightarrow proxy:menu right\n");
		QW_StuffcmdToViewer(v, "bind leftarrow proxy:menu left\n");

		QW_StuffcmdToViewer(v, "bind enter proxy:menu select\n");

		QW_StuffcmdToViewer(v, "bind home proxy:menu home\n");
		QW_StuffcmdToViewer(v, "bind end proxy:menu end\n");
		QW_StuffcmdToViewer(v, "bind pause proxy:menu\n");
		QW_StuffcmdToViewer(v, "bind backspace proxy:back up\n");
	}
	else
	{
		*v->expectcommand = '\0';

		if (cluster->notalking)
			return;

		if (qtv && qtv->usequkeworldprotocols && !noupwards)
		{
			if (qtv->controller == v || !*v->name)
				SendClientCommand(qtv, "say %s\n", message);
			else
				SendClientCommand(qtv, "say %s: %s\n", v->name, message);
		}
		else
		{

			InitNetMsg(&msg, buf, sizeof(buf));

			WriteByte(&msg, svc_print);
			WriteByte(&msg, 3);	//PRINT_CHAT
			WriteString2(&msg, v->name);
			WriteString2(&msg, "\x8d ");
			WriteString2(&msg, message);
			WriteString(&msg, "\n");

			Broadcast(cluster, msg.data, msg.cursize);
		}
	}
}

viewer_t *QW_IsOn(cluster_t *cluster, char *name)
{
	viewer_t *v;
	for (v = cluster->viewers; v; v = v->next)
		if (!strcmp(v->name, name))		//this needs to allow dequakified names.
			return v;

	return NULL;
}

void QW_PrintfToViewer(viewer_t *v, char *format, ...)
{
	va_list		argptr;
	char buf[1024];

	va_start (argptr, format);
	vsnprintf (buf+2, sizeof(buf)-2, format, argptr);
	va_end (argptr);

	buf[0] = svc_print;
	buf[1] = 2;	//PRINT_HIGH

	SendBufferToViewer(v, buf, strlen(buf)+1, true);
}


void QW_StuffcmdToViewer(viewer_t *v, char *format, ...)
{
	va_list		argptr;
	char buf[1024];

	va_start (argptr, format);
	vsnprintf (buf+1, sizeof(buf)-1, format, argptr);
	va_end (argptr);

	buf[0] = svc_stufftext;
	SendBufferToViewer(v, buf, strlen(buf)+1, true);
}

static const filename_t ConnectionlessModelList[] = {{""}, {"maps/start.bsp"}, {"progs/player.mdl"}, {""}};
static const filename_t ConnectionlessSoundList[] = {{""}, {""}};


void ParseQWC(cluster_t *cluster, sv_t *qtv, viewer_t *v, netmsg_t *m)
{
//	usercmd_t	oldest, oldcmd, newcmd;
	char buf[1024];
	netmsg_t msg;

	v->delta_frame = -1;

	while (m->readpos < m->cursize)
	{
		switch (ReadByte(m))
		{
		case clc_nop:
			return;
		case clc_delta:
			v->delta_frame = ReadByte(m);
			break;
		case clc_stringcmd:
			ReadString (m, buf, sizeof(buf));
			printf("stringcmd: %s\n", buf);

			if (!strcmp(buf, "new"))
			{
				if (qtv && qtv->parsingconnectiondata)
					QW_StuffcmdToViewer(v, "cmd new\n");
				else
				{
					SendServerData(qtv, v);
				}
			}
			else if (!strncmp(buf, "modellist ", 10))
			{
				char *cmd = buf+10;
				int svcount = atoi(cmd);
				int first;

				while((*cmd >= '0' && *cmd <= '9') || *cmd == '-')
					cmd++;
				first = atoi(cmd);

				InitNetMsg(&msg, buf, sizeof(buf));

				if (svcount != v->servercount)
				{	//looks like we changed map without them.
					SendServerData(qtv, v);
					return;
				}

				if (!qtv)
					SendList(qtv, first, ConnectionlessModelList, svc_modellist, &msg);
				else
					SendList(qtv, first, qtv->modellist, svc_modellist, &msg);
				SendBufferToViewer(v, msg.data, msg.cursize, true);
			}
			else if (!strncmp(buf, "soundlist ", 10))
			{
				char *cmd = buf+10;
				int svcount = atoi(cmd);
				int first;

				while((*cmd >= '0' && *cmd <= '9') || *cmd == '-')
					cmd++;
				first = atoi(cmd);

				InitNetMsg(&msg, buf, sizeof(buf));

				if (svcount != v->servercount)
				{	//looks like we changed map without them.
					SendServerData(qtv, v);
					return;
				}

				if (!qtv)
					SendList(qtv, first, ConnectionlessSoundList, svc_soundlist, &msg);
				else
					SendList(qtv, first, qtv->soundlist, svc_soundlist, &msg);
				SendBufferToViewer(v, msg.data, msg.cursize, true);
			}
			else if (!strncmp(buf, "prespawn", 8))
			{
				char skin[128];

				if (atoi(buf + 9) != v->servercount)
					SendServerData(qtv, v);	//we're old.
				else
				{
					int crc;
					int r;
					char *s;
					s = buf+9;
					while((*s >= '0' && *s <= '9') || *s == '-')
						s++;
					while(*s == ' ')
						s++;
					r = atoi(s);

					if (r == 0)
					{
						while((*s >= '0' && *s <= '9') || *s == '-')
							s++;
						while(*s == ' ')
							s++;
						crc = atoi(s);

						if (qtv && qtv->controller == v)
						{
							if (!qtv->bsp)
							{
								QW_PrintfToViewer(v, "Proxy was unable to check your map version\n");
								qtv->drop = true;
							}
							else if (crc != BSP_Checksum(qtv->bsp))
							{
								QW_PrintfToViewer(v, "Your map (%s) does not match the servers\n", qtv->modellist[1].name);
								qtv->drop = true;
							}
						}
					}

					InitNetMsg(&msg, buf, sizeof(buf));

					if (qtv)
					{
						r = Prespawn(qtv, v->netchan.message.cursize, &msg, r, v->thisplayer);
						SendBufferToViewer(v, msg.data, msg.cursize, true);
					}
					else
					{
						r = SendCurrentUserinfos(qtv, v->netchan.message.cursize, &msg, r, v->thisplayer);
						if (r > MAX_CLIENTS)
							r = -1;
						SendBufferToViewer(v, msg.data, msg.cursize, true);
					}

					if (r < 0)
						sprintf(skin, "%ccmd spawn\n", svc_stufftext);
					else
						sprintf(skin, "%ccmd prespawn %i %i\n", svc_stufftext, v->servercount, r);

					SendBufferToViewer(v, skin, strlen(skin)+1, true);
				}
			}
			else if (!strncmp(buf, "spawn", 5))
			{
				char skin[64];
				sprintf(skin, "%cskins\n", svc_stufftext);
				SendBufferToViewer(v, skin, strlen(skin)+1, true);
			}
			else if (!strncmp(buf, "begin", 5))
			{
				if (atoi(buf+6) != v->servercount)
					SendServerData(qtv, v);	//this is unfortunate!
				else
				{
					v->thinksitsconnected = true;
					if (qtv && qtv->ispaused)
					{
						char msgb[] = {svc_setpause, 1};
						SendBufferToViewer(v, msgb, sizeof(msgb), true);
					}
				}
			}
			else if (!strncmp(buf, "download", 8))
			{
				netmsg_t m;
				InitNetMsg(&m, buf, sizeof(buf));
				WriteByte(&m, svc_download);
				WriteShort(&m, -1);
				WriteByte(&m, 0);
				SendBufferToViewer(v, m.data, m.cursize, true);
			}
			else if (!strncmp(buf, "drop", 4))
				v->drop = true;
			else if (!strncmp(buf, "ison", 4))
			{
				viewer_t *other;
				if ((other = QW_IsOn(cluster, buf+5)))
				{
					if (!other->server)
						QW_PrintfToViewer(v, "%s is on the proxy, but not yet watching a game\n");
					else
						QW_PrintfToViewer(v, "%s is watching %s\n", buf+5, other->server->server);
				}
				else
					QW_PrintfToViewer(v, "%s is not on the proxy, sorry\n", buf+5);	//the apology is to make the alternatives distinct.
			}
			else if (!strncmp(buf, "ptrack ", 7))
				v->trackplayer = atoi(buf+7);
			else if (!strncmp(buf, "ptrack", 6))
				v->trackplayer = -1;
			else if (!strncmp(buf, "pings", 5))
			{
			}
			else if (!strncmp(buf, "say \"", 5))
				QTV_Say(cluster, qtv, v, buf+5, false);
			else if (!strncmp(buf, "say ", 4))
				QTV_Say(cluster, qtv, v, buf+4, false);

			else if (!strncmp(buf, "say_team \"", 10))
				QTV_Say(cluster, qtv, v, buf+10, true);
			else if (!strncmp(buf, "say_team ", 9))
				QTV_Say(cluster, qtv, v, buf+9, true);

			else if (!strncmp(buf, "servers", 7))
			{
				QW_SetMenu(v, MENU_SERVERS);
			}
			else if (!strncmp(buf, "reset", 5))
			{
				QW_SetViewersServer(v, NULL);
				QW_SetMenu(v, MENU_SERVERS);
			}
			else if (!strncmp(buf, "admin", 5))
			{
				if (!*cluster->password)
				{
					if (Netchan_IsLocal(v->netchan.remote_address))
					{
						Sys_Printf(cluster, "Local player %s logs in as admin\n", v->name);
						QW_SetMenu(v, MENU_ADMIN);
						v->isadmin = true;
					}
					else
						QW_PrintfToViewer(v, "There is no admin password set\nYou may not log in.\n");
				}
				else if (v->isadmin)
					QW_SetMenu(v, MENU_ADMIN);
				else
				{
					strcpy(v->expectcommand, "admin");
					QW_StuffcmdToViewer(v, "echo Please enter the rcon password\nmessagemode\n");
				}
			}

			else if (!strncmp(buf, "setinfo", 5))
			{
				#define TOKENIZE_PUNCTUATION ""
				#define MAX_ARGS 3
				#define ARG_LEN 256

				int i;
				char arg[MAX_ARGS][ARG_LEN];
				char *argptrs[MAX_ARGS];
				char *command = buf;

				for (i = 0; i < MAX_ARGS; i++)
				{
					command = COM_ParseToken(command, arg[i], ARG_LEN, TOKENIZE_PUNCTUATION);
					argptrs[i] = arg[i];
				}

				Info_SetValueForStarKey(v->userinfo, arg[1], arg[2], sizeof(v->userinfo));
				Info_ValueForKey(v->userinfo, "name", v->name, sizeof(v->name));

				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
			}

			else if (!qtv)
			{
				//all the other things need an active server.
				QW_PrintfToViewer(v, "Choose a server first\n");
			}
			else if (!strncmp(buf, "serverinfo", 5))
			{
				char *key, *value, *end;
				int len;
				netmsg_t m;
				InitNetMsg(&m, buf, sizeof(buf));
				WriteByte(&m, svc_print);
				WriteByte(&m, 2);
				end = qtv->serverinfo;
				for(;;)
				{
					if (!*end)
						break;
					key = end;
					value = strchr(key+1, '\\');
					if (!value)
						break;
					end = strchr(value+1, '\\');
					if (!end)
						end = value+strlen(value);

					len = value-key;

					key++;
					while(*key != '\\' && *key)
						WriteByte(&m, *key++);

					for (; len < 20; len++)
						WriteByte(&m, ' ');

					value++;
					while(*value != '\\' && *value)
						WriteByte(&m, *value++);
					WriteByte(&m, '\n');
				}
				WriteByte(&m, 0);

//				WriteString(&m, qtv->serverinfo);
				SendBufferToViewer(v, m.data, m.cursize, true);
			}
			else
			{
				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
				else
					Sys_Printf(cluster, "Client sent unknown string command: %s\n", buf);
			}

			break;

		case clc_move:
			v->lost = ReadByte(m);
			ReadByte(m);
			ReadDeltaUsercmd(m, &nullcmd, &v->ucmds[0]);
			ReadDeltaUsercmd(m, &v->ucmds[0], &v->ucmds[1]);
			ReadDeltaUsercmd(m, &v->ucmds[1], &v->ucmds[2]);
/*
			if (v->server && v->server->controller)
			{
				v->
			}
*/
/*			if (v->menunum)
			{
				if (newcmd.buttons&1 && !(oldcmd.buttons&1))
					Menu_Enter(cluster, v, 0);
				if (newcmd.buttons&2 && !(oldcmd.buttons&2))
					Menu_Enter(cluster, v, 1);
				if (newcmd.sidemove && !oldcmd.sidemove)
					Menu_Enter(cluster, v, newcmd.sidemove<0);

				if (newcmd.forwardmove && !oldcmd.forwardmove)
				{	//they pressed the button...
					if (newcmd.forwardmove < 0)
					{
						v->menuop+=1;
					}
					else
					{
						v->menuop-=1;
					}
				}
			}
*/
			PMove(v, &v->ucmds[2]);
			break;
		case clc_tmove:
			v->origin[0] = ((signed short)ReadShort(m))/8.0f;
			v->origin[1] = ((signed short)ReadShort(m))/8.0f;
			v->origin[2] = ((signed short)ReadShort(m))/8.0f;
			break;

		case clc_upload:
			v->drop = true;
			return;

		default:
			v->drop = true;
			return;
		}
	}
}

void Menu_Enter(cluster_t *cluster, viewer_t *viewer, int buttonnum)
{
	//build a possible message, even though it'll probably not be sent

	sv_t *sv;
	int i, min;

	switch(viewer->menunum)
	{
	default:
		break;

	case MENU_ADMINSERVER:
		if (viewer->server)
		{
			i = 0;
			sv = viewer->server;
			if (i++ == viewer->menuop)
			{	//mvd port
				QW_StuffcmdToViewer(viewer, "echo Please enter a new tcp port number\nmessagemode\n");
				strcpy(viewer->expectcommand, "setmvdport");
			}
			if (i++ == viewer->menuop)
			{	//disconnect
				QTV_Shutdown(viewer->server);
			}
			if (i++ == viewer->menuop)
			{	//back
				QW_SetMenu(viewer, MENU_ADMIN);
			}
			break;
		}
		//fallthrough
	case MENU_SERVERS:
		if (!cluster->servers)
		{
			QW_StuffcmdToViewer(viewer, "echo Please enter a server ip\nmessagemode\n");
			strcpy(viewer->expectcommand, "insecadddemo");
		}
		else
		{
			if (viewer->menuop < 0)
				viewer->menuop = 0;
			i = 0;
			min = viewer->menuop - 10;
			if (min < 0)
				min = 0;
			for (sv = cluster->servers; sv && i<min; sv = sv->next, i++)
			{//skip over the early connections.
			}
			min+=20;
			for (; sv && i < min; sv = sv->next, i++)
			{
				if (i == viewer->menuop)
				{
					/*if (sv->parsingconnectiondata || !sv->modellist[1].name[0])
					{
						QW_PrintfToViewer(viewer, "But that stream isn't connected\n");
					}
					else*/
					{
						QW_SetViewersServer(viewer, sv);
						QW_SetMenu(viewer, MENU_NONE);
						viewer->thinksitsconnected = false;
					}
					break;
				}
			}
		}
		break;
	case MENU_ADMIN:
		i = 0;
		if (i++ == viewer->menuop)
		{	//connection stuff
			QW_SetMenu(viewer, MENU_ADMINSERVER);
		}
		if (i++ == viewer->menuop)
		{	//qw port
			QW_StuffcmdToViewer(viewer, "echo You will need to reconnect\n");
			cluster->qwlistenportnum += buttonnum?-1:1;
		}
		if (i++ == viewer->menuop)
		{	//hostname
			strcpy(viewer->expectcommand, "hostname");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new hostname\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//master
			strcpy(viewer->expectcommand, "master");
			QW_StuffcmdToViewer(viewer, "echo Please enter the master dns or ip\necho Enter '.' for masterless mode\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//password
			strcpy(viewer->expectcommand, "password");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new rcon password\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//add server
			strcpy(viewer->expectcommand, "messagemode");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new qtv server dns or ip\naddserver\n");
		}
		if (i++ == viewer->menuop)
		{	//add demo
			strcpy(viewer->expectcommand, "adddemo");
			QW_StuffcmdToViewer(viewer, "echo Please enter the name of the demo to play\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//choke
			cluster->chokeonnotupdated ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//late forwarding
			cluster->lateforward ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//no talking
			cluster->notalking ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//nobsp
			cluster->nobsp ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//back
			QW_SetMenu(viewer, MENU_NONE);
		}


		break;
	}
}

void Menu_Draw(cluster_t *cluster, viewer_t *viewer)
{
	char buffer[2048];
	char str[64];
	sv_t *sv;
	int i, min;
	unsigned char *s;

	netmsg_t m;
	InitNetMsg(&m, buffer, sizeof(buffer));

	WriteByte(&m, svc_centerprint);

	WriteString2(&m, "FTEQTV\n");
	if (strcmp(cluster->hostname, DEFAULT_HOSTNAME))
		WriteString2(&m, cluster->hostname);

	switch(viewer->menunum)
	{
	default:
		WriteString2(&m, "bad menu");
		break;

	case 3:	//per-connection options
		if (viewer->server)
		{
			sv = viewer->server;
			WriteString2(&m, "\n\nConnection Admin\n");
			WriteString2(&m, sv->hostname);
			if (sv->file)
				WriteString2(&m, " (demo)");
			WriteString2(&m, "\n\n");

			if (viewer->menuop < 0)
				viewer->menuop = 0;

			i = 0;
			WriteString2(&m, "            port");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			if (sv->listenmvd == INVALID_SOCKET)
				sprintf(str, "!%-19i", sv->tcplistenportnum);
			else
				sprintf(str, "%-20i", sv->tcplistenportnum);
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			WriteString2(&m, "      disconnect");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			sprintf(str, "%-20s", "...");
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			WriteString2(&m, "            back");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			sprintf(str, "%-20s", "...");
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			if (viewer->menuop >= i)
				viewer->menuop = i - 1;

			break;
		}
		//fallthrough
	case 1:	//connections list

		WriteString2(&m, "\n\nServers\n\n");

		if (!cluster->servers)
		{
			WriteString2(&m, "No active connections");
		}
		else
		{
			if (viewer->menuop < 0)
				viewer->menuop = 0;
			i = 0;
			min = viewer->menuop - 10;
			if (min < 0)
				min = 0;
			for (sv = cluster->servers; sv && i<min; sv = sv->next, i++)
			{//skip over the early connections.
			}
			min+=20;
			for (; sv && i < min; sv = sv->next, i++)
			{
				Info_ValueForKey(sv->serverinfo, "hostname", str, sizeof(str));
				if (sv->parsingconnectiondata || !sv->modellist[1].name[0])
					snprintf(str, sizeof(str), "%s", sv->server);

				if (i == viewer->menuop)
					for (s = (unsigned char *)str; *s; s++)
					{
						if ((unsigned)*s >= ' ')
							*s = 128 | (*s&~128);
					}
				WriteString2(&m, str);
				WriteString2(&m, "\n");
			}
		}
		break;

	case 2:	//admin menu

		WriteString2(&m, "\n\nCluster Admin\n\n");

		if (viewer->menuop < 0)
			viewer->menuop = 0;
		i = 0;

		WriteString2(&m, " this connection");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "            port");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20i", cluster->qwlistenportnum);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        hostname");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->hostname);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "          master");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->master);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        password");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "      add server");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        add demo");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "           choke");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->chokeonnotupdated?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "delay forwarding");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->lateforward?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "         talking");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->notalking?"no":"yes");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "           nobsp");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->nobsp?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "            back");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		if (viewer->menuop >= i)
			viewer->menuop = i - 1;
		break;
	}


	WriteByte(&m, 0);
	SendBufferToViewer(viewer, m.data, m.cursize, true);
}

static const char dropcmd[] = {svc_stufftext, 'd', 'i', 's', 'c', 'o', 'n', 'n', 'e', 'c', 't', '\n', '\0'};

void QW_FreeViewer(cluster_t *cluster, viewer_t *viewer)
{
	int i;
	//note: unlink them yourself.

	Sys_Printf(cluster, "Dropping viewer %s\n", viewer->name);

	//spam them thrice, then forget about them
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);

	for (i = 0; i < MAX_BACK_BUFFERS; i++)
	{
		if (viewer->backbuf[i].data)
			free(viewer->backbuf[i].data);
	}

	if (viewer->server->controller == viewer)
		viewer->server->drop = true;
	if (viewer->server)
		viewer->server->numviewers--;

	free(viewer);

	cluster->numviewers--;
}

void QW_UpdateUDPStuff(cluster_t *cluster)
{
	char buffer[MAX_MSGLEN*2];
	netadr_t from;
	int fromsize = sizeof(from);
	int read;
	int qport;
	netmsg_t m;

	sv_t *useserver;
	viewer_t *v, *f;

	if (*cluster->master && (cluster->curtime > cluster->mastersendtime || cluster->mastersendtime > cluster->curtime + 4*1000*60))	//urm... time wrapped?
	{
		if (NET_StringToAddr(cluster->master, &from, 27000))
		{
			sprintf(buffer, "a\n%i\n0\n", cluster->mastersequence++);	//fill buffer with a heartbeat
//why is there no \xff\xff\xff\xff ?..
			NET_SendPacket(cluster, cluster->qwdsocket, strlen(buffer), buffer, from);
		}
		else
			Sys_Printf(cluster, "Cannot resolve master %s\n", cluster->master);

		cluster->mastersendtime = cluster->curtime + 3*1000*60;	//3 minuites.
	}

	m.data = buffer;
	m.cursize = 0;
	m.maxsize = MAX_MSGLEN;
	m.readpos = 0;

	for (;;)
	{
		read = recvfrom(cluster->qwdsocket, buffer, sizeof(buffer), 0, (struct sockaddr*)from, &fromsize);

		if (read <= 5)	//otherwise it's a runt or bad.
		{
			if (read < 0)	//it's bad.
				break;
			continue;
		}

		m.data = buffer;
		m.cursize = read;
		m.maxsize = MAX_MSGLEN;
		m.readpos = 0;

		if (*(int*)buffer == -1)
		{	//connectionless message
			ConnectionlessPacket(cluster, &from, &m);
			continue;
		}

		//read the qport
		ReadLong(&m);
		ReadLong(&m);
		qport = ReadShort(&m);

		for (v = cluster->viewers; v; v = v->next)
		{
			if (Net_CompareAddress(&v->netchan.remote_address, &from, v->netchan.qport, qport))
			{
				if (Netchan_Process(&v->netchan, &m))
				{
					useserver = v->server;
					if (useserver && useserver->parsingconnectiondata)
						useserver = NULL;

					v->netchan.outgoing_sequence = v->netchan.incoming_sequence;	//compensate for client->server packetloss.
					if (v->server && v->server->controller == v)
					{
//						v->maysend = true;
						v->server->maysend =  true;
//						v->server->netchan.outgoing_sequence = v->netchan.incoming_sequence;
					}
					else
					{
						if (!v->server)
							v->maysend = true;
						else if (!v->chokeme || !cluster->chokeonnotupdated)
						{
							v->maysend = true;
							v->chokeme = cluster->chokeonnotupdated;
						}
					}

					ParseQWC(cluster, useserver, v, &m);

					if (v->server && v->server->controller == v)
					{
						QTV_Run(v->server);
					}
				}
				break;
			}
		}
	}


	if (cluster->viewers && cluster->viewers->drop)
	{
		Sys_Printf(cluster, "Dropping client\n");
		f = cluster->viewers;
		cluster->viewers = f->next;

		QW_FreeViewer(cluster, f);
	}

	for (v = cluster->viewers; v; v = v->next)
	{
		if (v->next && v->next->drop)
		{	//free the next/
			Sys_Printf(cluster, "Dropping client\n");
			f = v->next;
			v->next = f->next;

			QW_FreeViewer(cluster, f);
		}

		if (v->maysend)	//don't send incompleate connection data.
		{
			useserver = v->server;
			if (useserver && useserver->parsingconnectiondata)
				useserver = NULL;

			v->maysend = false;
			m.cursize = 0;
			if (v->thinksitsconnected)
			{
				SendPlayerStates(useserver, v, &m);
				UpdateStats(useserver, v);

				if (v->menunum)
					Menu_Draw(cluster, v);
				else if (v->server && v->server->parsingconnectiondata)
				{
					WriteByte(&m, svc_centerprint);
					WriteString(&m, v->server->status);
				}
			}
			Netchan_Transmit(cluster, &v->netchan, m.cursize, m.data);

			if (!v->netchan.message.cursize && v->backbuffered)
			{//shift the backbuffers around
				memcpy(v->netchan.message.data, v->backbuf[0].data,  v->backbuf[0].cursize);
				v->netchan.message.cursize = v->backbuf[0].cursize;
				for (read = 0; read < v->backbuffered; read++)
				{
					if (read == v->backbuffered-1)
					{
						v->backbuf[read].cursize = 0;
					}
					else
					{
						memcpy(v->backbuf[read].data, v->backbuf[read+1].data,  v->backbuf[read+1].cursize);
						v->backbuf[read].cursize = v->backbuf[read+1].cursize;
					}
				}
			}
		}
	}
}
