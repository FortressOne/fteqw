#include "qwsvdef.h"

#ifndef CLIENTONLY

#if defined(_WIN32) && !defined(MINGW)
#define inline //_inline	//fix for stupid VC
#endif

#ifdef SVRANKING

typedef struct {
	int usedslots;
	int leader;
	int freeslot;
} rankfileheader_t;

//endian
#define swaplong	LittleLong
#define swapfloat	LittleFloat

rankfileheader_t rankfileheader;
FILE *rankfile;

cvar_t rank_autoadd = {"rank_autoadd", "1"};
cvar_t rank_needlogin = {"rank_needlogin", "0"};
cvar_t rank_filename = {"rank_filename", ""};
char rank_cvargroup[] = "server rankings";

void inline READ_PLAYERSTATS(int x, rankstats_t *os)
{	
	int i;

	fseek(rankfile, sizeof(rankfileheader_t)+sizeof(rankheader_t)+((x-1)*sizeof(rankinfo_t)), SEEK_SET);
	fread(os, sizeof(rankstats_t), 1, rankfile);

	os->kills = swaplong(os->kills);
	os->deaths = swaplong(os->deaths);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		os->parm[i] = swapfloat(os->parm[i]);
	os->timeonserver = swapfloat(os->timeonserver);
//	os->flags1 = (os->flags1);
//	os->trustlevel = (os->trustlevel);	
//	os->pad2 = (os->pad2);	
//	os->pad3 = (os->pad3);
}

void inline WRITE_PLAYERSTATS(int x, rankstats_t *os)
{
	rankstats_t ns;
	int i;

	fseek(rankfile, sizeof(rankfileheader_t)+sizeof(rankheader_t)+((x-1)*sizeof(rankinfo_t)), SEEK_SET);

	ns.kills = swaplong(os->kills);
	ns.deaths = swaplong(os->deaths);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		ns.parm[i] = swapfloat(os->parm[i]);
	ns.timeonserver = swapfloat(os->timeonserver);
	ns.flags1 = (os->flags1);
	ns.trustlevel = (os->trustlevel);	
	ns.pad2 = (os->pad2);	
	ns.pad3 = (os->pad3);

	fwrite(&ns, sizeof(rankstats_t), 1, rankfile);
}

void inline READ_PLAYERHEADER(int x, rankheader_t *oh)
{	
	fseek(rankfile, sizeof(rankfileheader_t)+((x-1)*sizeof(rankinfo_t)), SEEK_SET);

	fread(oh, sizeof(rankheader_t), 1, rankfile);

	oh->prev = swaplong(oh->prev);		//score is held for convineance.
	oh->next = swaplong(oh->next);
//	strcpy(oh->name, oh->name);
	oh->pwd = swaplong(oh->pwd);
	oh->score = swapfloat(oh->score);	
}

void inline WRITE_PLAYERHEADER(int x, rankheader_t *oh)
{
	rankheader_t nh;

	fseek(rankfile, sizeof(rankfileheader_t)+((x-1)*sizeof(rankinfo_t)), SEEK_SET);

	nh.prev = swaplong(oh->prev);		//score is held for convineance.
	nh.next = swaplong(oh->next);
	Q_strncpyz(nh.name, oh->name, sizeof(nh.name));
	nh.pwd = swaplong(oh->pwd);
	nh.score = swapfloat(oh->score);

	fwrite(&nh, sizeof(rankheader_t), 1, rankfile);
}

void inline READ_PLAYERINFO(int x, rankinfo_t *inf)
{
	READ_PLAYERHEADER(x, &inf->h);
	READ_PLAYERSTATS(x, &inf->s);
}

void inline WRITEHEADER(void)
{
	rankfileheader_t nh;

	nh.usedslots	= swaplong(rankfileheader.usedslots);
	nh.leader		= swaplong(rankfileheader.leader);
	nh.freeslot		= swaplong(rankfileheader.freeslot);

	fseek(rankfile, 0, SEEK_SET);
	fwrite(&nh, sizeof(rankfileheader_t), 1, rankfile);
}
//#define WRITEHEADER() 	{fseek(rankfile, 0, SEEK_SET);fwrite(&rankfileheader, sizeof(rankfileheader_t), 1, rankfile);}

#define NAMECMP(saved, against)	Q_strncasecmp(saved, against, 31)

