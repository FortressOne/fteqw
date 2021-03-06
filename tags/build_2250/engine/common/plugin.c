//This file should be easily portable.
//The biggest strength of this plugin system is that ALL interactions are performed via
//named functions, this makes it *really* easy to port plugins from one engine to annother.

#include "quakedef.h"

#ifdef PLUGINS

cvar_t plug_sbar = {"plug_sbar", "1"};
cvar_t plug_loaddefault = {"plug_loaddefault", "1"};

#include "glquake.h"

typedef struct plugin_s {
	char *name;
	vm_t *vm;
	int tick;
	int executestring;
	int conexecutecommand;
	int menufunction;
	int sbarlevel[3];	//0 - main sbar, 1 - supplementry sbar sections (make sure these can be switched off), 2 - overlays (scoreboard). menus kill all.
	int reschange;

	struct plugin_s *next;
} plugin_t;

plugin_t *currentplug;

//custom plugin builtins.
typedef int (*Plug_Builtin_t)(void *offset, unsigned int mask, const long *arg);
void Plug_RegisterBuiltin(char *name, Plug_Builtin_t bi, int flags);
#define PLUG_BIF_DLLONLY	1
#define PLUG_BIF_QVMONLY	2

void Plug_Init(void);

void Plug_Tick(void);
qboolean Plugin_ExecuteString(void);
void Plug_Shutdown(void);


static plugin_t *plugs;
static plugin_t *menuplug;	//plugin that has the current menu


typedef struct {
	char *name;
	Plug_Builtin_t func;
} Plug_Plugins_t;
Plug_Plugins_t *plugbuiltins;
int numplugbuiltins;

void Plug_RegisterBuiltin(char *name, Plug_Builtin_t bi, int flags)
{
	//randomize the order a little.
	int newnum;
	
	newnum = rand()%128;
	while(newnum < numplugbuiltins && plugbuiltins[newnum].func)
		newnum+=128;

	if (newnum >= numplugbuiltins)
	{
		numplugbuiltins = newnum+128;
		plugbuiltins = BZ_Realloc(plugbuiltins, sizeof(Plug_Plugins_t)*numplugbuiltins);
	}

	//got an empty number.
	plugbuiltins[newnum].name = name;
	plugbuiltins[newnum].func = bi;
}

/*
static void Plug_RegisterBuiltinIndex(char *name, Plug_Builtin_t bi, int flags, int index)	//I d
{
	//randomize the order a little.
	int newnum;
	
	newnum = rand()%128;
	while(newnum+1 < numplugbuiltins && plugbuiltins[newnum+1].func)
		newnum+=128;

	newnum++;

	if (newnum >= numplugbuiltins)
	{
		numplugbuiltins = newnum+128;
		plugbuiltins = BZ_Realloc(plugbuiltins, sizeof(Plug_Plugins_t)*numplugbuiltins);
	}

	//got an empty number.
	plugbuiltins[newnum].name = name;
	plugbuiltins[newnum].func = bi;
}
*/

int Plug_FindBuiltin(void *offset, unsigned int mask, const long *args)
{
	int i;
	for (i = 0; i < numplugbuiltins; i++)
		if (plugbuiltins[i].name)
			if (!strcmp(plugbuiltins[i].name, (char *)VM_POINTER(args[0])))
				return -i;

	return 0;
}

long Plug_SystemCallsEx(void *offset, unsigned int mask, int fn, const long *arg)
{
	fn = fn+1;

	if (fn>=0 && fn < numplugbuiltins && plugbuiltins[fn].func!=NULL)
		return plugbuiltins[fn].func(offset, mask, arg);

	Sys_Error("QVM Plugin tried calling invalid builtin %i", fn);
	return 0;
}

#ifdef _DEBUG
static long Plug_SystemCallsExWrapper(void *offset, unsigned int mask, int fn, const long *arg)
{	//this is so we can use edit and continue properly (vc doesn't like function pointers for edit+continue)
	return Plug_SystemCallsEx(offset, mask, fn, arg);
}
#define Plug_SystemCallsEx Plug_SystemCallsExWrapper
#endif

