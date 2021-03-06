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
#include "quakedef.h"
#include "winquake.h"

cvar_t	cl_nopred = {"cl_nopred","0"};
cvar_t	cl_pushlatency = {"pushlatency","-999"};

extern	frame_t		*view_frame;

#define	MAX_PARSE_ENTITIES	1024
extern entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];
int	CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles);


extern float	pm_airaccelerate;

extern usercmd_t independantphysics[MAX_SPLITS];

#ifdef Q2CLIENT
char *Get_Q2ConfigString(int i);

#ifdef Q2BSPS
void Q2_Pmove (q2pmove_t *pmove);
#define	Q2PMF_DUCKED			1
#define	Q2PMF_JUMP_HELD		2
#define	Q2PMF_ON_GROUND		4
#define	Q2PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	Q2PMF_TIME_LAND		16	// pm_time is time before rejump
#define	Q2PMF_TIME_TELEPORT	32	// pm_time is non-moving time
#define Q2PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)
#endif

vec3_t cl_predicted_origins[UPDATE_BACKUP];


/*
===================
CL_CheckPredictionError
===================
*/
#ifdef Q2BSPS
void CLQ2_CheckPredictionError (void)
{
	int		frame;
	int		delta[3];
	int		i;
	int		len;

	if (cl_nopred.value || (cl.q2frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (UPDATE_MASK);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cl.q2frame.playerstate.pmove.origin, cl_predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear (cl.prediction_error);
	}
	else
	{
		if (/*cl_showmiss->value && */(delta[0] || delta[1] || delta[2]) )
			Con_Printf ("prediction miss on %i: %i\n", cl.q2frame.serverframe, 
			delta[0] + delta[1] + delta[2]);

		VectorCopy (cl.q2frame.playerstate.pmove.origin, cl_predicted_origins[frame]);

		// save for error itnerpolation
		for (i=0 ; i<3 ; i++)
			cl.prediction_error[i] = delta[i]*0.125;
	}
}


/*
====================
CL_ClipMoveToEntities

====================
*/
void CLQ2_ClipMoveToEntities ( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr )
{
	int			i, x, zd, zu;
	trace_t		trace;
	int			headnode;
	float		*angles;
	entity_state_t	*ent;
	int			num;
	model_t		*cmodel;
	vec3_t		bmins, bmaxs;

	for (i=0 ; i<cl.q2frame.num_entities ; i++)
	{
		num = (cl.q2frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cl.playernum[0]+1)
			continue;

		if (ent->solid == 31)
		{	// special value for bmodel
			cmodel = cl.model_precache[ent->modelindex];
			if (!cmodel)
				continue;
			headnode = cmodel->hulls[0].firstclipnode;
			angles = ent->angles;
		}
		else
		{	// encoded bbox
			x = 8*(ent->solid & 31);
			zd = 8*((ent->solid>>5) & 31);
			zu = 8*((ent->solid>>10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headnode = CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3_origin;	// boxes don't rotate
		}

		if (tr->allsolid)
			return;

		trace = CM_TransformedBoxTrace (start, end,
			mins, maxs, headnode,  MASK_PLAYERSOLID,
			ent->origin, angles);

		if (trace.allsolid || trace.startsolid ||
		trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_s *)ent;
		 	if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
				*tr = trace;
		}
		else if (trace.startsolid)
			tr->startsolid = true;
	}
}


/*
================
CL_PMTrace
================
*/
q2trace_t	CLQ2_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	q2trace_t	q2t;
	trace_t		t;

	// check against world
	t = CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *)1;

	// check all other solid models
	CLQ2_ClipMoveToEntities (start, mins, maxs, end, &t);

	q2t.allsolid = t.allsolid;
	q2t.contents = t.contents;
	VectorCopy(t.endpos, q2t.endpos);
	q2t.ent = t.ent;
	q2t.fraction = t.fraction;
	q2t.plane = t.plane;
	q2t.startsolid = t.startsolid;
	q2t.surface = t.surface;

	return q2t;
}

