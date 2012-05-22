/*
this file handles the local player, and marshalling between the different sorts of player models.

RefreshPlayer: Called from the engine each time a player (ent with player.mdl) is about to be drawn
ParsePlayer: Called from CSQC_Ent_Parse for csqc protocol ents
*/

#include "playerframes.inc"

static void() RemovePlayer;

enum
{
	MF_BAD,
#ifdef MD3PMODELS
	MF_QUAKE3,
#endif
#ifdef Q4PMODELS
	MF_QUAKE4,
#endif
#ifdef HLPMODELS
	MF_HLPM,
#endif
#ifdef XONPMODELS
	MF_XON,
#endif
	MF_QUAKE
};


.vector oldangles;
.vector oldorigin;
.vector newangles;
.vector neworigin;
.float updatetime;
.float modelclass;


typedef struct
{
	string name;
	struct
	{
		float firstframe, framecount;
	} anim[ACT_COUNT];
} q1modellist_t;

q1modellist_t q1models[GIB_MAX] =
{
	{
		"progs/player.mdl",
		{
			{17,   12},	//axstand1
			{0,    6},		//axrun1
			{0,    6},		//axrun1(again)
			{117,  6},		//axatt1
			{122,  6},		//axattb1
			{128,  6},		//axattc1
			{134,  6},		//axattd1
			{29,   6},		//axpain1
			{29,   6},		//axpain1(again)
			{41,   9},		//axdeth1
			{41,   9},		//axdeth1
			{41,   9},		//axdeth1
			{41,   9},		//axdeth1
			{41,   9},		//axdeth1
		}
	},
	{
		"progs/player.mdl",
		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
	},
	{
		"progs/soldier.mdl",
		{
			{0,    8},		//stand1
			{90,   24},	//walk1
			{73,   8},		//run1
			{81,   9},		//atk1
			{81,   9},		//atk1(again)
			{81,   9},		//atk1(again2)
			{81,   9},		//atk1(again3) - should this be the reload anim just for the sake of it?
			{40,   6},		//pain1
			{46,   14},	//painb1
//			{60,   13},	//painc1
			{8,    10},	//die1
			{18,   11},	//cdie1
			{8,    10},	//die1(again)
			{18,   11},	//cdie1(again)
			{8,    10},	//die1(again2)

		}

	},
	{
		"progs/zombie.mdl"
		/*strictly speaking, zombies do not have a death animation, as they only ever gib on death, but they do have a pain animation where they lie down.*/
		{
			{0,   15},		//stand1
			{15,  19},		//walk1
			{34,  18},		//run1
			{52,  13},		//atta1
			{65,  14},		//attb1
			{79,  12},		//attc1
			{52,  13},		//atta1(again)
			{91,  12},		//paina1
			{103, 28},		//painb1
//			{131, 18},		//painc1
//			{149, 13},		//paind1
//			{162, 30},		//paine1
			{162,  11},	//paine1-paine11(again)
			{162,  11},	//paine1-paine11(again)
			{162,  11},	//paine1-paine11(again)
			{162,  11},	//paine1-paine11(again)
			{162,  11},	//paine1-paine11(again)
		}
	},
	{
		"progs/demon.mdl",
		{
			{0,   13},		//stand1
			{13,   8},		//walk1
			{21,   6},		//run1
			{54,  15},		//swipe
			{27,  10},		//leap
			{54,  15},		//swipe(again)
			{27,  10},		//leap(again)
			{39,   6},		//pain1
			{39,   6},		//pain1(again)
			{45,   9},		//death1
			{45,   9},		//death1
			{45,   9},		//death1
			{45,   9},		//death1
			{45,   9},		//death1
		}
	},
	{
		"progs/ogre.mdl"
/*		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
*/
	},
	{
		"progs/dog.mdl"
/*		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
*/
	},
	{
		"progs/wizard.mdl"
/*		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
*/
	},
	{
		"progs/shambler.mdl"
/*		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
*/
	},
	{
		"progs/knight.mdl"
/*		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
*/
	},
	{
		"progs/hellknight.mdl"
/*		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
*/
	},
	{
		"progs/fish.mdl"
/*		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
*/
	},
	{
		"progs/enforcer.mdl",
		{
			{0,   7},		//stand1
			{7,   16},		//walk1
			{23,   8},		//run1
			{31,  10},		//atk
			{31,  10},		//atk(again)
			{31,  10},		//atk(again)
			{31,  10},		//atk(again)
			{66,   4},		//paina1
			{70,   5},		//painb1
//			{75,   8},		//painc1
//			{83,  19},		//paind1
			{41,  14},		//death1
			{55,  11},		//fdie1
			{41,  14},		//death1(again)
			{55,   11},	//fdie1(again)
			{41,  14},		//death1(again)
		}
	},
	{
		"progs/shalrath.mdl"
/*		{
			{12,   5},		//stand1
			{6,    6},		//rockrun1
			{6,    6},		//rockrun1(again)
			{103,  2},		//nailatt1
			{105,  2},		//light1
			{107,  6},		//rockatt1
			{113,  6},		//shotatt1
			{35,   6},		//pain1
			{35,   6},		//pain1(again)
			{50,   11},	//deatha1
			{61,   9},		//deathb1
			{70,   15},	//deathc1
			{85,   9},		//deathd1
			{93,   9},		//deathe1
		}
*/
	}
};