//I'm not keen on this.
//but dlls call it without saying what sort of vm it comes from, so I've got to have them as specifics
static int EXPORT_FN Plug_SystemCalls(int arg, ...)
{
	long args[9];
	va_list argptr;

	va_start(argptr, arg);
	args[0]=va_arg(argptr, int);
	args[1]=va_arg(argptr, int);
	args[2]=va_arg(argptr, int);
	args[3]=va_arg(argptr, int);
	args[4]=va_arg(argptr, int);
	args[5]=va_arg(argptr, int);
	args[6]=va_arg(argptr, int);
	args[7]=va_arg(argptr, int);
	args[8]=va_arg(argptr, int);
	va_end(argptr);

	arg = -arg;

	if (arg>=0 && arg < numplugbuiltins && plugbuiltins[arg].func)
		return plugbuiltins[arg].func(NULL, ~0, args);

	Sys_Error("DLL Plugin tried calling invalid biultin %i", arg);
	return 0;
}


plugin_t *Plug_Load(char *file)
{
	plugin_t *newplug;
	long argarray;

	for (newplug = plugs; newplug; newplug = newplug->next)
	{
		if (!stricmp(newplug->name, file))
			return NULL;
	}

	newplug = Z_Malloc(sizeof(plugin_t)+strlen(file)+1);
	newplug->name = (char*)(newplug+1);
	strcpy(newplug->name, file);
	
	newplug->vm = VM_Create(NULL, file, Plug_SystemCalls, Plug_SystemCallsEx);
	currentplug = newplug;
	if (newplug->vm)
	{
		newplug->next = plugs;
		plugs = newplug;

		argarray = (long)"Plug_GetEngineFunction";
		VM_Call(newplug->vm, 0, Plug_FindBuiltin(NULL, ~0, &argarray));

		if (newplug->reschange)
			VM_Call(newplug->vm, newplug->reschange, vid.width, vid.height);
	}
	else
	{
		Z_Free(newplug);
		newplug = NULL;
	}
	currentplug = NULL;

	return newplug;
}

int Plug_Emumerated (char *name, int size, void *param)
{
	char vmname[MAX_QPATH];
	strcpy(vmname, name);
	vmname[strlen(vmname) - strlen(param)] = '\0';
	Plug_Load(vmname);

	return true;
}

int Plug_Con_Print(void *offset, unsigned int mask, const long *arg)
{
	Con_Print((char*)VM_POINTER(arg[0]));
	return 0;
}
int Plug_Sys_Error(void *offset, unsigned int mask, const long *arg)
{
	Sys_Error("%s", (char*)offset+arg[0]);
	return 0;
}
int Plug_ExportToEngine(void *offset, unsigned int mask, const long *arg)
{
	char *name = (char*)VM_POINTER(arg[0]);
	if (!strcmp(name, "Tick"))
		currentplug->tick = arg[1];
	else if (!strcmp(name, "ExecuteCommand"))
		currentplug->executestring = arg[1];
	else if (!strcmp(name, "ConExecuteCommand"))
		currentplug->conexecutecommand = arg[1];
	else if (!strcmp(name, "MenuEvent"))
		currentplug->menufunction = arg[1];
	else if (!strcmp(name, "UpdateVideo"))
		currentplug->reschange = arg[1];
	else if (!strcmp(name, "SbarBase"))			//basic SBAR.
		currentplug->sbarlevel[0] = arg[1];
	else if (!strcmp(name, "SbarSupplement"))	//supplementry stuff - teamplay
		currentplug->sbarlevel[1] = arg[1];
	else if (!strcmp(name, "SbarOverlay"))		//overlay - scoreboard type stuff.
		currentplug->sbarlevel[2] = arg[1];
	else
		return 0;
	return 1;
}