qboolean Rank_OpenRankings(void)
{
	if (!rankfile)
	{
		if (!*rank_filename.string)
		{	
			return false;
		}

		if (strstr(rank_filename.string, ".."))
			return false;

		rankfile = fopen(va("%s/%s", com_gamedir, rank_filename.string), "r+b");
		if (!rankfile)	//hmm... try creating
			rankfile = fopen(va("%s/%s", com_gamedir, rank_filename.string), "w+b");
		if (!rankfile)
			return false;	//couldn't open file.

		memset(&rankfileheader, 0, sizeof(rankfileheader));

		fseek(rankfile, 0, SEEK_SET);
		fread(&rankfileheader, sizeof(rankfileheader_t), 1, rankfile);

		rankfileheader.usedslots	= swaplong(rankfileheader.usedslots);
		rankfileheader.leader		= swaplong(rankfileheader.leader);
		rankfileheader.freeslot		= swaplong(rankfileheader.freeslot);

		return true;	//success.
	}
	return true;	//already open
}

void LINKUN(int id)
{
	int idnext, idprev;
	rankheader_t hnext, hprev, info;

	READ_PLAYERHEADER(id, &info);

	idnext = info.next;
	if (idnext)
		READ_PLAYERHEADER(idnext, &hnext);
	idprev = info.prev;
	if (idprev)
		READ_PLAYERHEADER(idprev, &hprev);

	if (idnext)
	{
		hnext.prev = idprev;
		WRITE_PLAYERHEADER(idnext, &hnext);
	}
	if (idprev)
	{
		hprev.next = idnext;	
		WRITE_PLAYERHEADER(idprev, &hprev);
	}
	else if (rankfileheader.leader == id)	//ensure header is accurate
	{
		rankfileheader.leader = info.next;
		WRITEHEADER();
	}
	else if (rankfileheader.freeslot == id)
	{		
		rankfileheader.freeslot = info.next;
		WRITEHEADER();
	}

	info.next = 0;
	info.prev = 0;
	WRITE_PLAYERHEADER(id, &info);
}

void LINKBEFORE(int bef, int id, rankheader_t *info)
{
	int idnext, idprev;
	rankheader_t hnext, hprev;

	if (!bef)
		Sys_Error("Cannot link before no entry\n");

	idnext = bef;
	READ_PLAYERHEADER(idnext, &hnext);
	idprev = hnext.prev;
	if (idprev)
		READ_PLAYERHEADER(idprev, &hprev);

//now we know the before and after entries.

	hnext.prev = id;	
	WRITE_PLAYERHEADER(idnext, &hnext);

	if (idprev)
	{
		hprev.next = id;
		WRITE_PLAYERHEADER(idprev, &hprev);
	}
	else if (rankfileheader.leader == bef)
	{
		rankfileheader.leader = id;
		WRITEHEADER();
	}
	else if (rankfileheader.freeslot == bef)
	{
		rankfileheader.freeslot = id;
		WRITEHEADER();
	}
	info->next = idnext;
	info->prev = idprev;
	WRITE_PLAYERHEADER(id, info);
}

void LINKAFTER(int aft, int id, rankheader_t *info)
{
	int idnext, idprev;
	rankheader_t hnext, hprev;

	idprev = aft;
	if (idprev)
	{
		READ_PLAYERHEADER(idprev, &hprev);
		idnext = hprev.next;		
	}
	else
		idnext = rankfileheader.leader;

	if (idnext)
		READ_PLAYERHEADER(idnext, &hnext);

//now we know the before and after entries.

	if (idnext)
	{
		hnext.prev = id;
		WRITE_PLAYERHEADER(idnext, &hnext);
	}

	if (idprev)
	{
		hprev.next = id;
		WRITE_PLAYERHEADER(idprev, &hprev);
	}
	else if (rankfileheader.leader == idnext)
	{
		rankfileheader.leader = id;
		WRITEHEADER();
	}
	else if (rankfileheader.freeslot == idnext)
	{
		rankfileheader.freeslot = id;
		WRITEHEADER();
	}
	info->next = idnext;
	info->prev = idprev;
	WRITE_PLAYERHEADER(id, info);
}