#pragma target fte
static void() Player_Interpolate =
{	//do some frame interpolation.
	if (self.entnum == player_localentnum)
	{
		Pred_UpdateLocalMovement(self);
	}
	else if (self.modelclass > GIB_PLAYER)
	{	/*non-player stepping monsters interpolate*/
		float frac = (time - self.updatetime) / 0.1;
		frac = bound(0, frac, 1);
		self.origin = self.neworigin * frac + self.oldorigin * (1-frac);
		//FIXME: this linearly interpolates, which is bad if that means its interpolating from 360 to 0. spinny!
		self.angles = self.newangles * frac + self.oldangles * (1-frac);
	}

	switch(self.modelstyle)
	{
#ifdef MD3PMODELS
	case MF_QUAKE3:
		Anim_Draw();
		break;
#endif
#ifdef Q4PMODELS
	case MF_QUAKE4:
		Q4PM_Draw();
		break;
#endif
#ifdef HLPMODELS
	case MF_HLPM:
		HLPM_Draw();
		break;
#endif
#ifdef XONPMODELS
	case MF_XON:
		XON_Draw();
		break;
#endif
	default:
		self.lerpfrac -= frametime*10;
		while (self.lerpfrac < 0)
		{
			float ff = q1models[self.modelclass].anim[self.action].firstframe;
			float fc = q1models[self.modelclass].anim[self.action].framecount;
			self.frame2 = self.frame;
			self.frame += 1;
			if (self.frame < ff || self.frame >= (ff+fc))
			{
				if (self.action >= ACT_DIE1 && self.frame == (ff+fc))	//death animations do not loop
					self.frame = ff+fc-1;
				else
					self.frame = ff;
			}
			self.lerpfrac += 1;
		}

		if (chasecam || self.entnum != player_localentnum)
			self.renderflags = 0;
		else
			self.renderflags = RF_EXTERNALMODEL;

		if (self.sveffects & SVE_INVIS)
			setmodel(self, "progs/eyes.mdl");
		else
			setmodel(self, q1models[self.modelclass].name);

		break;
	}
};

static void(float g) Player_SetLocalInfoGender =
{
	if (player_local == self)
	{
		//if it was forced, don't lie to everyone else.
		if (CVARS(cg_forceskin) != "")
			return;

		switch(g)
		{
		case GENDER_FEMALE:
			localcmd("setinfo s f\n");
			break;
		case GENDER_MALE:
			localcmd("setinfo s \"\"\n");
			break;
		default:
		case GENDER_NEUTER:
			localcmd("setinfo s n\n");
			break;
		}
	}
}

static void() Player_SetQ1Model =
{
	self.modelstyle = MF_QUAKE;
	self.drawmask = MASK_NORMAL;	
	setmodel(self, "progs/player.mdl");
	Player_SetLocalInfoGender(GENDER_MALE);
}

#ifdef MD3PMODELS
static void(string skinname) Player_SetQ3Model =
{
	if (!Anim_SetModel(skinname))
	{
		Anim_UnsetModel();
		return;
	}
	Player_SetLocalInfoGender(Anim_GetGender());
	self.modelstyle = MF_QUAKE3;
};
#endif

#ifdef Q4PMODELS
static void(string skinname) Player_SetQ4Model =
{
	if (!Q4PM_SetModel(skinname))
	{
		Q4PM_UnsetModel();
		return;
	}
	Player_SetLocalInfoGender(GENDER_MALE);
	self.modelstyle = MF_QUAKE4;
};
#endif

#ifdef HLPMODELS
static void(string skinname) Player_SetHLModel =
{
	if (!HLPM_SetModel(skinname))
	{
		HLPM_UnsetModel();
		return;
	}
	Player_SetLocalInfoGender(GENDER_MALE);
	self.modelstyle = MF_HLPM;
};
#endif

#ifdef XONPMODELS
static void(string skinname) Player_SetXONModel =
{
	if (!XON_SetModel(skinname))
	{
		XON_UnsetModel();
		return;
	}
	Player_SetLocalInfoGender(GENDER_MALE);
	self.modelstyle = MF_XON;
};
#endif