typedef struct {
	//Make SURE that the engine has resolved all cvar pointers into globals before this happens.
	plugin_t *plugin;
	cvar_t *var;
} plugincvararray_t;
int plugincvararraylen;
plugincvararray_t *plugincvararray;
//qhandle_t Cvar_Register (char *name, char *defaultval, int flags, char *grouphint);
int Plug_Cvar_Register(void *offset, unsigned int mask, const long *arg)
{
	char *name = VM_POINTER(arg[0]);
	char *defaultvalue = VM_POINTER(arg[1]);
	unsigned int flags = VM_LONG(arg[2]);
	char *groupname = VM_POINTER(arg[3]);
	cvar_t *var;
	int i;

	var = Cvar_Get(name, defaultvalue, flags&1, groupname);

	for (i = 0; i < plugincvararraylen; i++)
	{
		if (!plugincvararray[i].var)
		{	//hmm... a gap...
			plugincvararray[i].plugin = currentplug;
			plugincvararray[i].var = var;
			return i;
		}
	}

	plugincvararray = BZ_Realloc(plugincvararray, (plugincvararraylen+1)*sizeof(plugincvararray_t));
	plugincvararray[plugincvararraylen].plugin = currentplug;
	plugincvararray[plugincvararraylen].var = var;
	plugincvararraylen++;
	return plugincvararraylen-1;
}
//int Cvar_Update, (qhandle_t handle, int modificationcount, char *stringv, float *floatv));	//stringv is 256 chars long, don't expect this function to do anything if modification count is unchanged.
int Plug_Cvar_Update(void *offset, unsigned int mask, const long *arg)
{
	int handle;
	int modcount;
	char *stringv;	//255 bytes long.
	float *floatv;
	cvar_t *var;
	handle = VM_LONG(arg[0]);
	if (handle < 0 || handle >= plugincvararraylen)
		return 0;
	if (plugincvararray[handle].plugin != currentplug)
		return 0;	//I'm not letting you know what annother plugin has registered.

	if (VM_OOB(arg[2], 256) || VM_OOB(arg[3], 4))	//Oi, plugin - you screwed up
		return 0;

	modcount = VM_LONG(arg[1]);
	stringv = VM_POINTER(arg[2]);
	floatv = VM_POINTER(arg[3]);

	var = plugincvararray[handle].var;


	strcpy(stringv, var->string);
	*floatv = var->value;

	return var->modified;
}

//void Cmd_Args(char *buffer, int buffersize)
int Plug_Cmd_Args(void *offset, unsigned int mask, const long *arg)
{
	char *buffer = (char*)VM_POINTER(arg[0]);
	char *args;
	args = Cmd_Args();
	if (strlen(args)+1>arg[1])
		return 0;
	strcpy(buffer, args);
	return 1;
}
//void Cmd_Argv(int num, char *buffer, int buffersize)
int Plug_Cmd_Argv(void *offset, unsigned int mask, const long *arg)
{
	char *buffer = (char*)VM_POINTER(arg[1]);
	char *args;
	args = Cmd_Argv(arg[0]);
	if (strlen(args)+1>arg[2])
		return 0;
	strcpy(buffer, args);
	return 1;
}
//int Cmd_Argc(void)
int Plug_Cmd_Argc(void *offset, unsigned int mask, const long *arg)
{
	return Cmd_Argc();
}

int Plug_Menu_Control(void *offset, unsigned int mask, const long *arg)
{
	switch(VM_LONG(arg[0]))
	{
	case 0:	//take away all menus
	case 1:
		if (menuplug)
		{
			plugin_t *oldplug = currentplug;
			currentplug = menuplug;
			Plug_Menu_Event(3, 0);
			menuplug = NULL;
			currentplug = oldplug;
			key_dest = key_game;
		}
		if (VM_LONG(arg[0]) != 1)
			return 1;
		//give us menu control
		menuplug = currentplug;
		key_dest = key_menu;
		m_state = m_plugin;
		return 1;
	case 2: //weather it's us or not.
		return currentplug == menuplug && m_state == m_plugin;
	case 3:	//weather a menu is active
		return key_dest == key_menu;
	default:
		return 0;
	}
}

typedef struct {
	//Make SURE that the engine has resolved all cvar pointers into globals before this happens.
	plugin_t *plugin;
	char name[64];
	qboolean picfromwad;
	mpic_t *pic;
} pluginimagearray_t;
int pluginimagearraylen;
pluginimagearray_t *pluginimagearray;