rankstats_t *Rank_GetPlayerStats(int id, rankstats_t *buffer)	//returns the players persistant stats.
{
	if (!Rank_OpenRankings())
		return NULL;
	if (!id)
	{
		Con_Printf("WARNING: Rank_GetPlayerStats with id 0\n");
		memset(buffer, 0, sizeof(rankstats_t));
		return NULL;
	}
	READ_PLAYERSTATS(id, buffer);

	return buffer;
}
rankinfo_t *Rank_GetPlayerInfo(int id, rankinfo_t *buffer)		//return stats + rankings.
{
	if (!id)
	{
		Con_Printf("WARNING: Rank_GetPlayerInfo with id 0\n");
		memset(buffer, 0, sizeof(rankinfo_t));
		return NULL;
	}

	if (!Rank_OpenRankings())
		return NULL;

	READ_PLAYERINFO(id, buffer);

	return buffer;
}
void Rank_SetPlayerStats(int id, rankstats_t *stats)
{
	//rewrite to seek in a proper direction.
	int nid;
	rankheader_t rh, nh;

	if (!id)
	{
		Con_Printf("WARNING: Rank_SetPlayerStats with id 0\n");		
		return;
	}

	//write
	WRITE_PLAYERSTATS(id, stats);

	//now re-sort.
	READ_PLAYERHEADER(id, &nh);
	nh.score = (stats->kills+1)/((float)stats->deaths+1);
	//WRITE_PLAYERHEADER(id, &nh); //saved on link.
	
	LINKUN(id);
	
	nid = rankfileheader.leader;
	if (!nid)	//Hmm. First player!
	{
		LINKAFTER(0, id, &nh);
		fflush(rankfile);
		return;
	}
	while(nid)
	{
		READ_PLAYERHEADER(nid, &rh);
		if (rh.score < nh.score)
		{
			LINKAFTER(rh.prev, id, &nh);
			//LINKBEFORE(nid, id, &nh);	//we are doing better than this guy.
			fflush(rankfile);
			return;
		}
		if (!rh.next)
		{
			LINKAFTER(nid, id, &nh);	//Bum. We got to the end of the list and we are the WORST player!
			fflush(rankfile);
			return;
		}
		nid = rh.next;
	}	
}

int Rank_GetPlayerID(char *name, int pwd, qboolean allowadd)
{
	rankstats_t rs;
	rankheader_t rh;
	int id;

	if (!Rank_OpenRankings())
		return 0;

	id = rankfileheader.leader;	//assumtion. A leader is more likly to be logging in than a begginer.
	while(id)
	{
		READ_PLAYERHEADER(id, &rh);
		if (!NAMECMP(rh.name, name))
		{
			if (rh.pwd == pwd || !rh.pwd)
				return id;
			return 0;
		}
		id = rh.next;
	}

	if (!rank_autoadd.value || !allowadd)
		return 0;

	id = rankfileheader.freeslot;
	if (id)
	{	
		READ_PLAYERHEADER(id, &rh);
		rankfileheader.freeslot = rh.next;
		WRITEHEADER();

		memset(&rh, 0, sizeof(rh));
		Q_strncpyz(rh.name, name, sizeof(rh.name));
		rh.pwd = pwd;
		rh.prev = 0;
		rh.next = rankfileheader.usedslots;		
		rankfileheader.usedslots = id;
		WRITEHEADER();

		WRITE_PLAYERHEADER(id, &rh);

		memset(&rs, 0, sizeof(rs));
		rs.trustlevel = 1;
		Rank_SetPlayerStats(id, &rs);

		fflush(rankfile);
		return id;
	}

	id = ++rankfileheader.usedslots;
	WRITEHEADER();
	memset(&rh, 0, sizeof(rh));
	Q_strncpyz(rh.name, name, sizeof(rh.name));
	rh.prev = 0;
	rh.pwd = pwd;
	rh.next = 0;
	WRITE_PLAYERHEADER(id, &rh);

	memset(&rs, 0, sizeof(rs));
	rs.trustlevel = 1;
	WRITE_PLAYERSTATS(id, &rs);

	Rank_SetPlayerStats(id, &rs);

	fflush(rankfile);
	return id;
}

