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
// cvar.h

/*

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

it is sufficient to initialize a cvar_t with just the first two fields, or
you can add a ,true flag for variables that you want saved to the configuration
file when the game is quit:

cvar_t	r_draworder = {"r_draworder","1"};
cvar_t	scr_screensize = {"screensize","1",true};

Cvars must be registered before use, or they will have a 0 value instead of the float interpretation of the string.  Generally, all cvar_t declarations should be registered in the apropriate init function before any console commands are executed:
Cvar_RegisterVariable (&host_framerate);


C code usually just references a cvar in place:
if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or
cvar_set (name, value) internal functions:
teamplay = cvar("teamplay");
cvar_set ("registered", "1");

The user can access cvars from the console in two ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/

typedef struct cvar_s
{
	//must match q2's definition
	char		*name;
	char		*string;
	char		*latched_string;	// for CVAR_LATCH vars
	int			flags;
	int			modified;	// increased each time the cvar is changed
	float		value;
	struct cvar_s *next;

	//free style :)
	char		*name2;

	char		*defaultstr;	//default
	qbyte		restriction;
} cvar_t;

#define FCVAR(ConsoleName,ConsoleName2,Value,Flags) {ConsoleName, Value, NULL, Flags, 0, 0, 0, ConsoleName2}
#define SCVARF(ConsoleName,Value, Flags) FCVAR(ConsoleName, NULL, Value, Flags)
#define SCVAR(ConsoleName,Value) FCVAR(ConsoleName, NULL, Value, 0)

typedef struct cvar_group_s
{
	const char *name;
	struct cvar_group_s *next;

	cvar_t *cvars;
} cvar_group_t;

//q2 constants
#define	CVAR_ARCHIVE		1	// set to cause it to be saved to vars.rc
#define	CVAR_USERINFO		2	// added to userinfo  when changed
#define	CVAR_SERVERINFO		4	// added to serverinfo when changed
#define	CVAR_NOSET			8	// don't allow change from console at all,
							// but can be set from the command line
#define	CVAR_LATCH			16	// save changes until server restart

//freestyle
#define CVAR_POINTER		32	// q2 style. May be converted to q1 if needed. These are often specified on the command line and then converted into q1 when registered properly.
#define CVAR_NOTFROMSERVER	64	// the console will ignore changes to cvars if set at from the server or any gamecode. This is to protect against security flaws - like qterm
#define CVAR_USERCREATED	128	//write a 'set' or 'seta' in front of the var name.
#define CVAR_CHEAT			256	//latch to the default, unless cheats are enabled.
#define CVAR_SEMICHEAT		512	//if strict ruleset, force to 0/blank.
#define CVAR_RENDERERLATCH	1024	//requires a vid_restart to reapply.
#define CVAR_SERVEROVERRIDE 2048	//the server has overridden out local value - should probably be called SERVERLATCH

#define CVAR_LASTFLAG CVAR_SERVEROVERRIDE

#define CVAR_LATCHMASK		(CVAR_LATCH|CVAR_RENDERERLATCH|CVAR_SERVEROVERRIDE|CVAR_CHEAT|CVAR_SEMICHEAT)	//you're only allowed one of these.
#define CVAR_NEEDDEFAULT	CVAR_CHEAT

cvar_t *Cvar_Get (const char *var_name, const char *value, int flags, const char *groupname);

void Cvar_LockFromServer(cvar_t *var, const char *str);

qboolean 	Cvar_Register (cvar_t *variable, const char *cvargroup);
// registers a cvar that already has the name, string, and optionally the
// archive elements set.

//#define Cvar_RegisterVariable(x) Cvar_Register(x,__FILE__);

cvar_t	*Cvar_ForceSet (cvar_t *var, const char *value);
cvar_t 	*Cvar_Set (cvar_t *var, const char *value);
// equivelant to "<name> <variable>" typed at the console

void	Cvar_SetValue (cvar_t *var, float value);
// expands value to a string and calls Cvar_Set

void Cvar_ApplyLatches(int latchflag);
//sets vars to thier latched values

float	Cvar_VariableValue (const char *var_name);
// returns 0 if not defined or non numeric

char	*Cvar_VariableString (const char *var_name);
// returns an empty string if not defined

char 	*Cvar_CompleteVariable (const char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

qboolean Cvar_Command (int level);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_WriteVariables (vfsfile_t *f, qboolean all);
// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.

cvar_t *Cvar_FindVar (const char *var_name);

void Cvar_Shutdown(void);

void Cvar_ForceCheatVars(qboolean semicheats, qboolean absolutecheats);	//locks/unlocks cheat cvars depending on weather we are allowed them.

//extern cvar_t	*cvar_vars;

// cvar const cache, used for removing fairly common default cvar values
#define CC_CACHE_ENTRIES 8

#define CC_CACHE_SIZE 2048
#define CC_CACHE_STEP 2048

typedef struct cvar_const_cache_s {
	char *cached[CC_CACHE_ENTRIES];
	const_block_t *cb;
} cvar_const_cache_t;