static void() Player_UnsetModel =
{
	switch(self.modelstyle)
	{
#ifdef MD3PMODELS
	case MF_QUAKE3:
		Anim_UnsetModel();
		break;
#endif
#ifdef Q4PMODELS
	case MF_QUAKE4:
		Q4PM_UnsetModel();
		break;
#endif
#ifdef HLPMODELS
	case MF_HLPM:
		HLPM_UnsetModel();
		break;
#endif
#ifdef XONPMODELS
	case MF_XON:
		XON_UnsetModel();
		break;
#endif
	default:
		//nothing to do
		break;
	}
	self.modelstyle = MF_BAD;
	setmodel(self, "");
};

static entity() Player_DupModel =
{
	local entity e;

	switch(self.modelstyle)
	{
#ifdef MD3PMODELS
	case MF_QUAKE3:
		e = Anim_DupModel();
		break;
#endif
#ifdef Q4PMODELS
	case MF_QUAKE4:
		e = Q4PM_DupModel();
		break;
#endif
#ifdef HLPMODELS
	case MF_HLPM:
		e = HLPM_DupModel();
		break;
#endif
#ifdef XONPMODELS
	case MF_XON:
		e = XON_DupModel();
		break;
#endif
	default:
		e = spawn();
		break;
	}
	e.modelstyle = self.modelstyle;
	return e;
};

.float starttime;
.float oldframe;
static void() bodythink =
{
	local float final;
	if (self.frame >= $axdeth1 && self.frame <= $axdeth9)
		final = $axdeth9;
	else if (self.frame >= $deatha1 && self.frame <= $deatha11)
		final = $deatha11;
	else if (self.frame >= $deathb1 && self.frame <= $deathb9)
		final = $deathb9;
	else if (self.frame >= $deathc1 && self.frame <= $deathc15)
		final = $deathc15;
	else if (self.frame >= $deathd1 && self.frame <= $deathd9)
		final = $deathd9;
	else if (self.frame >= $deathe1 && self.frame <= $deathe9)
		final = $deathe9;

	self.frame = self.frame+1;

	if (self.frame > final)
		self.frame = final;

	self.nextthink = time + 0.1;
};
static void() DeadBodyPredraw =
{
	float ftime;
	ftime = time - self.starttime;

	if (self.removetime < time)
	{
		Player_UnsetModel();
		remove(self);
		return;
	}

	Movetype_Bounce();

	Player_Interpolate();
	self.origin_z -= ftime*0.5;
};

static void() JustRessed =
{
	local entity e;

	e = Player_DupModel();
	e.frame = self.oldframe;	//and stay dead!
	e.frame2 = self.oldframe;	//and stay dead!
	setmodel(e, "progs/player.mdl");
	e.origin = self.lastorg;
	e.velocity = self.lastvel;
	e.predraw = DeadBodyPredraw;
	e.removetime = time + 30;
	e.lerptime = time;

	e.gibbable = GIB_PLAYER;
	e.solid = SOLID_TRIGGER;

	e.classname = "deadbody";
	e.movetype = MOVETYPE_TOSS;

	e.starttime = time;
	e.nextthink = time + 0.1;
	e.think = bodythink;
	e.drawmask = self.drawmask;
};

static void() RemovePlayer =
{
	if (player_local == self)
		player_local = world;
		
	if (self.haddied)
	{
		JustRessed();
		self.haddied = false;
	}

	Player_UnsetModel();

#ifndef NOEXTENSIONS
	if (self.oldskin)
		strunzone(self.oldskin);
	self.oldskin = string_null;
#endif

	self.predraw = __NULL__;
};

