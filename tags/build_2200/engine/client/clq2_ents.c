#include "bothdefs.h"
#ifdef Q2CLIENT
#include "quakedef.h"

extern cvar_t r_drawviewmodel;

extern cvar_t cl_nopred;
typedef enum
{
	Q2EV_NONE,
	Q2EV_ITEM_RESPAWN,
	Q2EV_FOOTSTEP,
	Q2EV_FALLSHORT,
	Q2EV_FALL,
	Q2EV_FALLFAR,
	Q2EV_PLAYER_TELEPORT,
	Q2EV_OTHER_TELEPORT
} q2entity_event_t;

#define Q2PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)

float LerpAngle (float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}


#define	Q2PS_M_TYPE			(1<<0)
#define	Q2PS_M_ORIGIN			(1<<1)
#define	Q2PS_M_VELOCITY		(1<<2)
#define	Q2PS_M_TIME			(1<<3)
#define	Q2PS_M_FLAGS			(1<<4)
#define	Q2PS_M_GRAVITY		(1<<5)
#define	Q2PS_M_DELTA_ANGLES	(1<<6)

#define	Q2PS_VIEWOFFSET		(1<<7)
#define	Q2PS_VIEWANGLES		(1<<8)
#define	Q2PS_KICKANGLES		(1<<9)
#define	Q2PS_BLEND			(1<<10)
#define	Q2PS_FOV				(1<<11)
#define	Q2PS_WEAPONINDEX		(1<<12)
#define	Q2PS_WEAPONFRAME		(1<<13)
#define	Q2PS_RDFLAGS			(1<<14)



// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define	Q2EF_ROTATE				0x00000001		// rotate (bonus items)
#define	Q2EF_GIB				0x00000002		// leave a trail
#define	Q2EF_BLASTER			0x00000008		// redlight + trail
#define	Q2EF_ROCKET				0x00000010		// redlight + trail
#define	Q2EF_GRENADE			0x00000020
#define	Q2EF_HYPERBLASTER		0x00000040
#define	Q2EF_BFG				0x00000080
#define Q2EF_COLOR_SHELL		0x00000100
#define Q2EF_POWERSCREEN		0x00000200
#define	Q2EF_ANIM01				0x00000400		// automatically cycle between frames 0 and 1 at 2 hz
#define	Q2EF_ANIM23				0x00000800		// automatically cycle between frames 2 and 3 at 2 hz
#define Q2EF_ANIM_ALL			0x00001000		// automatically cycle through all frames at 2hz
#define Q2EF_ANIM_ALLFAST		0x00002000		// automatically cycle through all frames at 10hz
#define	Q2EF_FLIES				0x00004000
#define	Q2EF_QUAD				0x00008000
#define	Q2EF_PENT				0x00010000
#define	Q2EF_TELEPORTER			0x00020000		// particle fountain
#define Q2EF_FLAG1				0x00040000
#define Q2EF_FLAG2				0x00080000
// RAFAEL
#define Q2EF_IONRIPPER			0x00100000
#define Q2EF_GREENGIB			0x00200000
#define	Q2EF_BLUEHYPERBLASTER	0x00400000
#define Q2EF_SPINNINGLIGHTS		0x00800000
#define Q2EF_PLASMA				0x01000000
#define Q2EF_TRAP				0x02000000

//ROGUE
#define Q2EF_TRACKER			0x04000000
#define	Q2EF_DOUBLE				0x08000000
#define	Q2EF_SPHERETRANS		0x10000000
#define Q2EF_TAGTRAIL			0x20000000
#define Q2EF_HALF_DAMAGE		0x40000000
#define Q2EF_TRACKERTRAIL		0x80000000
//ROGUE

// entity_state_t->renderfx flags
#define	Q2RF_MINLIGHT			1		// allways have some light (viewmodel)
#define	Q2RF_VIEWERMODEL		2		// don't draw through eyes, only mirrors
#define	Q2RF_WEAPONMODEL		4		// only draw through eyes
#define	Q2RF_FULLBRIGHT			8		// allways draw full intensity
#define	Q2RF_DEPTHHACK			16		// for view weapon Z crunching
#define	Q2RF_TRANSLUCENT		32
#define	Q2RF_FRAMELERP			64
#define Q2RF_BEAM				128
#define	Q2RF_CUSTOMSKIN			256		// skin is an index in image_precache
#define	Q2RF_GLOW				512		// pulse lighting for bonus items
#define Q2RF_SHELL_RED			1024
#define	Q2RF_SHELL_GREEN		2048
#define Q2RF_SHELL_BLUE			4096

//ROGUE
#define Q2RF_IR_VISIBLE			0x00008000		// 32768
#define	Q2RF_SHELL_DOUBLE		0x00010000		// 65536
#define	Q2RF_SHELL_HALF_DAM		0x00020000
#define Q2RF_USE_DISGUISE		0x00040000
//ROGUE

// player_state_t->refdef flags
#define	Q2RDF_UNDERWATER		1		// warp the screen as apropriate
#define Q2RDF_NOWORLDMODEL		2		// used for player configuration screen

//ROGUE
#define	Q2RDF_IRGOGGLES			4
#define Q2RDF_UVGOGGLES			8
//ROGUE




#define Q2MAX_STATS	32



typedef struct q2centity_s
{
	entity_state_t	baseline;		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int			serverframe;		// if not current, this ent isn't in the frame

	trailstate_t trailstate;
//	float		trailcount;			// for diminishing grenade trails
	vec3_t		lerp_origin;		// for trails (variable hz)

	int			fly_stoptime;
} q2centity_t;


void CLQ2_EntityEvent(entity_state_t *es){};
void CLQ2_TeleporterParticles(entity_state_t *es){};
void CLQ2_IonripperTrail(vec3_t oldorg, vec3_t neworg){};
void CLQ2_TrackerTrail(vec3_t oldorg, vec3_t neworg, int flags){};
void CLQ2_Tracker_Shell(vec3_t org){};
void CLQ2_TagTrail(vec3_t oldorg, vec3_t neworg, int flags){};
void CLQ2_FlagTrail(vec3_t oldorg, vec3_t neworg, int flags){};
void CLQ2_TrapParticles(entity_t *ent){};
void CLQ2_BfgParticles(entity_t *ent){};
void CLQ2_FlyEffect(q2centity_t *ent, vec3_t org){};
void CLQ2_DiminishingTrail(vec3_t oldorg, vec3_t neworg, q2centity_t *ent, unsigned int effects){};
void CLQ2_BlasterTrail(vec3_t oldorg, vec3_t neworg);
void CLQ2_BlasterTrail2(vec3_t oldorg, vec3_t neworg){};
void CLQ2_RocketTrail(vec3_t oldorg, vec3_t neworg, q2centity_t *ent)
{
	extern int rt_rocket;
	R_RocketTrail(oldorg, neworg, rt_rocket, &ent->trailstate);
};




