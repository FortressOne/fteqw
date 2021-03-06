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

/* file generated by qcc, do not modify */

typedef struct globalvars_s
{
	int null;
	union {
		vec3_t vec;
		float f;
		int i;
	} ret;
	union {
		vec3_t vec;
		float f;
		int i;
	} param[8];
} globalvars_t;

typedef struct nqglobalvars_s
{
	int	*self;
	int	*other;
	int	*world;
	float	*time;
	float	*frametime;	
	int		*newmis;
	float	*force_retouch;
	string_t	*mapname;
	float	*deathmatch;
	float	*coop;
	float	*teamplay;
	float	*serverflags;
	float	*total_secrets;
	float	*total_monsters;
	float	*found_secrets;
	float	*killed_monsters;
	vec3_t	*V_v_forward;
	vec3_t	*V_v_up;
	vec3_t	*V_v_right;
	float	*trace_allsolid;
	float	*trace_startsolid;
	float	*trace_fraction;
	float	*trace_surfaceflags;
	float	*trace_endcontents;
	vec3_t	*V_trace_endpos;
	vec3_t	*V_trace_plane_normal;
	float	*trace_plane_dist;
	int	*trace_ent;
	float	*trace_inopen;
	float	*trace_inwater;
	int	*msg_entity;
	func_t	*main;
	func_t	*StartFrame;
	func_t	*PlayerPreThink;
	func_t	*PlayerPostThink;
	func_t	*ClientKill;
	func_t	*ClientConnect;
	func_t	*PutClientInServer;
	func_t	*ClientDisconnect;
	func_t	*SetNewParms;
	func_t	*SetChangeParms;
	float *cycle_wrapped;
	float *dimension_send;
} nqglobalvars_t;

#define P_VEC(v) (pr_global_struct->V_##v)

typedef struct stdentvars_s //standard = standard for qw
{
	float	modelindex;
	vec3_t	absmin;
	vec3_t	absmax;
	float	ltime;
	int		lastruntime;	//type doesn't match the qc, we use a hidden double instead. this is dead.
	float	movetype;
	float	solid;
	vec3_t	origin;
	vec3_t	oldorigin;
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t	avelocity;
	string_t	classname;
	string_t	model;
	float	frame;
	float	skin;
	float	effects;
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	size;
	func_t	touch;
	func_t	use;
	func_t	think;
	func_t	blocked;
	float	nextthink;
	int	groundentity;
	float	health;
	float	frags;
	float	weapon;
	string_t	weaponmodel;
	float	weaponframe;
	float	currentammo;
	float	ammo_shells;
	float	ammo_nails;
	float	ammo_rockets;
	float	ammo_cells;
	float	items;
	float	takedamage;
	int	chain;
	float	deadflag;
	vec3_t	view_ofs;
	float	button0;
	float	button1;	//dead field in nq mode
	float	button2;
	float	impulse;
	float	fixangle;
	vec3_t	v_angle;
	string_t	netname;
	int	enemy;
	float	flags;
	float	colormap;
	float	team;
	float	max_health;
	float	teleport_time;
	float	armortype;
	float	armorvalue;
	float	waterlevel;
	float	watertype;
	float	ideal_yaw;
	float	yaw_speed;
	int	aiment;
	int	goalentity;
	float	spawnflags;
	string_t	target;
	string_t	targetname;
	float	dmg_take;
	float	dmg_save;
	int	dmg_inflictor;
	int	owner;
	vec3_t	movedir;
	string_t	message;	//WARNING: hexen2 uses a float and not a string
	float	sounds;
	string_t	noise;
	string_t	noise1;
	string_t	noise2;
	string_t	noise3;

#ifdef VM_Q1
} stdentvars_t;

typedef struct extentvars_s
{
#endif

	//extra vars. use these if you wish.
	float	maxspeed;	//added in quake 1.09
	float	gravity;	//added in quake 1.09 (for hipnotic)
	float	items2;		//added in quake 1.09 (for hipnotic)
	vec3_t	punchangle; //std in nq

	float	scale;	//DP_ENT_SCALE
	float	alpha;	//DP_ENT_ALPHA
	float	fatness;	//FTE_PEXT_FATNESS
	int		view2;	//FTE_PEXT_VIEW2
	float	fteflags;
	vec3_t	movement;	
	float	vweapmodelindex;

	//dp extra fields
	int		nodrawtoclient;		//
	int		drawonlytoclient;
	int		viewmodelforclient;	//DP_ENT_VIEWMODEL
	int		exteriormodeltoclient;
	float	button3;	//DP_INPUTBUTTONS (note in qw, we set 1 to equal 3, to match zquake/fuhquake/mvdsv)
	float	button4;
	float	button5;
	float	button6;
	float	button7;
	float	button8;

	float	viewzoom;	//DP_VIEWZOOM

	int		tag_entity;
	float	tag_index;

	float	glow_size;
	float	glow_color;
	float	glow_trail;

	vec3_t	color;
	vec3_t	colormod;
	float	light_lev;
	float	style;
	float	pflags;
	float	clientcolors;

	//EXT_DIMENSION_VISIBLE
	float	dimension_see;
	float	dimension_seen;
	//EXT_DIMENSION_GHOST
	float	dimension_ghost;
	float	dimension_ghost_alpha;
	//EXT_DIMENSION_PHYSICS
	float	dimension_solid;
	float	dimension_hit;

	//hexen2 stuff
	float	playerclass;	//hexen2 requirements
	float	hull;
	float	drawflags;

	int	movechain;
	func_t	chainmoved;

	float	light_level;//hexen2's grabbing light level from client
	float	abslight;	//hexen2's force a lightlevel
	float	hasted;	//hexen2 uses this AS WELL as maxspeed

	//csqc stuph.
	func_t	SendEntity;
	float	Version;

#ifdef VM_Q1
} extentvars_t;
#else
} stdentvars_t;
#endif

