.string target;
.string targetname;
.vector mangle;
.string message;

string() getentitytoken = #355;

void() cs_teleport_touch =
{
	local entity t;
	t = find(world, targetname, self.target);
	if (t)
	{
		makevectors(t.angles);
		Pred_Predict_Teleport(t.origin + '0 0 27', v_forward*300, t.angles);
	}
};

void() spawn_trigger_teleport =
{
	self.solid = SOLID_TRIGGER;
	setmodel(self, self.model);
	self.model = "";

	self.touch = cs_teleport_touch;
};

void() spawn_info_teleport_destination =
{
};

void() spawn_trigger_push =
{
	self.solid = SOLID_TRIGGER;
	setmodel(self, self.model);
	self.model = "";

	self.touch = cs_teleport_touch;
};

float(float isnew) RefreshPusher =
{
	if (self.origin != self.oldorigin || isnew)
	{
		self.oldorigin = self.origin;
		setorigin(self, self.origin);
		self.drawmask = MASK_NORMAL;
		self.solid = SOLID_BSP;
		self.movetype = MOVETYPE_PUSH;
	}
};

void() spawn_pusher =
{
	deltalisten(self.model, RefreshPusher, RSES_NOROTATE);
	remove(self);
};

void() spawn_worldspawn =
{
	levelname = self.message;
	remove(self);
};

.float spawnflags;
float(float inhibitspawnflags) parsenewmapentity =
{
	local string field, value;
	local entity nent;
	local void() spawnfunc;
	local float inhibitspawnflags;
	nent = spawn();

	while (1)
	{
		field = getentitytoken();
		if not (field)
			break;

		if (field == "}")
		{
			if (!nent.classname)
				break;

			if (nent.spawnflags & inhibitspawnflags)
			{
				/*do nothing, just make sure the following blocks can't trigger*/
				//print("inhibit ", nent.classname, "\n");
			}
			else if (spawnfunc)
			{
				self = nent;
				spawnfunc();
				return true;
			}
			else
			{
//				print("ignoring ", nent.classname, "\n");
			}

			//I don't know what you are, go away.
			if (nent.classname!="")
				strunzone(nent.classname);
			if (nent.message!="")
				strunzone(nent.message);
			remove(nent);
			return true;
		}

		value = getentitytoken();
		if not (value)
			break;

//		print("ent field ", field, " has value ", value, "\n");

		if (field == "classname")
		{
			if (value == "trigger_teleport")
				spawnfunc = spawn_trigger_teleport;
			if (value == "info_teleport_destination")
				spawnfunc = spawn_info_teleport_destination;
			if (value == "trigger_push")
				spawnfunc = spawn_trigger_push;
			if (value == "worldspawn")
				spawnfunc = spawn_worldspawn;
			if (value == "func_door")
				spawnfunc = spawn_pusher;
			if (value == "func_door_secret")
				spawnfunc = spawn_pusher;
			if (value == "func_wall")
				spawnfunc = spawn_pusher;
			if (value == "func_episodegate")
				spawnfunc = spawn_pusher;
			if (value == "func_bossgate")
				spawnfunc = spawn_pusher;
			if (value == "func_plat")
				spawnfunc = spawn_pusher;
			nent.classname = strzone(value);
		}
		else if (field == "targetname")
			nent.targetname = strzone(value);
		else if (field == "target")
			nent.target = strzone(value);
		else if (field == "origin")
			nent.origin = stov(value);
		else if (field == "angles")
			nent.angles = stov(value);
		else if (field == "angle")
		{
			nent.angles_x = 0;
			nent.angles_y = stof(value);
			nent.angles_z = 0;
		}
		else if (field == "mangle")
			nent.angles = stov(value);
		else if (field == "model")
			nent.model = value;
		else if (field == "message")
			nent.message = strzone(value);
		else if (field == "spawnflags")
			nent.spawnflags = stof(value);
/*		else if (field == "light");
		else if (field == "mangle");
		else if (field == "killtarget");
		else if (field == "wad");
		else if (field == "height");
		else if (field == "lip");
		else if (field == "count");
		else if (field == "dmg");
		else if (field == "delay");
		else if (field == "worldtype");
		else if (field == "sounds");
		else if (field == "map");
		else if (field == "wait");
		else if (field == "style");
		else if (field == "health");
		else if (field == "speed");
		else
			print("ignoring field ", field, "\n");
*/
	}

	remove(nent);

	return false;
};

void() CSQC_WorldLoaded =
{
	local string tok;
	local float inhibitspawnflags;

	/*this kinda needs to match the server*/
	if (stof(serverkey("deathmatch")))
		inhibitspawnflags = 2048/*SPAWNFLAG_NOT_DEATHMATCH*/;
	else
	{
		inhibitspawnflags = stof(serverkey("skill"));
		if (inhibitspawnflags < 0.5)
			inhibitspawnflags = 256/*SPAWNFLAG_NOT_EASY*/;
		else if (inhibitspawnflags > 1.5)
			inhibitspawnflags = 1024/*SPAWNFLAG_NOT_HARD*/;
		else
			inhibitspawnflags = 512/*SPAWNFLAG_NOT_MEDIUM*/;
	}

	while (1)
	{
		tok = getentitytoken();

		if (tok == "")
			break;

		if (tok != "{")
		{
			print("bad entity data\n");
			return;
		}
		
		if (!parsenewmapentity(inhibitspawnflags))
		{
			print("bad entity data\n");
			return;
		}
	}
};