#define MAX_Q2EDICTS 1024
#define	MAX_PARSE_ENTITIES	1024


q2centity_t cl_entities[MAX_Q2EDICTS];
entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];


//extern	struct model_s	*cl_mod_powerscreen;

//PGM
int	vidref_val;
//PGM

/*
=========================================================================

FRAME PARSING

=========================================================================
*/

#if 0

typedef struct
{
	int		modelindex;
	int		num; // entity number
	int		effects;
	vec3_t	origin;
	vec3_t	oldorigin;
	vec3_t	angles;
	qboolean present;
} projectile_t;

#define	MAX_PROJECTILES	64
projectile_t	cl_projectiles[MAX_PROJECTILES];

void CLQ2_ClearProjectiles (void)
{
	int i;

	for (i = 0; i < MAX_PROJECTILES; i++) {
//		if (cl_projectiles[i].present)
//			Com_DPrintf("PROJ: %d CLEARED\n", cl_projectiles[i].num);
		cl_projectiles[i].present = false;
	}
}

/*
=====================
CL_ParseProjectiles

Flechettes are passed as efficient temporary entities
=====================
*/
void CLQ2_ParseProjectiles (void)
{
	int		i, c, j;
	byte	bits[8];
	byte	b;
	projectile_t	pr;
	int lastempty = -1;
	qboolean old = false;

	c = MSG_ReadByte (&net_message);
	for (i=0 ; i<c ; i++)
	{
		bits[0] = MSG_ReadByte (&net_message);
		bits[1] = MSG_ReadByte (&net_message);
		bits[2] = MSG_ReadByte (&net_message);
		bits[3] = MSG_ReadByte (&net_message);
		bits[4] = MSG_ReadByte (&net_message);
		pr.origin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		pr.origin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		pr.origin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		VectorCopy(pr.origin, pr.oldorigin);

		if (bits[4] & 64)
			pr.effects = EF_BLASTER;
		else
			pr.effects = 0;

		if (bits[4] & 128) {
			old = true;
			bits[0] = MSG_ReadByte (&net_message);
			bits[1] = MSG_ReadByte (&net_message);
			bits[2] = MSG_ReadByte (&net_message);
			bits[3] = MSG_ReadByte (&net_message);
			bits[4] = MSG_ReadByte (&net_message);
			pr.oldorigin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
			pr.oldorigin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
			pr.oldorigin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		}

		bits[0] = MSG_ReadByte (&net_message);
		bits[1] = MSG_ReadByte (&net_message);
		bits[2] = MSG_ReadByte (&net_message);

		pr.angles[0] = 360*bits[0]/256;
		pr.angles[1] = 360*bits[1]/256;
		pr.modelindex = bits[2];

		b = MSG_ReadByte (&net_message);
		pr.num = (b & 0x7f);
		if (b & 128) // extra entity number byte
			pr.num |= (MSG_ReadByte (&net_message) << 7);

		pr.present = true;

		// find if this projectile already exists from previous frame 
		for (j = 0; j < MAX_PROJECTILES; j++) {
			if (cl_projectiles[j].modelindex) {
				if (cl_projectiles[j].num == pr.num) {
					// already present, set up oldorigin for interpolation
					if (!old)
						VectorCopy(cl_projectiles[j].origin, pr.oldorigin);
					cl_projectiles[j] = pr;
					break;
				}
			} else
				lastempty = j;
		}

		// not present previous frame, add it
		if (j == MAX_PROJECTILES) {
			if (lastempty != -1) {
				cl_projectiles[lastempty] = pr;
			}
		}
	}
}

/*
=============
CL_LinkProjectiles

=============
*/
void CLQ2_AddProjectiles (void)
{
	int		i, j;
	projectile_t	*pr;
	entity_t		ent;

	memset (&ent, 0, sizeof(ent));

	for (i=0, pr=cl_projectiles ; i < MAX_PROJECTILES ; i++, pr++)
	{
		// grab an entity to fill in
		if (pr->modelindex < 1)
			continue;
		if (!pr->present) {
			pr->modelindex = 0;
			continue; // not present this frame (it was in the previous frame)
		}

		ent.model = cl.model_draw[pr->modelindex];

		// interpolate origin
		for (j=0 ; j<3 ; j++)
		{
			ent.origin[j] = ent.oldorigin[j] = pr->oldorigin[j] + cl.lerpfrac * 
				(pr->origin[j] - pr->oldorigin[j]);

		}

		if (pr->effects & EF_BLASTER)
			CLQ2_BlasterTrail (pr->oldorigin, ent.origin);
		V_AddLight (pr->origin, 200, 0.2, 0.2, 0);

		VectorCopy (pr->angles, ent.angles);
		V_AddEntity (&ent);
	}
}
#endif