int Plug_Draw_LoadImage(void *offset, unsigned int mask, const long *arg)
{
	char *name = VM_POINTER(arg[0]);
	qboolean fromwad = arg[1];
	int i;

	mpic_t *pic;

	for (i = 0; i < pluginimagearraylen; i++)
	{
		if (!pluginimagearray[i].plugin)
			break;
		if (pluginimagearray[i].plugin == currentplug)
		{
			if (!strcmp(name, pluginimagearray[i].name))
				break;
		}
	}
	if (i == pluginimagearraylen)
	{
		pluginimagearraylen++;
		pluginimagearray = BZ_Realloc(pluginimagearray, pluginimagearraylen*sizeof(pluginimagearray_t));
	}

	if (pluginimagearray[i].pic)
		return i;	//already loaded.

	if (qrenderer)
	{
		if (fromwad)
			pic = Draw_SafePicFromWad(name);
		else
		{
#ifdef RGLQUAKE	//GL saves images persistantly (so don't bother with cachepic stuff)
			if (qrenderer == QR_OPENGL)
				pic = Draw_SafeCachePic(name);
			else
#endif
				pic = NULL;
		}
	}
	else
		pic = NULL;

	Q_strncpyz(pluginimagearray[i].name, name, sizeof(pluginimagearray[i].name));
	pluginimagearray[i].picfromwad = fromwad;
	pluginimagearray[i].pic = pic;
	pluginimagearray[i].plugin = currentplug;
	return i;
}

void Plug_DrawReloadImages(void)
{
	int i;
	for (i = 0; i < pluginimagearraylen; i++)
	{
		if (!pluginimagearray[i].plugin)
		{
			pluginimagearray[i].pic = NULL;
			continue;
		}

		if (pluginimagearray[i].picfromwad)
			pluginimagearray[i].pic = Draw_SafePicFromWad(pluginimagearray[i].name);
#ifdef RGLQUAKE
		else if (qrenderer == QR_OPENGL)
				pluginimagearray[i].pic = Draw_SafeCachePic(pluginimagearray[i].name);
#endif
		else
			pluginimagearray[i].pic = NULL;
	}
}

void Plug_FreePlugImages(plugin_t *plug)
{
	int i;
	for (i = 0; i < pluginimagearraylen; i++)
	{
		if (pluginimagearray[i].plugin == plug)
		{
			pluginimagearray[i].plugin = 0;
			pluginimagearray[i].pic = NULL;
			pluginimagearray[i].name[0] = '\0';
		}
	}
}

//int Draw_Image (float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t image)
int Plug_Draw_Image(void *offset, unsigned int mask, const long *arg)
{
	mpic_t *pic;
	int i;
	if (!qrenderer)
		return 0;
	if (!Draw_Image)
		return 0;

	i = VM_LONG(arg[8]);
	if (i < 0 || i >= pluginimagearraylen)
		return -1;	// you fool
	if (pluginimagearray[i].plugin != currentplug)
		return -1;

	if (pluginimagearray[i].pic)
		pic = pluginimagearray[i].pic;
	else if (pluginimagearray[i].picfromwad)
		return 0;	//wasn't loaded.
	else
		pic = Draw_CachePic(pluginimagearray[i].name);

	Draw_Image(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]), VM_FLOAT(arg[2]), VM_FLOAT(arg[3]), VM_FLOAT(arg[4]), VM_FLOAT(arg[5]), VM_FLOAT(arg[6]), VM_FLOAT(arg[7]), pic);
	return 1;
}

int Plug_Draw_Character(void *offset, unsigned int mask, const long *arg)
{
	Draw_Character(arg[0], arg[1], (unsigned int)arg[2]);
	return 0;
}

int Plug_Draw_Fill(void *offset, unsigned int mask, const long *arg)
{
	float x, y, width, height;
	x = VM_FLOAT(arg[0]);
	y = VM_FLOAT(arg[1]);
	width = VM_FLOAT(arg[2]);
	height = VM_FLOAT(arg[3]);
	switch(qrenderer)	//FIXME: I don't want qrenderer seen outside the refresh
	{
#ifdef RGLQUAKE
	case QR_OPENGL:
		qglDisable(GL_TEXTURE_2D);
		qglBegin(GL_QUADS);
		qglVertex2f(x, y);
		qglVertex2f(x+width, y);
		qglVertex2f(x+width, y+height);
		qglVertex2f(x, y+height);
		qglEnd();
		qglEnable(GL_TEXTURE_2D);
		return 1;
#endif
	default:
		break;
	}
	return 0;
}