float(float isnew) RefreshPlayer =
{
	local string newskin;

	self.predraw = Player_Interpolate;

	if (isnew)
		self.haddied = false;

	//if its not new and they're not dead, but they were before..
	if (self.action < ACT_DIE1 || self.action > ACT_DIE5)
	{
		if (self.haddied)
		{
			JustRessed();
			self.haddied = false;
		}
		self.solid = SOLID_BBOX;
	}
	else
	{
		self.haddied = true;
		self.solid = SOLID_NOT;
	}

	//for interpolation and prediction
	self.lastorg = self.origin;
	self.lastvel = self.velocity;

	//only for interpolation
	self.oldorigin = self.neworigin;
	self.oldangles = self.newangles;
	self.newangles = self.angles;
	self.neworigin = self.origin;
	self.updatetime = time;

	if (isnew)
	{
		self.oldorigin = self.neworigin;
		self.oldangles = self.newangles;

		if (self.entnum == player_localentnum)
		{
			player_local = self;
		}

		self.modelstyle = MF_BAD;

		self.oldskin = string_null;
		self.drawmask = MASK_NORMAL;

		self.removefunc = RemovePlayer;
	}

	if (self.sveffects & SVE_INVIS)
	{
		if (self.modelstyle != MF_QUAKE)
		{
			Player_UnsetModel();
			Player_SetQ1Model();	//every model uses eyes
		}
	}
	else
	{
#ifndef NOEXTENSIONS
		newskin = CVARS(cg_forceskin);
		if (newskin == "")
			newskin = getplayerkeyvalue(self.entnum-1, "model");
		if (newskin == "")
			newskin = getplayerkeyvalue(self.entnum-1, "skin");
		if (newskin != self.oldskin)
		{
			Player_UnsetModel();

			if (self.oldskin)
				strunzone(self.oldskin);
			if (newskin == string_null)
				self.oldskin = string_null;
			else
				self.oldskin = strzone(newskin);
		}
#else
#ifdef MD3PMODELS
		self.oldskin = "ranger";
#else
		self.oldskin = string_null;
#endif
#endif
		if (self.modelstyle == MF_BAD)
		{
			newskin = self.oldskin;

#ifdef MD3PMODELS
			if (self.modelstyle == MF_BAD && newskin != "")
				Player_SetQ3Model(newskin);
#endif
#ifdef Q4PMODELS
			if (self.modelstyle == MF_BAD && newskin != "")
				Player_SetQ4Model(newskin);
#endif
#ifdef HLPMODELS
			if (self.modelstyle == MF_BAD && newskin != "")
				Player_SetHLModel(newskin);
#endif
#ifdef XONPMODELS
			if (self.modelstyle == MF_BAD && newskin != "")
				Player_SetXONModel(newskin);
#endif
			if (self.modelstyle == MF_BAD)
			{
				if (newskin != "")
					print("Failed to load skin \"", newskin, "\"\n");
				Player_SetQ1Model();
			}
		}
	}

	setsize(self, VEC_HULL_MIN, VEC_HULL_MAX);

	self.oldframe = self.frame;

	if (player_local == self)
		Pred_PlayerUpdated(self);

	return TRUE;
};

#include "playerframes.inc"
/*called when we are not using csqctest's ssqc*/
float(float isnew) RefreshPlayerSSQCLess =
{
	switch(self.frame)
	{
	case $axrun1 .. $axrun6 :
	case $rockrun1 .. $rockrun6 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_RUN;
		break;
	case $stand1 .. $stand5 :
	case $axstnd1 .. $axstnd12 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_IDLE;
		break;
	case $axstnd1 .. $axstnd12 :
	case $axpain1 .. $axpain6 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_PAIN1;
		break;

	case $axdeth1.. $axdeth9 :
	case $deatha1.. $deatha11 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_DIE1;
		break;
	case $deathb1.. $deathb9 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_DIE2;
		break;
	case $deathc1.. $deathc15 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_DIE3;
		break;
	case $deathd1.. $deathd9 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_DIE4;
		break;
	case $deathe1.. $deathe9 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_DIE5;
		break;

	case $nailatt1.. $nailatt2 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_ATTACK1;
		break;
	case $light1.. $light2 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_ATTACK2;
		break;
	case $rockatt1.. $rockatt6 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_ATTACK3;
		break;
	case $shotatt1.. $shotatt6 :
		self.modelclass = GIB_PLAYER;
		self.action = ACT_ATTACK4;
		break;

	case $axatt1.. $axatt6 :
		self.action = ACT_ATTACK1;
		break;
	case $axattb1.. $axattb6 :
		self.action = ACT_ATTACK2;
		break;
	case $axattc1.. $axattc6 :
		self.action = ACT_ATTACK3;
		break;
	case $axattd1.. $axattd6 :
		self.action = ACT_ATTACK4;
		break;
	}
	RefreshPlayer(isnew);
};

//this is sent after the server has run our movement command.
nonstatic void(float isnew) ParsePlayer =
{
	local float f;

	f = readbyte();

	if (f & SF_INITIAL)
		self.modelclass = readbyte();
	if (f & SF_ACTION)
		self.action = readbyte();
	if (f & SF_ANGLES)
	{
		self.angles_x = readbyte()*(360/256);
		self.angles_y = readbyte()*(360/256);
	}
	if (f & SF_ORIGIN)
	{
		self.origin_x = readcoord();
		self.origin_y = readcoord();
		self.origin_z = readcoord();
	}
	if (f & SF_VELOCITY)
	{
		self.velocity_x = readshort();
		self.velocity_y = readshort();
		self.velocity_z = readshort();
	}
	if (f & SF_FLAGS)
		self.sveffects = readbyte();

	if (self.modelclass > GIB_PLAYER)
		self.colormap = 0;
	else
		self.colormap = self.entnum;

	RefreshPlayer(isnew);
};