int		CLQ2_PMpointcontents (vec3_t point)
{
	int			i;
	entity_state_t	*ent;
	int			num;
	model_t		*cmodel;
	int			contents;

	contents = CM_PointContents (point, 0);

	for (i=0 ; i<cl.q2frame.num_entities ; i++)
	{
		num = (cl.q2frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (ent->solid != 31) // special value for bmodel
			continue;

		cmodel = cl.model_precache[ent->modelindex];
		if (!cmodel)
			continue;

		contents |= CM_TransformedPointContents (point, cmodel->hulls[0].firstclipnode, ent->origin, ent->angles);
	}

	return contents;
}

#endif
/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CLQ2_PredictMovement (void)	//q2 doesn't support split clients.
{
#ifdef Q2BSPS
	int			ack, current;
	int			frame;
	int			oldframe;
	usercmd_t	*cmd;
	q2pmove_t		pm;
	int			step;
	int			oldz;
#endif
	int			i;

	if (cls.state != ca_active)
		return;

//	if (cl_paused->value)
//		return;

#ifdef Q2BSPS
	if (cl_nopred.value || (cl.q2frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
#endif
	{	// just set angles
		for (i=0 ; i<3 ; i++)
		{
			cl.predicted_angles[i] = cl.viewangles[0][i] + SHORT2ANGLE(cl.q2frame.playerstate.pmove.delta_angles[i]);
		}
		return;
	}
#ifdef Q2BSPS
	ack = cls.netchan.incoming_acknowledged;
	current = cls.netchan.outgoing_sequence;

	// if we are too far out of date, just freeze
	if (current - ack >= UPDATE_MASK)
	{
//		if (cl_showmiss->value)
			Con_Printf ("exceeded CMD_BACKUP\n");
		return;	
	}

	// copy current state to pmove
	memset (&pm, 0, sizeof(pm));
	pm.trace = CLQ2_PMTrace;
	pm.pointcontents = CLQ2_PMpointcontents;

	pm_airaccelerate = atof(Get_Q2ConfigString(Q2CS_AIRACCEL));

	pm.s = cl.q2frame.playerstate.pmove;

//	SCR_DebugGraph (current - ack - 1, 0);

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (UPDATE_MASK);
		cmd = &cl.frames[frame].cmd[0];

		pm.cmd = *cmd;
		Q2_Pmove (&pm);

		// save for debug checking
		VectorCopy (pm.s.origin, cl_predicted_origins[frame]);
	}

	if (independantphysics[0].msec)
	{
		cmd = &independantphysics[0];

		pm.cmd = *cmd;
		Q2_Pmove (&pm);
	}

	oldframe = (ack-2) & (UPDATE_MASK);
	oldz = cl_predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & Q2PMF_ON_GROUND) )
	{
		cl.predicted_step = step * 0.125;
		cl.predicted_step_time = realtime - host_frametime * 500;
	}

	cl.onground[0] = !!(pm.s.pm_flags & Q2PMF_ON_GROUND);


	// copy results out for rendering
	cl.predicted_origin[0] = pm.s.origin[0]*0.125;
	cl.predicted_origin[1] = pm.s.origin[1]*0.125;
	cl.predicted_origin[2] = pm.s.origin[2]*0.125;

	VectorCopy (pm.viewangles, cl.predicted_angles);
#endif
}

/*
=================
CL_NudgePosition

If pmove.origin is in a solid position,
try nudging slightly on all axis to
allow for the cut precision of the net coordinates
=================
*/
void CL_NudgePosition (void)
{
	vec3_t	base;
	int		x, y;

	if (cl.worldmodel->hulls->funcs.HullPointContents (cl.worldmodel->hulls, pmove.origin) == FTECONTENTS_EMPTY)
		return;

	VectorCopy (pmove.origin, base);
	for (x=-1 ; x<=1 ; x++)
	{
		for (y=-1 ; y<=1 ; y++)
		{
			pmove.origin[0] = base[0] + x * 1.0/8;
			pmove.origin[1] = base[1] + y * 1.0/8;
			if (cl.worldmodel->hulls->funcs.HullPointContents (cl.worldmodel->hulls, pmove.origin) == FTECONTENTS_EMPTY)
				return;
		}
	}
	Con_DPrintf ("CL_NudgePosition: stuck\n");
}

#endif