int Plug_Draw_ColourP(void *offset, unsigned int mask, const long *arg)
{
	qbyte *pal = host_basepal + VM_LONG(arg[0])*3;

	if (arg[0]<0 || arg[0]>255)
		return false;

	if (Draw_ImageColours)
	{
		Draw_ImageColours(pal[0]/255.0f, pal[1]/255.0f, pal[2]/255.0f, 1);
		return 1;
	}
	return 0;
}

int Plug_Draw_Colour3f(void *offset, unsigned int mask, const long *arg)
{
	if (Draw_ImageColours)
	{
		Draw_ImageColours(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]), VM_FLOAT(arg[2]), 1);
		return 1;
	}
	return 0;
}
int Plug_Draw_Colour4f(void *offset, unsigned int mask, const long *arg)
{
	if (Draw_ImageColours)
	{
		Draw_ImageColours(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]), VM_FLOAT(arg[2]), VM_FLOAT(arg[3]));
		return 1;
	}
	return 0;
}

int Plug_Key_GetKeyCode(void *offset, unsigned int mask, const long *arg)
{
	int modifier;
	return Key_StringToKeynum(VM_POINTER(arg[0]), &modifier);
}

//void Cvar_SetString (char *name, char *value);
int Plug_Cvar_SetString(void *offset, unsigned int mask, const long *arg)
{
	char *name = VM_POINTER(arg[0]),
		*value = VM_POINTER(arg[1]);
	cvar_t *var = Cvar_Get(name, value, 0, "Plugin vars");
	if (var)
	{
		Cvar_Set(var, value);
		return 1;
	}

	return 0;
}

//void Cvar_SetFloat (char *name, float value);
int Plug_Cvar_SetFloat(void *offset, unsigned int mask, const long *arg)
{
	char *name = VM_POINTER(arg[0]);
	float value = VM_FLOAT(arg[1]);
	cvar_t *var = Cvar_Get(name, "", 0, "Plugin vars");	//"" because I'm lazy
	if (var)
	{
		Cvar_SetValue(var, value);
		return 1;
	}

	return 0;
}

//void Cvar_GetFloat (char *name);
int Plug_Cvar_GetFloat(void *offset, unsigned int mask, const long *arg)
{
	char *name = VM_POINTER(arg[0]);
	int ret;
	cvar_t *var = Cvar_Get(name, "", 0, "Plugin vars");
	if (var)
	{
		VM_FLOAT(ret) = var->value;
	}
	else
		VM_FLOAT(ret) = 0;
	return ret;
}

//qboolean Cvar_GetString (char *name, char *retstring, int sizeofretstring);
int Plug_Cvar_GetString(void *offset, unsigned int mask, const long *arg)
{
	char *name, *ret;
	int retsize;
	cvar_t *var;
	if (VM_OOB(arg[1], arg[2]))
	{
		return false;
	}

	name = VM_POINTER(arg[0]);
	ret = VM_POINTER(arg[1]);
	retsize = VM_LONG(arg[2]);


	var = Cvar_Get(name, "", 0, "Plugin vars");
	if (strlen(var->name)+1 > retsize)
		return false;

	strcpy(ret, var->string);

	return true;
}

//void Cmd_AddText (char *text, qboolean insert);	//abort the entire engine.
int Plug_Cmd_AddText(void *offset, unsigned int mask, const long *arg)
{
	if (VM_LONG(arg[1]))
		Cbuf_InsertText(VM_POINTER(arg[0]), RESTRICT_LOCAL);
	else
		Cbuf_AddText(VM_POINTER(arg[0]), RESTRICT_LOCAL);

	return 1;
}

