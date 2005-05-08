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
{	int	*pad[28];
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
	float	*parm1;
	float	*parm2;
	float	*parm3;
	float	*parm4;
	float	*parm5;
	float	*parm6;
	float	*parm7;
	float	*parm8;
	float	*parm9;
	float	*parm10;
	float	*parm11;
	float	*parm12;
	float	*parm13;
	float	*parm14;
	float	*parm15;
	float	*parm16;
	vec3_t	*V_v_forward;
	vec3_t	*V_v_up;
	vec3_t	*V_v_right;
	float	*trace_allsolid;
	float	*trace_startsolid;
	float	*trace_fraction;
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

typedef struct entvars_s
{
	float	modelindex;
	vec3_t	absmin;
	vec3_t	absmax;
	float	ltime;
	float	lastruntime;
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
	float	button1;
	float	button2;
	float	button3;	//3 and 1 are the same
	float	button4;
	float	button5;
	float	button6;
	float	button7;
	float	button8;
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
	float	sounds;
	string_t	noise;
	string_t	noise1;
	string_t	noise2;
	string_t	noise3;

	//extra vars. use these if you wish.
	float	gravity;
	float	maxspeed;
	float	items2;
	vec3_t	punchangle;
	float	scale;
	float	alpha;
	float	fatness;
	int		view2;
	float	fteflags;
	vec3_t	movement;
	float	vweapmodelindex;

	//dp extra fields
	int		nodrawtoclient;
	int		drawonlytoclient;

	//EXT_DIMENSION_VISIBLE
	float	dimension_see;
	float	dimension_seen;
	//EXT_DIMENSION_GHOST
	float	dimension_ghost;
	float	dimension_ghost_alpha;
	//EXT_DIMENSION_PHYSICS
	float	dimension_solid;
	float	dimension_hit;

	//udc_exeffect support. hacky I know.
	float	seefcolour;
	float	seefsizex;
	float	seefsizey;
	float	seefsizez;
	float	seefoffset;

	//hexen2 stuff
	float	playerclass;	//hexen2 requirements
	float	hull;
	float	drawflags;

	int		movechain;
	func_t	chainmoved;

	float	light_level;//hexen2's grabbing light level from client
	float	abslight;	//hexen2's force a lightlevel
	float	hasted;	//hexen2 uses this AS WELL as maxspeed


	//csqc stuph.
	func_t	SendEntity;
	float	Version;
} entvars_t;