void Rank_AddUser_f (void)
{
	rankstats_t rs;
	rankheader_t rh;
	int id;

	char *name = Cmd_Argv(1);
	int pwd = atoi(Cmd_Argv(2));
	int userlevel = atoi(Cmd_Argv(3));

	if (Cmd_Argc() < 2)
	{
		Con_Printf("%s: <name> [pwd] [rights]\n");
		return;
	}
	//2
	if (Cmd_Argc() >= 4)
	{
		if (userlevel >= Cmd_ExecLevel)
		{
			Con_Printf("You cannot add a user of equal or higher rank.\n");
			return;
		}
		else if (userlevel < RESTRICT_MIN)
			userlevel = RESTRICT_MIN;
	}
	if (Cmd_Argc() > 4)
	{
		Con_Printf("Too many arguments\n");
		return;
	}

	SV_FixupName(name, name);

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return;
	}

	id = rankfileheader.leader;
	while(id)
	{
		READ_PLAYERHEADER(id, &rh);
		if (!NAMECMP(rh.name, name))
		{
			Con_Printf("User %s already exists\n", name);
			return;
		}
		id = rh.next;
	}

	id = rankfileheader.freeslot;
	if (id)
	{	
		READ_PLAYERHEADER(id, &rh);
		rankfileheader.freeslot = rh.next;
		WRITEHEADER();

		memset(&rh, 0, sizeof(rh));
		Q_strncpyz(rh.name, name, sizeof(rh.name));
		rh.pwd = pwd;		
		rh.prev = 0;
		rh.next = rankfileheader.usedslots;		
		rankfileheader.usedslots = id;
		WRITEHEADER();

		WRITE_PLAYERHEADER(id, &rh);

		memset(&rs, 0, sizeof(rs));
		rs.trustlevel = userlevel;
		Rank_SetPlayerStats(id, &rs);

		fflush(rankfile);
		return;
	}

	id = ++rankfileheader.usedslots;
	WRITEHEADER();
	memset(&rh, 0, sizeof(rh));
	Q_strncpyz(rh.name, name, sizeof(rh.name));
	rh.prev = 0;
	rh.pwd = pwd;
	rh.next = 0;
	WRITE_PLAYERHEADER(id, &rh);

	memset(&rs, 0, sizeof(rs));
	rs.trustlevel = userlevel;
	WRITE_PLAYERSTATS(id, &rs);

	Rank_SetPlayerStats(id, &rs);

	fflush(rankfile);
}

void Rank_SetPass_f (void)
{
	rankheader_t rh;
	char *name = Cmd_Argv(1);
	int newpass = atoi(Cmd_Argv(2));

	int id;

	if (Cmd_Argc() != 3)
	{
		Con_Printf("setpass <name> <newpass>\n");
		return;
	}

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return;
	}

	SV_FixupName(name, name);

	id = rankfileheader.leader;
	while(id)
	{
		READ_PLAYERHEADER(id, &rh);
		if (!NAMECMP(rh.name, name))
		{
			Con_Printf("Changing passcode of user %s.\n", rh.name);
			rh.pwd = newpass;
			WRITE_PLAYERHEADER(id, &rh);
			return;
		}
		id = rh.next;
	}
}

int Rank_GetPass (char *name)
{
	rankheader_t rh;

	int id;

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return 0;
	}

	SV_FixupName(name, name);

	id = rankfileheader.leader;
	while(id)
	{
		READ_PLAYERHEADER(id, &rh);
		if (!NAMECMP(rh.name, name))
		{
			return rh.pwd;
		}
		id = rh.next;
	}
	return 0;
}


int Rank_Enumerate (unsigned int first, unsigned int last, void (*callback) (const rankinfo_t *ri))	//leader first.
{
	rankinfo_t ri;
	int id;
	int num;

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return 0;
	}

	id = rankfileheader.leader;	//start at the leaders
	num=1;
	while(id)
	{
		READ_PLAYERINFO(id, &ri);

		if (num >= last)
			return num - first;
		if (num >= first)
			callback(&ri);

		num++;
		id = ri.h.next;
	}
	return num - first;
}

void Rank_RankingList_f (void)
{	
	rankinfo_t ri;
	int id;
	int num;

	FILE *outfile;

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return;
	}

	outfile = fopen("list.txt", "wb");

	fprintf(outfile, "%5s: %32s, %5s %5s\r\n", "", "Name", "Kills", "Deaths");

	id = rankfileheader.leader;	//start at the leaders
	num=1;
	while(id)
	{
		READ_PLAYERINFO(id, &ri);

		fprintf(outfile, "%5i: %32s, %5i %5i\r\n", num, ri.h.name, ri.s.kills, ri.s.deaths);

		num++;
		id = ri.h.next;
	}

	fclose(outfile);
}

void Rank_Remove_f (void)
{
	rankinfo_t ri;
	int id;
	int num;
	int remnum;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("Removes a ranking entry.\nUse ranklist to find the entry number.");
		return;
	}
	remnum = atoi(Cmd_Argv(1));

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return;
	}

	id = rankfileheader.leader;	//start at the leaders
	num=1;
	while(id)
	{
		READ_PLAYERINFO(id, &ri);

		if (num == remnum)
		{			
			LINKUN(id);
			ri.h.next = rankfileheader.freeslot;
			ri.h.prev = 0;
			rankfileheader.freeslot = id;
			WRITE_PLAYERHEADER(id, &ri.h);
			WRITEHEADER();
			fflush(rankfile);

			Con_Printf("Client %s removed from rankings\n", ri.h.name);
			return;
		}
		num++;
		id = ri.h.next;
	}

	Con_Printf("Client %i not found\n", remnum);
}