int plugincommandarraylen;
typedef struct {
	plugin_t *plugin;
	char command[64];
} plugincommand_t;
plugincommand_t *plugincommandarray;
void Plug_Command_f(void)
{
	int i;
	char *cmd = Cmd_Argv(0);
	plugin_t *oldplug = currentplug;
	for (i = 0; i < plugincommandarraylen; i++)
	{
		if (!plugincommandarray[i].plugin)
			continue;	//don't check commands who's owners died.

		if (stricmp(plugincommandarray[i].command, cmd))	//not the right command
			continue;

		currentplug = plugincommandarray[i].plugin;

		if (currentplug->executestring)
			VM_Call(currentplug->vm, currentplug->executestring, 0);
		break;
	}

	currentplug = oldplug;
}

int Plug_Cmd_AddCommand(void *offset, unsigned int mask, const long *arg)
{
	int i;
	char *name = VM_POINTER(arg[0]);
	for (i = 0; i < plugincommandarraylen; i++)
	{
		if (!plugincommandarray[i].plugin)
			break;
		if (plugincommandarray[i].plugin == currentplug)
		{
			if (!strcmp(name, plugincommandarray[i].command))
				break;
		}
	}
	if (i == plugincommandarraylen)
	{
		plugincommandarraylen++;
		plugincommandarray = BZ_Realloc(plugincommandarray, plugincommandarraylen*sizeof(plugincommand_t));
	}

	Q_strncpyz(plugincommandarray[i].command, name, sizeof(plugincommandarray[i].command));
	if (!Cmd_AddRemCommand(plugincommandarray[i].command, Plug_Command_f))
		return false;
	plugincommandarray[i].plugin = currentplug;	//worked
	return true;
}
void Plug_FreeConCommands(plugin_t *plug)
{
	int i;
	for (i = 0; i < plugincommandarraylen; i++)
	{
		if (plugincommandarray[i].plugin == plug)
		{
			plugincommandarray[i].plugin = NULL;
			Cmd_RemoveCommand(plugincommandarray[i].command);
		}
	}
}

int Plug_CL_GetStats(void *offset, unsigned int mask, const long *arg)
{
	int i;
	int pnum = VM_LONG(arg[0]);
	unsigned int *stats = VM_POINTER(arg[1]);
	int pluginstats = VM_LONG(arg[2]);
	int max;

	if (VM_OOB(arg[1], arg[2]*4))
		return 0;

	max = pluginstats;
	if (max > MAX_CL_STATS)
		max = MAX_CL_STATS;
	for (i = 0; i < max; i++)
	{	//fill stats with the right player's stats
		stats[i] = cl.stats[pnum][i];
	}
	for (; i < pluginstats; i++)	//plugin has too many stats (wow)
		stats[i] = 0;					//fill the rest.
	return max;
}

int Plug_Con_SubPrint(void *offset, unsigned int mask, const long *arg)
{
	char *name = VM_POINTER(arg[0]);
	char *text = VM_POINTER(arg[1]);
	console_t *con;
	con = Con_FindConsole(name);
	if (!con)
	{
		con = Con_Create(name);
		Con_SetVisible(con);
	}

	Con_PrintCon(con, text);

	return 1;
}
int Plug_Con_RenameSub(void *offset, unsigned int mask, const long *arg)
{
	char *name = VM_POINTER(arg[0]);
	console_t *con;
	con = Con_FindConsole(name);
	if (!con)
		return 0;

	Q_strncpyz(con->name, name, sizeof(con->name));

	return 1;
}

void Plug_CloseAll_f(void);
void Plug_Close_f(void);
void Plug_Load_f(void)
{
	Plug_Load(Cmd_Argv(1));
}