/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
int	bitcounts[32];	/// just for protocol profiling
int CLQ2_ParseEntityBits (unsigned *bits)
{
	unsigned	b, total;
	int			i;
	int			number;

	total = MSG_ReadByte ();
	if (total & Q2U_MOREBITS1)
	{
		b = MSG_ReadByte ();
		total |= b<<8;
	}
	if (total & Q2U_MOREBITS2)
	{
		b = MSG_ReadByte ();
		total |= b<<16;
	}
	if (total & Q2U_MOREBITS3)
	{
		b = MSG_ReadByte ();
		total |= b<<24;
	}

	// count the bits for net profiling
	for (i=0 ; i<32 ; i++)
		if (total&(1<<i))
			bitcounts[i]++;

	if (total & Q2U_NUMBER16)
		number = MSG_ReadShort ();
	else
		number = MSG_ReadByte ();

	*bits = total;

	return number;
}

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
void CLQ2_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy (from->origin, to->old_origin);
	to->number = number;

	if (bits & Q2U_MODEL)
		to->modelindex = MSG_ReadByte ();
	if (bits & Q2U_MODEL2)
		to->modelindex2 = MSG_ReadByte ();
	if (bits & Q2U_MODEL3)
		to->modelindex3 = MSG_ReadByte ();
	if (bits & Q2U_MODEL4)
		to->modelindex4 = MSG_ReadByte ();
		
	if (bits & Q2U_FRAME8)
		to->frame = MSG_ReadByte ();
	if (bits & Q2U_FRAME16)
		to->frame = MSG_ReadShort ();

	if ((bits & Q2U_SKIN8) && (bits & Q2U_SKIN16))		//used for laser colors
		to->skinnum = MSG_ReadLong();
	else if (bits & Q2U_SKIN8)
		to->skinnum = MSG_ReadByte();
	else if (bits & Q2U_SKIN16)
		to->skinnum = MSG_ReadShort();

	if ( (bits & (Q2U_EFFECTS8|Q2U_EFFECTS16)) == (Q2U_EFFECTS8|Q2U_EFFECTS16) )
		to->effects = MSG_ReadLong();
	else if (bits & Q2U_EFFECTS8)
		to->effects = MSG_ReadByte();
	else if (bits & Q2U_EFFECTS16)
		to->effects = MSG_ReadShort();

	if ( (bits & (Q2U_RENDERFX8|Q2U_RENDERFX16)) == (Q2U_RENDERFX8|Q2U_RENDERFX16) )
		to->renderfx = MSG_ReadLong();
	else if (bits & Q2U_RENDERFX8)
		to->renderfx = MSG_ReadByte();
	else if (bits & Q2U_RENDERFX16)
		to->renderfx = MSG_ReadShort();

	if (bits & Q2U_ORIGIN1)
		to->origin[0] = MSG_ReadCoord ();
	if (bits & Q2U_ORIGIN2)
		to->origin[1] = MSG_ReadCoord ();
	if (bits & Q2U_ORIGIN3)
		to->origin[2] = MSG_ReadCoord ();
		
	if (bits & Q2U_ANGLE1)
		to->angles[0] = MSG_ReadAngle();
	if (bits & Q2U_ANGLE2)
		to->angles[1] = MSG_ReadAngle();
	if (bits & Q2U_ANGLE3)
		to->angles[2] = MSG_ReadAngle();

	if (bits & Q2U_OLDORIGIN)
		MSG_ReadPos (to->old_origin);

	if (bits & Q2U_SOUND)
		to->sound = MSG_ReadByte ();

	if (bits & Q2U_EVENT)
		to->event = MSG_ReadByte ();
	else
		to->event = 0;

	if (bits & Q2U_SOLID)
		to->solid = MSG_ReadShort ();
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CLQ2_DeltaEntity (q2frame_t *frame, int newnum, entity_state_t *old, int bits)
{
	q2centity_t	*ent;
	entity_state_t	*state;

	ent = &cl_entities[newnum];

	state = &cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)];
	cl.parse_entities++;
	frame->num_entities++;

	CLQ2_ParseDelta (old, state, newnum, bits);

	// some data changes will force no lerping
	if (state->modelindex != ent->current.modelindex
		|| state->modelindex2 != ent->current.modelindex2
		|| state->modelindex3 != ent->current.modelindex3
		|| state->modelindex4 != ent->current.modelindex4
		|| abs(state->origin[0] - ent->current.origin[0]) > 512
		|| abs(state->origin[1] - ent->current.origin[1]) > 512
		|| abs(state->origin[2] - ent->current.origin[2]) > 512
		|| state->event == Q2EV_PLAYER_TELEPORT
		|| state->event == Q2EV_OTHER_TELEPORT
		)
	{
		ent->serverframe = -99;
	}

	if (ent->serverframe != cl.q2frame.serverframe - 1)
	{	// wasn't in last update, so initialize some things
		ent->trailstate.lastdist = 0;	// for diminishing rocket / grenade trails
		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->event == Q2EV_OTHER_TELEPORT)
		{
			VectorCopy (state->origin, ent->prev.origin);
			VectorCopy (state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy (state->old_origin, ent->prev.origin);
			VectorCopy (state->old_origin, ent->lerp_origin);
		}
	}
	else
	{	// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverframe = cl.q2frame.serverframe;
	ent->current = *state;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CLQ2_ParsePacketEntities (q2frame_t *oldframe, q2frame_t *newframe)
{
	int			newnum;
	int			bits;
	entity_state_t	*oldstate=NULL;
	int			oldindex, oldnum;

	cl.validsequence = cls.netchan.incoming_sequence;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	if (!oldframe)
		oldnum = 99999;
	else
	{
		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		newnum = CLQ2_ParseEntityBits (&bits);
		if (newnum >= MAX_Q2EDICTS)
			Host_EndGame ("CL_ParsePacketEntities: bad number:%i", newnum);

		if (msg_readcount > net_message.cursize)
			Host_EndGame ("CL_ParsePacketEntities: end of message");

		if (!newnum)
			break;

		while (oldnum < newnum)
		{	// one or more entities from the old packet are unchanged
			if (cl_shownet.value == 3)
				Con_Printf ("   unchanged: %i\n", oldnum);
			CLQ2_DeltaEntity (newframe, oldnum, oldstate, 0);
			
			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & Q2U_REMOVE)
		{	// the entity present in oldframe is not in the current frame
			if (cl_shownet.value == 3)
				Con_Printf ("   remove: %i\n", newnum);
			if (oldnum != newnum)
				Con_Printf ("U_REMOVE: oldnum != newnum\n");

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum)
		{	// delta from previous state
			if (cl_shownet.value == 3)
				Con_Printf ("   delta: %i\n", newnum);
			CLQ2_DeltaEntity (newframe, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{	// delta from baseline
			if (cl_shownet.value == 3)
				Con_Printf ("   baseline: %i\n", newnum);
			CLQ2_DeltaEntity (newframe, newnum, &cl_entities[newnum].baseline, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{	// one or more entities from the old packet are unchanged
		if (cl_shownet.value == 3)
			Con_Printf ("   unchanged: %i\n", oldnum);
		CLQ2_DeltaEntity (newframe, oldnum, oldstate, 0);
		
		oldindex++;

		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}

void CLQ2_ParseBaseline (void)
{
	entity_state_t	*es;
	int				bits;
	int				newnum;
	entity_state_t	nullstate;

	memset (&nullstate, 0, sizeof(nullstate));

	newnum = CLQ2_ParseEntityBits (&bits);
	es = &cl_entities[newnum].baseline;
	CLQ2_ParseDelta (&nullstate, es, newnum, bits);
}


/*
===================
CL_ParsePlayerstate
===================
*/
void CLQ2_ParsePlayerstate (q2frame_t *oldframe, q2frame_t *newframe)
{
	int			flags;
	q2player_state_t	*state;
	int			i;
	int			statbits;

	state = &newframe->playerstate;

	// clear to old value before delta parsing
	if (oldframe)
		*state = oldframe->playerstate;
	else
		memset (state, 0, sizeof(*state));

	flags = MSG_ReadShort ();

	//
	// parse the pmove_state_t
	//
	if (flags & Q2PS_M_TYPE)
		state->pmove.pm_type = MSG_ReadByte ();

	if (flags & Q2PS_M_ORIGIN)
	{
		state->pmove.origin[0] = MSG_ReadShort ();
		state->pmove.origin[1] = MSG_ReadShort ();
		state->pmove.origin[2] = MSG_ReadShort ();
	}

	if (flags & Q2PS_M_VELOCITY)
	{
		state->pmove.velocity[0] = MSG_ReadShort ();
		state->pmove.velocity[1] = MSG_ReadShort ();
		state->pmove.velocity[2] = MSG_ReadShort ();
	}

	if (flags & Q2PS_M_TIME)
		state->pmove.pm_time = MSG_ReadByte ();

	if (flags & Q2PS_M_FLAGS)
		state->pmove.pm_flags = MSG_ReadByte ();

	if (flags & Q2PS_M_GRAVITY)
		state->pmove.gravity = MSG_ReadShort ();

	if (flags & Q2PS_M_DELTA_ANGLES)
	{
		state->pmove.delta_angles[0] = MSG_ReadShort ();
		state->pmove.delta_angles[1] = MSG_ReadShort ();
		state->pmove.delta_angles[2] = MSG_ReadShort ();
	}

//	if (cl.attractloop)
//		state->pmove.pm_type = Q2PM_FREEZE;		// demo playback

	//
	// parse the rest of the player_state_t
	//
	if (flags & Q2PS_VIEWOFFSET)
	{
		state->viewoffset[0] = MSG_ReadChar () * 0.25;
		state->viewoffset[1] = MSG_ReadChar () * 0.25;
		state->viewoffset[2] = MSG_ReadChar () * 0.25;
	}

	if (flags & Q2PS_VIEWANGLES)
	{
		state->viewangles[0] = MSG_ReadAngle16 ();
		state->viewangles[1] = MSG_ReadAngle16 ();
		state->viewangles[2] = MSG_ReadAngle16 ();
	}

	if (flags & Q2PS_KICKANGLES)
	{
		state->kick_angles[0] = MSG_ReadChar () * 0.25;
		state->kick_angles[1] = MSG_ReadChar () * 0.25;
		state->kick_angles[2] = MSG_ReadChar () * 0.25;
	}

	if (flags & Q2PS_WEAPONINDEX)
	{
		state->gunindex = MSG_ReadByte ();
	}

	if (flags & Q2PS_WEAPONFRAME)
	{
		state->gunframe = MSG_ReadByte ();
		state->gunoffset[0] = MSG_ReadChar ()*0.25;
		state->gunoffset[1] = MSG_ReadChar ()*0.25;
		state->gunoffset[2] = MSG_ReadChar ()*0.25;
		state->gunangles[0] = MSG_ReadChar ()*0.25;
		state->gunangles[1] = MSG_ReadChar ()*0.25;
		state->gunangles[2] = MSG_ReadChar ()*0.25;
	}

	if (flags & Q2PS_BLEND)
	{
		state->blend[0] = MSG_ReadByte ()/255.0;
		state->blend[1] = MSG_ReadByte ()/255.0;
		state->blend[2] = MSG_ReadByte ()/255.0;
		state->blend[3] = MSG_ReadByte ()/255.0;
	}

	if (flags & Q2PS_FOV)
		state->fov = MSG_ReadByte ();

	if (flags & Q2PS_RDFLAGS)
		state->rdflags = MSG_ReadByte ();

	// parse stats
	statbits = MSG_ReadLong ();
	for (i=0 ; i<Q2MAX_STATS ; i++)
		if (statbits & (1<<i) )
			state->stats[i] = MSG_ReadShort();
}


/*
==================
CL_FireEntityEvents

==================
*/
void CLQ2_FireEntityEvents (q2frame_t *frame)
{
	entity_state_t		*s1;
	int					pnum, num;

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		num = (frame->parse_entities + pnum)&(MAX_PARSE_ENTITIES-1);
		s1 = &cl_parse_entities[num];
		if (s1->event)
			CLQ2_EntityEvent (s1);

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if (s1->effects & Q2EF_TELEPORTER)
			CLQ2_TeleporterParticles (s1);
	}
}


/*
================
CL_ParseFrame
================
*/
void CLQ2_ParseFrame (void)
{
	int			cmd;
	int			len;
	q2frame_t		*old;

	memset (&cl.q2frame, 0, sizeof(cl.q2frame));

#if 0
	CLQ2_ClearProjectiles(); // clear projectiles for new frame
#endif

	cl.q2frame.serverframe = MSG_ReadLong ();
	cl.q2frame.deltaframe = MSG_ReadLong ();
	cl.q2frame.servertime = cl.q2frame.serverframe*100;

	cl.surpressCount = MSG_ReadByte ();

	if (cl_shownet.value == 3)
		Con_Printf ("   frame:%i  delta:%i\n", cl.q2frame.serverframe,
		cl.q2frame.deltaframe);

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message 
	if (cl.q2frame.deltaframe <= 0)
	{
		cl.q2frame.valid = true;		// uncompressed frame
		old = NULL;
//		cls.demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.q2frames[cl.q2frame.deltaframe & Q2UPDATE_MASK];
		if (!old->valid)
		{	// should never happen
			Con_Printf ("Delta from invalid frame (not supposed to happen!).\n");
		}
		if (old->serverframe != cl.q2frame.deltaframe)
		{	// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Con_Printf ("Delta frame too old.\n");
		}
		else if (cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES-128)
		{
			Con_Printf ("Delta parse_entities too old.\n");
		}
		else
			cl.q2frame.valid = true;	// valid delta parse
	}

	// clamp time 
	if (cl.time > cl.q2frame.servertime/1000.0)
		cl.time = cl.q2frame.servertime/1000.0;
	else if (cl.time < (cl.q2frame.servertime - 100)/1000.0)
		cl.time = (cl.q2frame.servertime - 100)/1000.0;

	// read areabits
	len = MSG_ReadByte ();
	MSG_ReadData (&cl.q2frame.areabits, len);

	// read playerinfo
	cmd = MSG_ReadByte ();
//	SHOWNET(svc_strings[cmd]);
	if (cmd != svcq2_playerinfo)
		Host_EndGame ("CL_ParseFrame: not playerinfo");
	CLQ2_ParsePlayerstate (old, &cl.q2frame);

	// read packet entities
	cmd = MSG_ReadByte ();
//	SHOWNET(svc_strings[cmd]);
	if (cmd != svcq2_packetentities)
		Host_EndGame ("CL_ParseFrame: not packetentities");
	CLQ2_ParsePacketEntities (old, &cl.q2frame);

	// save the frame off in the backup array for later delta comparisons
	cl.q2frames[cl.q2frame.serverframe & Q2UPDATE_MASK] = cl.q2frame;

	if (cl.q2frame.valid)
	{
		// getting a valid frame message ends the connection process
		if (cls.state != ca_active)
		{
			if (VID_SetWindowCaption)
				VID_SetWindowCaption(va("FTE QuakeWorld: %s (Q2)", cls.servername));

			cls.state = ca_active;
//			cl.force_refdef = true;
			cl.predicted_origin[0] = cl.q2frame.playerstate.pmove.origin[0]*0.125;
			cl.predicted_origin[1] = cl.q2frame.playerstate.pmove.origin[1]*0.125;
			cl.predicted_origin[2] = cl.q2frame.playerstate.pmove.origin[2]*0.125;
			VectorCopy (cl.q2frame.playerstate.viewangles, cl.predicted_angles);
//			if (cls.disable_servercount != cl.servercount
//				&& cl.refresh_prepped)
				SCR_EndLoadingPlaque ();	// get rid of loading plaque
		}
//		cl.sound_prepped = true;	// can start mixing ambient sounds
	
		// fire entity events
		CLQ2_FireEntityEvents (&cl.q2frame);
#ifdef Q2BSPS
		CLQ2_CheckPredictionError ();
#endif
	}
}

/*
==========================================================================

INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/
/*
struct model_s *S_RegisterSexedModel (entity_state_t *ent, char *base)
{
	int				n;
	char			*p;
	struct model_s	*mdl;
	char			model[MAX_QPATH];
	char			buffer[MAX_QPATH];

	// determine what model the client is using
	model[0] = 0;
	n = CS_PLAYERSKINS + ent->number - 1;
	if (cl.configstrings[n][0])
	{
		p = strchr(cl.configstrings[n], '\\');
		if (p)
		{
			p += 1;
			strcpy(model, p);
			p = strchr(model, '/');
			if (p)
				*p = 0;
		}
	}
	// if we can't figure it out, they're male
	if (!model[0])
		strcpy(model, "male");

	Com_sprintf (buffer, sizeof(buffer), "players/%s/%s", model, base+1);
	mdl = re.RegisterModel(buffer);
	if (!mdl) {
		// not found, try default weapon model
		Com_sprintf (buffer, sizeof(buffer), "players/%s/weapon.md2", model);
		mdl = re.RegisterModel(buffer);
		if (!mdl) {
			// no, revert to the male model
			Com_sprintf (buffer, sizeof(buffer), "players/%s/%s", "male", base+1);
			mdl = re.RegisterModel(buffer);
			if (!mdl) {
				// last try, default male weapon.md2
				Com_sprintf (buffer, sizeof(buffer), "players/male/weapon.md2");
				mdl = re.RegisterModel(buffer);
			}
		} 
	}

	return mdl;
}

*/

void V_AddEntity(entity_t *in)
{
	entity_t *ent;
	if (cl_numvisedicts == MAX_VISEDICTS)
		return;		// object list is full
	ent = &cl_visedicts[cl_numvisedicts];
	cl_numvisedicts++;

	*ent = *in;
}

void V_AddLight (vec3_t org, float quant, float r, float g, float b)
{
	CL_NewDlightRGB (0, org[0], org[1], org[2], quant, 0.1, r, g, b);
}
/*
===============
CL_AddPacketEntities

===============
*/
void CLQ2_AddPacketEntities (q2frame_t *frame)
{
	entity_t			ent;
	entity_state_t		*s1;
	float				autorotate;
	int					i;
	int					pnum;
	q2centity_t			*cent;
	int					autoanim;
//	q2clientinfo_t		*ci;
	player_info_t		*player;
	unsigned int		effects, renderfx;

	// bonus items rotate at a fixed rate
	autorotate = anglemod(cl.time*100);

	// brush models can auto animate their frames
	autoanim = 2*cl.time;

	memset (&ent, 0, sizeof(ent));

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		s1 = &cl_parse_entities[(frame->parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];

		cent = &cl_entities[s1->number];

		effects = s1->effects;
		renderfx = s1->renderfx;

		ent.keynum = pnum;

		ent.scale = 1;
		ent.alpha = 1;
		ent.fatness = 0;
		ent.scoreboard = NULL;

			// set frame
		if (effects & Q2EF_ANIM01)
			ent.frame = autoanim & 1;
		else if (effects & Q2EF_ANIM23)
			ent.frame = 2 + (autoanim & 1);
		else if (effects & Q2EF_ANIM_ALL)
			ent.frame = autoanim;
		else if (effects & Q2EF_ANIM_ALLFAST)
			ent.frame = cl.time / 100;
		else
			ent.frame = s1->frame;

		// quad and pent can do different things on client
		if (effects & Q2EF_PENT)
		{
			effects &= ~Q2EF_PENT;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_RED;
		}

		if (effects & Q2EF_QUAD)
		{
			effects &= ~Q2EF_QUAD;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_BLUE;
		}
//======
// PMM
		if (effects & Q2EF_DOUBLE)
		{
			effects &= ~Q2EF_DOUBLE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_DOUBLE;
		}

		if (effects & Q2EF_HALF_DAMAGE)
		{
			effects &= ~Q2EF_HALF_DAMAGE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_HALF_DAM;
		}
// pmm
//======
		ent.oldframe = cent->prev.frame;
		ent.lerptime = 1.0 - cl.lerpfrac;

		if (renderfx & (Q2RF_FRAMELERP|Q2RF_BEAM))
		{	// step origin discretely, because the frames
			// do the animation properly
			VectorCopy (cent->current.origin, ent.origin);
			VectorCopy (cent->current.old_origin, ent.oldorigin);
		}
		else
		{	// interpolate origin
			for (i=0 ; i<3 ; i++)
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac * 
					(cent->current.origin[i] - cent->prev.origin[i]);
			}
		}

		// create a new entity
	
		// tweak the color of beams
		if ( renderfx & Q2RF_BEAM )
		{	// the four beam colors are encoded in 32 bits of skinnum (hack)
			ent.alpha = 0.30;
			ent.skinnum = (s1->skinnum >> ((rand() % 4)*8)) & 0xff;
			ent.model = NULL;
		}
		else
		{
			// set skin
			if (s1->modelindex == 255)
			{	// use custom player skin
				ent.skinnum = 0;
				ent.model = NULL;


				player = &cl.players[s1->skinnum%MAX_CLIENTS];
				ent.model = player->model;
				if (!ent.model)
				{
					ent.model = Mod_ForName(va("players/%s/tris.md2", Info_ValueForKey(player->userinfo, "model")), false);
					if (!ent.model)
						ent.model = Mod_ForName("players/male/tris.md2", false);
				}
				ent.scoreboard = player;
/*				ci = &cl.clientinfo[s1->skinnum & 0xff];
//				ent.skin = ci->skin;
				ent.model = ci->model;
				if (!ent.skin || !ent.model)
				{
					ent.skin = cl.baseclientinfo.skin;
					ent.model = cl.baseclientinfo.model;
				}

//============
//PGM
				if (renderfx & Q2RF_USE_DISGUISE)
				{
					if(!strncmp((char *)ent.skin, "players/male", 12))
					{
						ent.skin = re.RegisterSkin ("players/male/disguise.pcx");
						ent.model = re.RegisterModel ("players/male/tris.md2");
					}
					else if(!strncmp((char *)ent.skin, "players/female", 14))
					{
						ent.skin = re.RegisterSkin ("players/female/disguise.pcx");
						ent.model = re.RegisterModel ("players/female/tris.md2");
					}
					else if(!strncmp((char *)ent.skin, "players/cyborg", 14))
					{
						ent.skin = re.RegisterSkin ("players/cyborg/disguise.pcx");
						ent.model = re.RegisterModel ("players/cyborg/tris.md2");
					}
				}*/
//PGM
//============
			}
			else
			{
				ent.skinnum = s1->skinnum;
//				ent.skin = NULL;
				ent.model = cl.model_precache[s1->modelindex];
			}
		}

		// only used for black hole model right now, FIXME: do better
		if (renderfx == Q2RF_TRANSLUCENT)
			ent.alpha = 0.70;

		// render effects (fullbright, translucent, etc)
		if ((effects & Q2EF_COLOR_SHELL))
			ent.flags = 0;	// renderfx go on color shell entity
		else
			ent.flags = renderfx;

		// calculate angles
		if (effects & Q2EF_ROTATE)
		{	// some bonus items auto-rotate
			ent.angles[0] = 0;
			ent.angles[1] = autorotate;
			ent.angles[2] = 0;
		}
		// RAFAEL
		else if (effects & Q2EF_SPINNINGLIGHTS)
		{
			ent.angles[0] = 0;
			ent.angles[1] = anglemod(cl.time/2) + s1->angles[1];
			ent.angles[2] = 180;
			{
				vec3_t forward;
				vec3_t start;

				AngleVectors (ent.angles, forward, NULL, NULL);
				VectorMA (ent.origin, 64, forward, start);
				V_AddLight (start, 100, 0.2, 0, 0);
			}
		}
		else
		{	// interpolate angles
			float	a1, a2;

			for (i=0 ; i<3 ; i++)
			{
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				ent.angles[i] = LerpAngle (a2, a1, cl.lerpfrac);
			}
		}

 	ent.angles[0]*=-1;

		if (s1->number == cl.playernum[0]+1)
		{
			ent.flags |= Q2RF_VIEWERMODEL;	// only draw from mirrors
			// FIXME: still pass to refresh

			if (effects & Q2EF_FLAG1)
				V_AddLight (ent.origin, 225, 0.2, 0.05, 0.05);
			else if (effects & Q2EF_FLAG2)
				V_AddLight (ent.origin, 225, 0.05, 0.05, 0.2);
			else if (effects & Q2EF_TAGTRAIL)						//PGM
				V_AddLight (ent.origin, 225, 0.2, 0.2, 0.0);	//PGM
			else if (effects & Q2EF_TRACKERTRAIL)					//PGM
				V_AddLight (ent.origin, 225, -0.2, -0.2, -0.2);	//PGM

			continue;
		}

		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		if (effects & Q2EF_BFG)
		{
			ent.flags |= Q2RF_TRANSLUCENT;
			ent.alpha = 0.30;
		}

		// RAFAEL
		if (effects & Q2EF_PLASMA)
		{
			ent.flags |= Q2RF_TRANSLUCENT;
			ent.alpha = 0.6;
		}

		if (effects & Q2EF_SPHERETRANS)
		{
			ent.flags |= Q2RF_TRANSLUCENT;
			// PMM - *sigh*  yet more EF overloading
			if (effects & Q2EF_TRACKERTRAIL)
				ent.alpha = 0.6;
			else
				ent.alpha = 0.3;
		}
//pmm

		// add to refresh list
		V_AddEntity (&ent);


		// color shells generate a seperate entity for the main model
		if (effects & Q2EF_COLOR_SHELL)
		{
			// PMM - at this point, all of the shells have been handled
			// if we're in the rogue pack, set up the custom mixing, otherwise just
			// keep going

			// all of the solo colors are fine.  we need to catch any of the combinations that look bad
			// (double & half) and turn them into the appropriate color, and make double/quad something special
/*			if (renderfx & Q2RF_SHELL_HALF_DAM)
			{
				
				{
					// ditch the half damage shell if any of red, blue, or double are on
					if (renderfx & (Q2RF_SHELL_RED|Q2RF_SHELL_BLUE|Q2RF_SHELL_DOUBLE))
						renderfx &= ~Q2RF_SHELL_HALF_DAM;
				}
			}

			if (renderfx & Q2RF_SHELL_DOUBLE)
			{

				{
					// lose the yellow shell if we have a red, blue, or green shell
					if (renderfx & (Q2RF_SHELL_RED|Q2RF_SHELL_BLUE|Q2RF_SHELL_GREEN))
						renderfx &= ~Q2RF_SHELL_DOUBLE;
					// if we have a red shell, turn it to purple by adding blue
					if (renderfx & Q2RF_SHELL_RED)
						renderfx |= Q2RF_SHELL_BLUE;
					// if we have a blue shell (and not a red shell), turn it to cyan by adding green
					else if (renderfx & Q2RF_SHELL_BLUE)
						// go to green if it's on already, otherwise do cyan (flash green)
						if (renderfx & Q2RF_SHELL_GREEN)
							renderfx &= ~Q2RF_SHELL_BLUE;
						else
							renderfx |= Q2RF_SHELL_GREEN;
				}
			}*/
			// pmm
			ent.flags = renderfx | Q2RF_TRANSLUCENT;
			ent.alpha = 0.30;
			ent.fatness = 10;
			V_AddEntity (&ent);
		}

//		ent.skin = NULL;		// never use a custom skin on others
		ent.skinnum = 0;
		ent.flags = 0;
		ent.alpha = 0;

		// duplicate for linked models
		if (s1->modelindex2)
		{
			if (s1->modelindex2 == 255)
			{	// custom weapon
				ent.model=NULL;
				/*
				ci = &cl.clientinfo[s1->skinnum & 0xff];
				i = (s1->skinnum >> 8); // 0 is default weapon model
				if (!cl_vwep->value || i > MAX_CLIENTWEAPONMODELS - 1)
					i = 0;
				ent.model = ci->weaponmodel[i];
				if (!ent.model) {
					if (i != 0)
						ent.model = ci->weaponmodel[0];
					if (!ent.model)
						ent.model = cl.baseclientinfo.weaponmodel[0];
				}
				*/
			}
			else
				ent.model = cl.model_precache[s1->modelindex2];

			// PMM - check for the defender sphere shell .. make it translucent
			// replaces the previous version which used the high bit on modelindex2 to determine transparency
/*			if (!Q_strcasecmp (cl.model_name[(s1->modelindex2)], "models/items/shell/tris.md2"))
			{
				ent.alpha = 0.32;
				ent.flags = Q2RF_TRANSLUCENT;
			}
*/			// pmm

			V_AddEntity (&ent);

			//PGM - make sure these get reset.
			ent.flags = 0;
			ent.alpha = 0;
			//PGM
		}
		if (s1->modelindex3)
		{
			ent.model = cl.model_precache[s1->modelindex3];
			V_AddEntity (&ent);
		}
		if (s1->modelindex4)
		{
			ent.model = cl.model_precache[s1->modelindex4];
			V_AddEntity (&ent);
		}

		if ( effects & Q2EF_POWERSCREEN )
		{
/*			ent.model = cl_mod_powerscreen;
			ent.oldframe = 0;
			ent.frame = 0;
			ent.flags |= (Q2RF_TRANSLUCENT | Q2RF_SHELL_GREEN);
			ent.alpha = 0.30;
			V_AddEntity (&ent);
*/		}

		// add automatic particle trails
		if ( (effects&~Q2EF_ROTATE) )
		{
			if (effects & Q2EF_ROCKET)
			{
				CLQ2_RocketTrail (cent->lerp_origin, ent.origin, cent);
				V_AddLight (ent.origin, 200, 0.2, 0.2, 0);
			}
			// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER. 
			// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
			else if (effects & Q2EF_BLASTER)
			{
//				CLQ2_BlasterTrail (cent->lerp_origin, ent.origin);
//PGM
				if (effects & Q2EF_TRACKER)	// lame... problematic?
				{
					CLQ2_BlasterTrail2 (cent->lerp_origin, ent.origin);
					V_AddLight (ent.origin, 200, 0, 0.2, 0);		
				}
				else
				{
					CLQ2_BlasterTrail (cent->lerp_origin, ent.origin);
					V_AddLight (ent.origin, 200, 0.2, 0.2, 0);
				}
//PGM
			}
			else if (effects & Q2EF_HYPERBLASTER)
			{
				if (effects & Q2EF_TRACKER)						// PGM	overloaded for blaster2.
					V_AddLight (ent.origin, 200, 0, 0.2, 0);		// PGM
				else											// PGM
					V_AddLight (ent.origin, 200, 0.2, 0.2, 0);
			}
			else if (effects & Q2EF_GIB)
			{
				CLQ2_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & Q2EF_GRENADE)
			{
				CLQ2_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & Q2EF_FLIES)
			{
				CLQ2_FlyEffect (cent, ent.origin);
			}
			else if (effects & Q2EF_BFG)
			{
				static int bfg_lightramp[6] = {300, 400, 600, 300, 150, 75};

				if (effects & Q2EF_ANIM_ALLFAST)
				{
					CLQ2_BfgParticles (&ent);
					i = 200;
				}
				else
				{
					i = bfg_lightramp[s1->frame];
				}
				V_AddLight (ent.origin, i, 0, 0.2, 0);
			}
			// RAFAEL
			else if (effects & Q2EF_TRAP)
			{
				ent.origin[2] += 32;
				CLQ2_TrapParticles (&ent);
				i = (rand()%100) + 100;
				V_AddLight (ent.origin, i, 0.2, 0.16, 0.05);
			}
			else if (effects & Q2EF_FLAG1)
			{
				CLQ2_FlagTrail (cent->lerp_origin, ent.origin, 242);
				V_AddLight (ent.origin, 225, 0.2, 0.05, 0.05);
			}
			else if (effects & Q2EF_FLAG2)
			{
				CLQ2_FlagTrail (cent->lerp_origin, ent.origin, 115);
				V_AddLight (ent.origin, 225, 0.05, 0.05, 0.2);
			}
//======
//ROGUE
			else if (effects & Q2EF_TAGTRAIL)
			{
				CLQ2_TagTrail (cent->lerp_origin, ent.origin, 220);
				V_AddLight (ent.origin, 225, 0.2, 0.2, 0.0);
			}
			else if (effects & Q2EF_TRACKERTRAIL)
			{
				if (effects & Q2EF_TRACKER)
				{
					float intensity;

					intensity = 50 + (500 * (sin(cl.time/500.0) + 1.0));
					// FIXME - check out this effect in rendition
//					if(qrenderer == RQ_OPENGL)
						V_AddLight (ent.origin, intensity, -0.2, -0.2, -0.2);
//					else
//						V_AddLight (ent.origin, -1.0 * intensity, 0.2, 0.2, 0.2);
					}
				else
				{
					CLQ2_Tracker_Shell (cent->lerp_origin);
					V_AddLight (ent.origin, 155, -0.2, -0.2, -0.2);
				}
			}
			else if (effects & Q2EF_TRACKER)
			{
				CLQ2_TrackerTrail (cent->lerp_origin, ent.origin, 0);
				// FIXME - check out this effect in rendition
//				if(qrenderer == QR_OPENGL)
					V_AddLight (ent.origin, 200, -0.2, -0.2, -0.2);
//				else
//					V_AddLight (ent.origin, -200, 0.2, 0.2, 0.2);
			}
//ROGUE
//======
			// RAFAEL
			else if (effects & Q2EF_GREENGIB)
			{
				CLQ2_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);				
			}
			// RAFAEL
			else if (effects & Q2EF_IONRIPPER)
			{
				CLQ2_IonripperTrail (cent->lerp_origin, ent.origin);
				V_AddLight (ent.origin, 100, 0.2, 0.1, 0.1);
			}
			// RAFAEL
			else if (effects & Q2EF_BLUEHYPERBLASTER)
			{
				V_AddLight (ent.origin, 200, 0, 0, 0.2);
			}
			// RAFAEL
			else if (effects & Q2EF_PLASMA)
			{
				if (effects & Q2EF_ANIM_ALLFAST)
				{
					CLQ2_BlasterTrail (cent->lerp_origin, ent.origin);
				}
				V_AddLight (ent.origin, 130, 0.2, 0.1, 0.1);
			}
		}

		VectorCopy (ent.origin, cent->lerp_origin);
	}
}



/*
==============
CL_AddViewWeapon
==============
*/
void CLQ2_AddViewWeapon (q2player_state_t *ps, q2player_state_t *ops)
{
#if 1
	entity_t	gun;		// view model
	int			i;

	// allow the gun to be completely removed
	if (!r_drawviewmodel.value)
		return;

	// don't draw gun if in wide angle view
	if (ps->fov > 90)
		return;

	memset (&gun, 0, sizeof(gun));

//	if (gun_model)
//		gun.model = gun_model;	// development tool
//	else
		gun.model = cl.model_precache[ps->gunindex];
	if (!gun.model)
		return;

	gun.scale = 1;
	gun.alpha = 1;
	if (r_drawviewmodel.value < 1 || r_drawviewmodel.value > 0)
		gun.alpha = r_drawviewmodel.value;

	// set up gun position
	for (i=0 ; i<3 ; i++)
	{
		gun.origin[i] = cl.simorg[0][i] + ops->gunoffset[i]
			+ cl.lerpfrac * (ps->gunoffset[i] - ops->gunoffset[i]);
		gun.angles[i] = r_refdef.viewangles[i] + LerpAngle (ops->gunangles[i],
			ps->gunangles[i], cl.lerpfrac);
	}
	gun.angles[0]*=-1;

//	if (gun_frame)
//	{
//		gun.frame = gun_frame;	// development tool
//		gun.oldframe = gun_frame;	// development tool
//	}
//	else
	{
		gun.frame = ps->gunframe;
		if (gun.frame == 0)
			gun.oldframe = 0;	// just changed weapons, don't lerp from old
		else
			gun.oldframe = ops->gunframe;
	}

	gun.flags = Q2RF_MINLIGHT | Q2RF_DEPTHHACK | Q2RF_WEAPONMODEL;
	gun.lerptime = 1.0 - cl.lerpfrac;
	VectorCopy (gun.origin, gun.oldorigin);	// don't lerp at all
	V_AddEntity (&gun);
#endif
}


/*
===============
CL_CalcViewValues

Sets r_refdef view values
===============
*/
void CLQ2_CalcViewValues (void)
{
	int			i;
	float		lerp, backlerp;
	q2centity_t	*ent;
	q2frame_t		*oldframe;
	q2player_state_t	*ps, *ops;

	// find the previous frame to interpolate from
	ps = &cl.q2frame.playerstate;
	i = (cl.q2frame.serverframe - 1) & Q2UPDATE_MASK;
	oldframe = &cl.q2frames[i];
	if (oldframe->serverframe != cl.q2frame.serverframe-1 || !oldframe->valid)
		oldframe = &cl.q2frame;		// previous frame was dropped or involid
	ops = &oldframe->playerstate;

	// see if the player entity was teleported this frame
	if ( fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256*8
		|| abs(ops->pmove.origin[1] - ps->pmove.origin[1]) > 256*8
		|| abs(ops->pmove.origin[2] - ps->pmove.origin[2]) > 256*8)
		ops = ps;		// don't interpolate

	ent = &cl_entities[cl.playernum[0]+1];
	lerp = cl.lerpfrac;

	// calculate the origin
	if (cl.worldmodel->fromgame == fg_quake2 && (!cl_nopred.value) && !(cl.q2frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
	{	// use predicted values
		unsigned	delta;

		backlerp = 1.0 - lerp;
		for (i=0 ; i<3 ; i++)
		{
			r_refdef.vieworg[i] = cl.predicted_origin[i] + ops->viewoffset[i] 
				+ cl.lerpfrac * (ps->viewoffset[i] - ops->viewoffset[i])
				- backlerp * cl.prediction_error[i];
		}

		// smooth out stair climbing
		delta = realtime - cl.predicted_step_time;
		if (delta < 100)
			r_refdef.vieworg[2] -= cl.predicted_step * (100 - delta) * 0.01;
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			r_refdef.vieworg
			[i] = ops->pmove.origin[i]*0.125 + ops->viewoffset[i] 
				+ lerp * (ps->pmove.origin[i]*0.125 + ps->viewoffset[i] 
				- (ops->pmove.origin[i]*0.125 + ops->viewoffset[i]) );
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if (cl.worldmodel->fromgame == fg_quake2 && cl.q2frame.playerstate.pmove.pm_type < Q2PM_DEAD )
	{	// use predicted values
		for (i=0 ; i<3 ; i++)
			r_refdef.viewangles[i] = cl.predicted_angles[i];
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			r_refdef.viewangles[i] = LerpAngle (ops->viewangles[i], ps->viewangles[i], lerp);
	}

	for (i=0 ; i<3 ; i++)
		r_refdef.viewangles[i] += LerpAngle (ops->kick_angles[i], ps->kick_angles[i], lerp);

	VectorCopy(r_refdef.vieworg, cl.simorg[0]);
	VectorCopy(r_refdef.viewangles, cl.simangles[0]);
//	VectorCopy(r_refdef.viewangles, cl.viewangles);

//	AngleVectors (r_refdef.viewangles, v_forward, v_right, v_up);

	// interpolate field of view
	r_refdef.fov_x = ops->fov + lerp * (ps->fov - ops->fov);

	// don't interpolate blend color
	for (i=0 ; i<4 ; i++)
		v_blend[i] = ps->blend[i];

	// add the weapon
	CLQ2_AddViewWeapon (ps, ops);
}

/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CLQ2_AddEntities (void)
{
	if (cls.state != ca_active)
		return;


	r_refdef.currentplayernum = 0;


	cl_visedicts = cl_visedicts_list[cls.netchan.incoming_sequence&1];

	cl_numvisedicts = 0;

	if (cl.time*1000 > cl.q2frame.servertime)
	{
//		if (cl_showclamp.value)
//			Con_Printf ("high clamp %f\n", cl.time - cl.q2frame.servertime);
		cl.time = (cl.q2frame.servertime)/1000.0;
		cl.lerpfrac = 1.0;
	}
	else if (cl.time*1000 < cl.q2frame.servertime - 100)
	{
//		if (cl_showclamp.value)
//			Con_Printf ("low clamp %f\n", cl.q2frame.servertime-100 - cl.time);
		cl.time = (cl.q2frame.servertime - 100)/1000.0;
		cl.lerpfrac = 0;
	}
	else
		cl.lerpfrac = 1.0 - (cl.q2frame.servertime - cl.time*1000) * 0.01;

//	if (cl_timedemo.value)
//		cl.lerpfrac = 1.0;

//	CLQ2_AddPacketEntities (&cl.qwframe);
//	CLQ2_AddTEnts ();
//	CLQ2_AddParticles ();
//	CLQ2_AddDLights ();
//	CLQ2_AddLightStyles ();

	CLQ2_CalcViewValues ();
	// PMM - moved this here so the heat beam has the right values for the vieworg, and can lock the beam to the gun
	CLQ2_AddPacketEntities (&cl.q2frame);
#if 0
	CLQ2_AddProjectiles ();
#endif
	CL_UpdateTEnts ();
//	CLQ2_AddParticles ();
//	CLQ2_AddDLights ();
//	CLQ2_AddLightStyles ();
}

void CL_GetNumberedEntityInfo (int num, float *org, float *ang)
{
	q2centity_t	*ent;

	if (num < 0 || num >= MAX_EDICTS)
		Host_EndGame ("CL_GetNumberedEntityInfo: bad ent");
	ent = &cl_entities[num];

	if (org)
		VectorCopy (ent->current.origin, org);
	if (ang)
		VectorCopy (ent->current.angles, ang);


	// FIXME: bmodel issues...
}
#endif