/*
==============
CL_PredictUsercmd
==============
*/
void CL_PredictUsercmd (int pnum, player_state_t *from, player_state_t *to, usercmd_t *u)
{
	extern vec3_t player_mins;
	extern vec3_t player_maxs;
	// split up very long moves
	if (u->msec > 50) {
		player_state_t temp;
		usercmd_t split;

		split = *u;
		split.msec /= 2;

		CL_PredictUsercmd (pnum, from, &temp, &split);
		CL_PredictUsercmd (pnum, &temp, to, &split);
		return;
	}

	VectorCopy (from->origin, pmove.origin);
	VectorCopy (u->angles, pmove.angles);
	VectorCopy (from->velocity, pmove.velocity);

	pmove.jump_msec = (cls.z_ext & Z_EXT_PM_TYPE) ? 0 : from->jump_msec;
	pmove.jump_held = from->jump_held;
	pmove.waterjumptime = from->waterjumptime;
	pmove.pm_type = from->pm_type;

	pmove.cmd = *u;

	movevars.entgravity = cl.entgravity[pnum];
	movevars.maxspeed = cl.maxspeed[pnum];
	movevars.bunnyspeedcap = cl.bunnyspeedcap;
	pmove.onladder = false;
	pmove.hullnum = from->hullnum;

	if (cl.worldmodel->fromgame == fg_quake2 || cl.worldmodel->fromgame == fg_quake3 || pmove.hullnum > MAX_MAP_HULLSM)
	{
		player_mins[0] = -16;
		player_mins[1] = -16;
		player_mins[2] = -24;
		player_maxs[0] = 16;
		player_maxs[1] = 16;
		player_maxs[2] = 32;

		VectorScale(player_mins, pmove.hullnum/56.0f, player_mins);
		VectorScale(player_maxs, pmove.hullnum/56.0f, player_maxs);
	}
	else
	{
		VectorCopy(cl.worldmodel->hulls[pmove.hullnum].clip_mins, player_mins);
		VectorCopy(cl.worldmodel->hulls[pmove.hullnum].clip_maxs, player_maxs);
	}
	if (DEFAULT_VIEWHEIGHT > player_maxs[2])
	{
		player_maxs[2] -= player_mins[2];
		player_mins[2] = 0;
	}

	PM_PlayerMove ();

	to->waterjumptime = pmove.waterjumptime;
	to->jump_held = pmove.jump_held;
	to->jump_msec = pmove.jump_msec;
	pmove.jump_msec = 0;

	VectorCopy (pmove.origin, to->origin);
	VectorCopy (pmove.angles, to->viewangles);
	VectorCopy (pmove.velocity, to->velocity);
	to->onground = pmove.onground;

	to->weaponframe = from->weaponframe;
	to->pm_type = from->pm_type;
	to->hullnum = from->hullnum;
}