void Plug_Init(void)
{
	Cvar_Register(&plug_sbar, "plugins");
	Cvar_Register(&plug_loaddefault, "plugins");
	Cmd_AddCommand("plug_closeall", Plug_CloseAll_f);
	Cmd_AddCommand("plug_close", Plug_Close_f);
	Cmd_AddCommand("plug_load", Plug_Load_f);

	Plug_RegisterBuiltin("Plug_GetEngineFunction",	Plug_FindBuiltin, 0);//plugin wishes to find a builtin number.
	Plug_RegisterBuiltin("Plug_ExportToEngine",		Plug_ExportToEngine, 0);	//plugin has a call back that we might be interested in.
	Plug_RegisterBuiltin("Con_Print",				Plug_Con_Print, 0);	//printf is not possible - qvm floats are never doubles, vararg floats in a cdecl call are always converted to doubles.
	Plug_RegisterBuiltin("Sys_Error",				Plug_Sys_Error, 0);
	Plug_RegisterBuiltin("Com_Error",				Plug_Sys_Error, 0);	//make zquake programmers happy.

	Plug_RegisterBuiltin("Cmd_AddCommand",			Plug_Cmd_AddCommand, 0);
	Plug_RegisterBuiltin("Cmd_Args",				Plug_Cmd_Args, 0);
	Plug_RegisterBuiltin("Cmd_Argc",				Plug_Cmd_Argc, 0);
	Plug_RegisterBuiltin("Cmd_Argv",				Plug_Cmd_Argv, 0);
	Plug_RegisterBuiltin("Cmd_AddText",				Plug_Cmd_AddText, 0);

	Plug_RegisterBuiltin("CL_GetStats",				Plug_CL_GetStats, 0);
	Plug_RegisterBuiltin("Menu_Control",			Plug_Menu_Control, 0);
	Plug_RegisterBuiltin("Key_GetKeyCode",			Plug_Key_GetKeyCode, 0);

	Plug_RegisterBuiltin("Cvar_Register",			Plug_Cvar_Register, 0);
	Plug_RegisterBuiltin("Cvar_SetString",			Plug_Cvar_SetString, 0);
	Plug_RegisterBuiltin("Cvar_SetFloat",			Plug_Cvar_SetFloat, 0);
	Plug_RegisterBuiltin("Cvar_GetString",			Plug_Cvar_GetString, 0);
	Plug_RegisterBuiltin("Cvar_GetFloat",			Plug_Cvar_GetFloat, 0);

	Plug_RegisterBuiltin("Draw_LoadImage",			Plug_Draw_LoadImage, 0);
	Plug_RegisterBuiltin("Draw_Image",				Plug_Draw_Image, 0);

	Plug_RegisterBuiltin("Draw_Character",			Plug_Draw_Character, 0);
	Plug_RegisterBuiltin("Draw_Fill",				Plug_Draw_Fill, 0);
	Plug_RegisterBuiltin("Draw_Colourp",			Plug_Draw_ColourP, 0);
	Plug_RegisterBuiltin("Draw_Colour3f",			Plug_Draw_Colour3f, 0);
	Plug_RegisterBuiltin("Draw_Colour4f",			Plug_Draw_Colour4f, 0);

	Plug_RegisterBuiltin("Con_SubPrint",			Plug_Con_SubPrint, 0);
	Plug_RegisterBuiltin("Con_RenameSub",			Plug_Con_RenameSub, 0);

#ifdef _WIN32
	COM_EnumerateFiles("plugins/*x86.dll",	Plug_Emumerated, "x86.dll");
#elif defined(__linux__)
	COM_EnumerateFiles("plugins/*x86.so",	Plug_Emumerated, "x86.so");
#endif
	COM_EnumerateFiles("plugins/*.qvm",		Plug_Emumerated, ".qvm");
}

void Plug_Tick(void)
{
	plugin_t *oldplug = currentplug;
	for (currentplug = plugs; currentplug; currentplug = currentplug->next)
	{
		if (currentplug->tick)
		{
			VM_Call(currentplug->vm, currentplug->tick, (int)(realtime*1000));
		}
	}
	currentplug = oldplug;
}

void Plug_ResChanged(void)
{
	plugin_t *oldplug = currentplug;
	for (currentplug = plugs; currentplug; currentplug = currentplug->next)
	{
		if (currentplug->reschange)
			VM_Call(currentplug->vm, currentplug->reschange, vid.width, vid.height);
	}
	currentplug = oldplug;
}