void Rank_ListTop10_f (void)
{	
	rankinfo_t ri;
	int id;
	int num;
	extern redirect_t	sv_redirected;

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return;
	}

	id = rankfileheader.leader;	//start at the leaders
	num=1;
	while(id)
	{
		READ_PLAYERINFO(id, &ri);

		if (sv_redirected)
			Con_Printf("%2i: %5i %5i %s\n", num, ri.s.kills, ri.s.deaths, ri.h.name);
		else
			Con_Printf("%2i: %32s, %5i %5i\n", num, ri.h.name, ri.s.kills, ri.s.deaths);
		if (num >= 10)
			break;

		num++;
		id = ri.h.next;
	}
	if (num < 10)
		Con_Printf("END\n");
}

void Rank_Find_f (void)
{	
	rankinfo_t ri;
	int id;

	char *match = Cmd_Argv(1);

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return;
	}

	id = rankfileheader.leader;	//start at the leaders

	while(id)
	{
		READ_PLAYERINFO(id, &ri);

		if (strstr(ri.h.name, match))
		{
			Con_Printf("%i %s\n", id, ri.h.name);
		}

		id = ri.h.next;
	}
}

void Rank_Refresh_f(void)
{
	int i;
	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return;
	}

	for (i=0, host_client = svs.clients ; i<MAX_CLIENTS ; i++, host_client++)
	{
		if (host_client->state != cs_spawned)
			continue;

		if (host_client->rankid)
		{
			rankstats_t rs;
			Rank_GetPlayerStats(host_client->rankid, &rs);
			rs.timeonserver += realtime - host_client->stats_started;
			host_client->stats_started = realtime;
			rs.kills += host_client->kills;
			rs.deaths += host_client->deaths;
			host_client->kills=0;
			host_client->deaths=0;
			Rank_SetPlayerStats(host_client->rankid, &rs);
		}
	}
	if (rankfile)
	{
		fclose(rankfile);
		rankfile = NULL;
	}
}

void Rank_RCon_f(void)
{
	int gofor, num, id;
	int newlevel;
	rankstats_t rs;
	rankinfo_t ri;

	if (!Rank_OpenRankings())
	{
		Con_Printf("Failed to open rankings file.\n");
		return;
	}

	gofor = atoi(Cmd_Argv(1));
	newlevel = atoi(Cmd_Argv(2));
	if (newlevel >= Cmd_ExecLevel)
	{
		Con_Printf("You cannot promote a user to the same level as you\n");
		return;
	}
	else if (newlevel < RESTRICT_MIN)
		newlevel = RESTRICT_MIN;

	//get user id
	id = rankfileheader.leader;	//start at the leaders
	num = 1;
	while(id)
	{
		READ_PLAYERINFO(id, &ri);

		if (num == gofor)
		{	
			//save new level
			Rank_GetPlayerStats(id, &rs);

			if (rs.trustlevel >= Cmd_ExecLevel)
			{
				Con_Printf("You cannot demote a higher or equal user.\n");
				return;
			}
			rs.trustlevel = newlevel;
			Rank_SetPlayerStats(id, &rs);

			if (!ri.h.pwd && newlevel > 1)
				Con_Printf("WARNING: user has no password set\n");

			fflush(rankfile);
			return;
		}

		num++;
		id = ri.h.next;
	}
	Con_Printf("Couldn't find ranked user %i\n", gofor);
}


void Rank_RegisterCommands(void)
{
	Cmd_AddCommand("ranklist", Rank_RankingList_f);
	Cmd_AddCommand("ranktopten", Rank_ListTop10_f);
	Cmd_AddCommand("rankfind", Rank_Find_f);
	Cmd_AddCommand("rankremove", Rank_Remove_f);
	Cmd_AddCommand("rankrefresh", Rank_Refresh_f);

	Cmd_AddCommand("rankrconlevel", Rank_RCon_f);

	Cmd_AddCommand("rankadd", Rank_AddUser_f);
	Cmd_AddCommand("adduser", Rank_AddUser_f);
	Cmd_AddCommand("setpass", Rank_SetPass_f);	

	Cvar_Register(&rank_autoadd, rank_cvargroup);
	Cvar_Register(&rank_needlogin, rank_cvargroup);
	Cvar_Register(&rank_filename, rank_cvargroup);
}
void Rank_Flush (void)	//new game dir?
{
	if (rankfile)
	{
		Rank_Refresh_f();
		if (!rankfile)
			return;
		fclose(rankfile);
		rankfile=NULL;
	}
}
#endif
#endif