//Used when cl_nopred is 1 to determine whether we are on ground, otherwise stepup smoothing code produces ugly jump physics
void CL_CatagorizePosition (int pnum)
{
	if (cl.spectator)
	{
		cl.onground[pnum] = false;	// in air
		return;
	}
	VectorClear (pmove.velocity);
	VectorCopy (cl.simorg[pnum], pmove.origin);
	pmove.numtouch = 0;
	PM_CategorizePosition ();
	cl.onground[pnum] = pmove.onground;
}
//Smooth out stair step ups.
//Called before CL_EmitEntities so that the player's lightning model origin is updated properly
void CL_CalcCrouch (int pnum)
{
	qboolean teleported;
	static vec3_t oldorigin[MAX_SPLITS];
	static float oldz[MAX_SPLITS] = {0}, extracrouch[MAX_SPLITS] = {0}, crouchspeed[MAX_SPLITS] = {100,100};
	vec3_t delta;

	VectorSubtract(cl.simorg[pnum], oldorigin[pnum], delta);

	teleported = Length(delta)>48;
	VectorCopy (cl.simorg[pnum], oldorigin[pnum]);

	if (teleported)
	{
		// possibly teleported or respawned
		oldz[pnum] = cl.simorg[pnum][2];
		extracrouch[pnum] = 0;
		crouchspeed[pnum] = 100;
		cl.crouch[pnum] = 0;
		VectorCopy (cl.simorg[pnum], oldorigin[pnum]);
		return;
	}

	if (cl.onground[pnum] && cl.simorg[pnum][2] - oldz[pnum] > 0)
	{
		if (cl.simorg[pnum][2] - oldz[pnum] > 20)
		{
			// if on steep stairs, increase speed
			if (crouchspeed[pnum] < 160)
			{
				extracrouch[pnum] = cl.simorg[pnum][2] - oldz[pnum] - host_frametime * 200 - 15;
				extracrouch[pnum] = min(extracrouch[pnum], 5);
			}
			crouchspeed[pnum] = 160;
		}

		oldz[pnum] += host_frametime * crouchspeed[pnum];
		if (oldz[pnum] > cl.simorg[pnum][2])
			oldz[pnum] = cl.simorg[pnum][2];

		if (cl.simorg[pnum][2] - oldz[pnum] > 15 + extracrouch[pnum])
			oldz[pnum] = cl.simorg[pnum][2] - 15 - extracrouch[pnum];
		extracrouch[pnum] -= host_frametime * 200;
		extracrouch[pnum] = max(extracrouch[pnum], 0);

		cl.crouch[pnum] = oldz[pnum] - cl.simorg[pnum][2];
	}
	else
	{
		// in air or moving down
		oldz[pnum] = cl.simorg[pnum][2];
		cl.crouch[pnum] += host_frametime * 150;
		if (cl.crouch[pnum] > 0)
			cl.crouch[pnum] = 0;
		crouchspeed[pnum] = 100;
		extracrouch[pnum] = 0;
	}
}