qboolean Plugin_ExecuteString(void)
{
	plugin_t *oldplug = currentplug;
	if (Cmd_Argc()>0)
	{
		for (currentplug = plugs; currentplug; currentplug = currentplug->next)
		{
			if (currentplug->executestring)
			{
				if (VM_Call(currentplug->vm, currentplug->executestring, 0))
				{
					currentplug = oldplug;
					return true;
				}
			}
		}
	}
	currentplug = oldplug;
	return false;
}

qboolean Plug_Menu_Event(int eventtype, int param)	//eventtype = draw/keydown/keyup, param = time/key
{
	plugin_t *oc=currentplug;
	qboolean ret;
	extern int mousecursor_x, mousecursor_y;

	if (!menuplug)
		return false;

	currentplug = menuplug;
	ret = VM_Call(menuplug->vm, menuplug->menufunction, eventtype, param, mousecursor_x, mousecursor_y);
	currentplug=oc;
	return ret;
}

void Plug_SBar(void)
{
	plugin_t *oc=currentplug;
	int cp;
	vrect_t rect;

	if (!plug_sbar.value)
		currentplug = NULL;
	else
	{
		for (currentplug = plugs; currentplug; currentplug = currentplug->next)
		{
			if (currentplug->sbarlevel[0])
			{
				for (cp = 0; cp < cl.splitclients; cp++)
				{	//if you don't use splitscreen, use a full videosize rect.
					SCR_VRectForPlayer(&rect, cp);
					VM_Call(currentplug->vm, currentplug->sbarlevel[0], cp, rect.x, rect.y, rect.width, rect.height);
				}
				break;
			}
		}
	}
	if (!currentplug)
	{
		Sbar_Draw();
		currentplug = oc;
		return;	//our current sbar draws a scoreboard too. We don't want that bug to be quite so apparent.
				//please don't implement this identical hack in your engines...
	}

	for (currentplug = plugs; currentplug; currentplug = currentplug->next)
	{
		if (currentplug->sbarlevel[1])
		{
			for (cp = 0; cp < cl.splitclients; cp++)
			{	//if you don't use splitscreen, use a full videosize rect.
				SCR_VRectForPlayer(&rect, cp);
				VM_Call(currentplug->vm, currentplug->sbarlevel[1], cp, rect.x, rect.y, rect.width, rect.height);
			}
		}
	}

	for (currentplug = plugs; currentplug; currentplug = currentplug->next)
	{
		if (currentplug->sbarlevel[2])
		{
			for (cp = 0; cp < cl.splitclients; cp++)
			{	//if you don't use splitscreen, use a full videosize rect.
				SCR_VRectForPlayer(&rect, cp);
				VM_Call(currentplug->vm, currentplug->sbarlevel[2], cp, rect.x, rect.y, rect.width, rect.height);
			}
		}
	}


	currentplug = oc;
}

void Plug_Close(plugin_t *plug)
{
	if (plugs == plug)
		plugs = plug->next;
	else
	{
		plugin_t *prev;
		for (prev = plugs; prev; prev = prev->next)
		{
			if (prev->next == plug)
				break;
		}
		if (!prev)
			Sys_Error("Plug_Close: not linked\n");
		prev->next = plug->next;
	}

	Con_Printf("Closing plugin %s\n", plug->name);
	VM_Destroy(plug->vm);

	Plug_FreePlugImages(plug);
	Plug_FreeConCommands(plug);

	if (currentplug == plug)
		currentplug = NULL;
	if (menuplug == plug)
	{
		menuplug = NULL;
		key_dest = key_game;
	}
}

void Plug_Close_f(void)
{
	plugin_t *plug;
	char *name = Cmd_Argv(1);
	if (Cmd_Argc()<2)
	{
		Con_Printf("Close which plugin?\n");
		return;
	}
	for (plug = plugs; plug; plug = plug->next)
	{
		if (!strcmp(plug->name, name))
		{
			Plug_Close(plug);
			return;
		}
	}
}

void Plug_CloseAll_f(void)
{
	if (currentplug)
		Sys_Error("Plug_CloseAll_f called inside a plugin!\n");
	while(plugs)
	{
		Plug_Close(plugs);
	}
}

void Plug_Shutdown(void)
{
	while(plugs)
	{
		Plug_Close(plugs);
	}
}

#endif