/*
==============
CL_PredictMove
==============
*/
void CL_PredictMovePNum (int pnum)
{
	frame_t ind;
	int			i;
	float		f;
	frame_t		*from, *to = NULL;
	int			oldphysent;
	vec3_t lrp;

	//these are to make svc_viewentity work better
	float *vel;
	float *org;
#ifdef Q2CLIENT
	if (cls.q2server)
	{
		cl.crouch[pnum] = 0;
		CLQ2_PredictMovement();
		return;
	}
#endif

	if (cl_pushlatency.value > 0)
		Cvar_Set (&cl_pushlatency, "0");

	if (cl.paused && !cls.demoplayback!=DPB_MVD && (!cl.spectator || !autocam[pnum])) 
		return;
#ifdef NQPROT
	if (cls.demoplayback!=DPB_NETQUAKE)	//don't increase time in nq demos.
#endif
	{
		cl.time = realtime - cls.latency - cl_pushlatency.value*0.001;
		if (cl.time > realtime)
			cl.time = realtime;
	}
/*	else
	{
		entitystate_t
		cl.crouch = 0;
		VectorCopy(cl
		return;
	}
*/
	if (cl.intermission && cl.intermission != 3)
	{
		cl.crouch[pnum] = 0;
		return;
	}

	if (!cl.validsequence)
	{
		return;
	}

	if (cls.netchan.outgoing_sequence - cl.validsequence >= UPDATE_BACKUP-1)
	{
		return;
	}

	if (cls.state == ca_onserver)
	{	// first update is the final signon stage
		cls.state = ca_active;
		if (VID_SetWindowCaption)
			VID_SetWindowCaption(va("FTE QuakeWorld: %s", cls.servername));

		SCR_EndLoadingPlaque();
	}

	// this is the last frame received from the server
	from = &cl.frames[cl.validsequence & UPDATE_MASK];

	if (!cl.intermission)
	{
		VectorCopy (cl.viewangles[pnum], cl.simangles[pnum]);
	}

	vel = from->playerstate[cl.playernum[pnum]].velocity;
	org = from->playerstate[cl.playernum[pnum]].origin;

#ifdef PEXT_SETVIEW
	if (cl.viewentity[pnum])
	{
		entity_state_t *CL_FindOldPacketEntity(int num);
		entity_state_t *CL_FindPacketEntity(int num);
		entity_state_t *state, *old;
		state = CL_FindPacketEntity (cl.viewentity[pnum]);
		old = CL_FindOldPacketEntity (cl.viewentity[pnum]);
		if (state)
		{
			if (old)
			{
				float f = (cl.time-cl.lerpents[cl.viewentity[pnum]].lerptime)/cl.lerpents[cl.viewentity[pnum]].lerprate;
				f=1-f;
				if (f<0)f=0;
				if (f>1)f=1;

				for (i=0 ; i<3 ; i++)
					lrp[i] = state->origin[i] + 
							f * (old->origin[i] - state->origin[i]);

				org = lrp;
			}
			else
				org = state->origin;

			goto fixedorg;
		}
	}
#endif
	if (((cl_nopred.value && cls.demoplayback!=DPB_MVD)|| cl.fixangle))
	{
fixedorg:
		VectorCopy (vel, cl.simvel[pnum]);
		VectorCopy (org, cl.simorg[pnum]);

		to = &cl.frames[cl.validsequence & UPDATE_MASK];



		CL_CatagorizePosition(pnum);
		goto out;
	}

	// predict forward until cl.time <= to->senttime
	oldphysent = pmove.numphysent;
	CL_SetSolidPlayers (cl.playernum[pnum]);

	to = &cl.frames[cl.validsequence & UPDATE_MASK];

	for (i=1 ; i<UPDATE_BACKUP-1 && cl.validsequence+i <
			cls.netchan.outgoing_sequence; i++)
	{
		to = &cl.frames[(cl.validsequence+i) & UPDATE_MASK];
		if (cl.intermission)
			to->playerstate->pm_type = PM_FLY;
		CL_PredictUsercmd (pnum, &from->playerstate[cl.playernum[pnum]]
			, &to->playerstate[cl.playernum[pnum]], &to->cmd[pnum]);

		cl.onground[pnum] = pmove.onground;

		if (to->senttime >= cl.time)
			break;
		from = to;
	}
	
	if (independantphysics[pnum].msec)
	{
		from = to;
		to = &ind;
		to->cmd[pnum] = independantphysics[pnum];
		to->senttime = cl.time;
			CL_PredictUsercmd (pnum, &from->playerstate[cl.playernum[pnum]]
			, &to->playerstate[cl.playernum[pnum]], &to->cmd[pnum]);

		cl.onground[pnum] = pmove.onground;
	}

	pmove.numphysent = oldphysent;

	if (1)//!independantphysics.msec)
	{
		VectorCopy (to->playerstate[cl.playernum[pnum]].velocity, cl.simvel[pnum]);
		VectorCopy (to->playerstate[cl.playernum[pnum]].origin, cl.simorg[pnum]);
	}
	else
	{
		// now interpolate some fraction of the final frame
		if (to->senttime == from->senttime)
			f = 0;
		else
		{
			f = (cl.time - from->senttime) / (to->senttime - from->senttime);

			if (f < 0)
				f = 0;
			if (f > 1)
				f = 1;
		}

		for (i=0 ; i<3 ; i++)
			if ( fabs(org[i] - to->playerstate[cl.playernum[pnum]].origin[i]) > 128)
			{	// teleported, so don't lerp
				VectorCopy (to->playerstate[cl.playernum[pnum]].velocity, cl.simvel[pnum]);
				VectorCopy (to->playerstate[cl.playernum[pnum]].origin, cl.simorg[pnum]);
				goto out;
			}
			
		for (i=0 ; i<3 ; i++)
		{
			cl.simorg[pnum][i] = org[i] 
				+ f*(to->playerstate[cl.playernum[pnum]].origin[i] - org[i]);
			cl.simvel[pnum][i] = vel[i] 
				+ f*(to->playerstate[cl.playernum[pnum]].velocity[i] - vel[i]);
		}
		CL_CatagorizePosition(pnum);

	}
out:
	CL_CalcCrouch (pnum);
}

void CL_PredictMove (void)
{
	int i;
	for (i = 0; i < cl.splitclients; i++)
		CL_PredictMovePNum(i);
}


/*
==============
CL_InitPrediction
==============
*/
void CL_InitPrediction (void)
{
	extern char cl_predictiongroup[];
	Cvar_Register (&cl_pushlatency, cl_predictiongroup);
	Cvar_Register (&cl_nopred,	cl_predictiongroup);
}
