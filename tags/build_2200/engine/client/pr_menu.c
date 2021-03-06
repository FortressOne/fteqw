#include "quakedef.h"

#ifdef MENU_DAT

#ifdef RGLQUAKE
#include "glquake.h"
#endif

int MP_TranslateFTEtoDPCodes(int code);

typedef struct menuedict_s
{
	qboolean	isfree;
	float		freetime; // sv.time when the object was freed
	int			entnum;
	qboolean	readonly;	//world
} menuedict_t;

#define	RETURN_SSTRING(s) (*(char **)&((int *)pr_globals)[OFS_RETURN] = PR_SetString(prinst, s))	//static - exe will not change it.
char *PF_TempStr(void);

int menuentsize;

//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(progfuncs_t *progfuncs, char *format, ...);
void PF_cvar_string (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_cvar_set (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_dprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_error (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_rint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_floor (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ceil (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Tokenize  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ArgV  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_FindString (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_FindFloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_nextent (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_randomvec (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Sin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Cos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Sqrt (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_bound (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_strlen(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_strcat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ftos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fabs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_vtos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_etos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_stof (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_mod (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_substring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_stov (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_dupstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_forgetstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Spawn (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_min (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_max (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_pow (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_localcmd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_random (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fopen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fclose (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fputs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fgets (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_normalize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_vlen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_vectoyaw (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_vectoangles (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchain (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchainfloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_coredump (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_traceon (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_traceoff (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_eprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_fclose_progs (progfuncs_t *prinst);
char *PF_VarString (progfuncs_t *prinst, int	first, struct globalvars_s *pr_globals);

//new generic functions.

//float	isfunction(string function_name) = #607;
void PF_isfunction (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*name = PR_GetStringOfs(prinst, OFS_PARM0);
	G_FLOAT(OFS_RETURN) = !!PR_FindFunction(prinst, name, PR_CURRENT);
}

//void	callfunction(...) = #605;
void PF_callfunction (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*name;
	func_t f;
	if (*prinst->callargc < 1)
		PR_BIError(prinst, "callfunction needs at least one argument\n");
	name = PR_GetStringOfs(prinst, OFS_PARM0+(*prinst->callargc-1)*3);
	f = PR_FindFunction(prinst, name, PR_CURRENT);
	if (f)
		PR_ExecuteProgram(prinst, f);
}

//void	loadfromfile(string file) = #69;
void PF_loadfromfile (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*filename = PR_GetStringOfs(prinst, OFS_PARM0);
	char *file = COM_LoadTempFile(filename);

	int size;

	if (!file)
	{
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}

	while(prinst->restoreent(prinst, file, &size, NULL))
	{
		file += size;
	}

	G_FLOAT(OFS_RETURN) = 0;
}

void PF_loadfromdata (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*file = PR_GetStringOfs(prinst, OFS_PARM0);

	int size;

	if (!*file)
	{
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}

	while(prinst->restoreent(prinst, file, &size, NULL))
	{
		file += size;
	}

	G_FLOAT(OFS_RETURN) = 0;
}

void PF_parseentitydata(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	void	*ed = G_EDICT(prinst, OFS_PARM0);
	char	*file = PR_GetStringOfs(prinst, OFS_PARM1);

	int size;

	if (!*file)
	{
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}

	if (!prinst->restoreent(prinst, file, &size, ed))
		Con_Printf("parseentitydata: missing opening data\n");
	else
	{
		file += size;
		while(*file < ' ' && *file)
			file++;
		if (*file)
			Con_Printf("parseentitydata: too much data\n");
	}
	
	G_FLOAT(OFS_RETURN) = 0;
}

void PF_mod (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = (float)(((int)G_FLOAT(OFS_PARM0))%((int)G_FLOAT(OFS_PARM1)));
}

typedef struct prvmsearch_s {
	int handle;
	progfuncs_t *fromprogs;	//share across menu/server
	int entries;
	char **names;
	int *sizes;

	struct prvmsearch_s *next;
} prvmsearch_t;
prvmsearch_t *prvmsearches;
int prvm_nextsearchhandle;

void search_close (progfuncs_t *prinst, int handle)
{
	int i;
	prvmsearch_t *prev, *s;

	prev = NULL;
	for (s = prvmsearches; s; )
	{
		if (s->handle == handle)
		{	//close it down.
			if (s->fromprogs != prinst)
			{
				Con_Printf("Handle wasn't valid with that progs\n");
				return;
			}
			if (prev)
				prev->next = s->next;
			else
				prvmsearches = s->next;

			for (i = 0; i < s->entries; i++)
			{
				BZ_Free(s->names[i]);
			}
			BZ_Free(s->names);
			BZ_Free(s->sizes);
			BZ_Free(s);

			return;
		}

		prev = s;
		s = s->next;
	}
}
//a progs was closed... hunt down it's searches, and warn about any searches left open.
void search_close_progs(progfuncs_t *prinst, qboolean complain)
{
	int i;
	prvmsearch_t *prev, *s;

	prev = NULL;
	for (s = prvmsearches; s; )
	{
		if (s->fromprogs == prinst)
		{	//close it down.

			if (complain)
				Con_Printf("Warning: Progs search was still active\n");
			if (prev)
				prev->next = s->next;
			else
				prvmsearches = s->next;

			for (i = 0; i < s->entries; i++)
			{
				BZ_Free(s->names[i]);
			}
			BZ_Free(s->names);
			BZ_Free(s->sizes);
			BZ_Free(s);

			if (prev)
				s = prev->next;
			else
				s = prvmsearches;
			continue;
		}

		prev = s;
		s = s->next;
	}

	if (!prvmsearches)
		prvm_nextsearchhandle = 0;	//might as well.
}

int search_enumerate(char *name, int fsize, void *parm)
{
	prvmsearch_t *s = parm;

	s->names = BZ_Realloc(s->names, ((s->entries+64)&~63) * sizeof(char*));
	s->sizes = BZ_Realloc(s->sizes, ((s->entries+64)&~63) * sizeof(int));
	s->names[s->entries] = BZ_Malloc(strlen(name)+1);
	strcpy(s->names[s->entries], name);
	s->sizes[s->entries] = fsize;

	s->entries++;
	return true;
}

//float	search_begin(string pattern, float caseinsensitive, float quiet) = #74;
void PF_search_begin (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{	//< 0 for error, > 0 for handle.
	char *pattern = PR_GetStringOfs(prinst, OFS_PARM0);
//	qboolean caseinsensative = G_FLOAT(OFS_PARM1);
//	qboolean quiet = G_FLOAT(OFS_PARM2);
	prvmsearch_t *s;

	s = BZ_Malloc(sizeof(*s));
	s->fromprogs = prinst;
	s->handle = prvm_nextsearchhandle++;

	COM_EnumerateFiles(pattern, search_enumerate, s);

	if (s->entries==0)
	{
		BZ_Free(s);
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}
	s->next = prvmsearches;
	prvmsearches = s;
	G_FLOAT(OFS_RETURN) = s->handle;
}
//void	search_end(float handle) = #75;
void PF_search_end (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	search_close(prinst, handle);
}
//float	search_getsize(float handle) = #76;
void PF_search_getsize (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	prvmsearch_t *s;
	G_FLOAT(OFS_RETURN) = -1;
	for (s = prvmsearches; s; s = s->next)
	{
		if (s->handle == handle)
		{	//close it down.
			if (s->fromprogs != prinst)
			{
				Con_Printf("Handle wasn't valid with that progs\n");
				return;
			}

			G_FLOAT(OFS_RETURN) = s->entries;
			return;
		}
	}
}
//string	search_getfilename(float handle, float num) = #77;
void PF_search_getfilename (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int handle = G_FLOAT(OFS_PARM0);
	int num = G_FLOAT(OFS_PARM1);
	prvmsearch_t *s;
	G_INT(OFS_RETURN) = 0;

	for (s = prvmsearches; s; s = s->next)
	{
		if (s->handle == handle)
		{	//close it down.
			if (s->fromprogs != prinst)
			{
				Con_Printf("Search handle wasn't valid with that progs\n");
				return;
			}

			if (num < 0 || num >= s->entries)
				return;
			G_INT(OFS_RETURN) = (int)(s->names[num] - prinst->stringtable);
			return;
		}
	}

	Con_Printf("Search handle wasn't valid\n");
}






void PF_developerprint (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *s;
	if (!developer.value)
		return;
	s = PF_VarString(prinst, 0, pr_globals);
	Con_Printf("%s", s);
}
void PF_print (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *s;
	s = PF_VarString(prinst, 0, pr_globals);
	Con_Printf("%s", s);
}




static void PF_cvar (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	cvar_t	*var;
	char	*str;
	
	str = PR_GetStringOfs(prinst, OFS_PARM0);
	if (!strcmp("vid_conwidth", str))
		G_FLOAT(OFS_RETURN) = vid.conwidth;
	else if (!strcmp("vid_conheight", str))
		G_FLOAT(OFS_RETURN) = vid.conheight;
	else
	{
		var = Cvar_Get(str, "", 0, "menu cvars");
		if (var)
		{
			if (var->latched_string)
				G_FLOAT(OFS_RETURN) = atof(var->latched_string);			else
				G_FLOAT(OFS_RETURN) = var->value;
		}
		else
			G_FLOAT(OFS_RETURN) = 0;
	}
}
void PF_getresolution (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float mode = G_FLOAT(OFS_PARM0);
	float *ret = G_VECTOR(OFS_RETURN);
#pragma message("fixme: PF_getresolution should return other modes")
	if (mode > 0)
	{
		ret[0] = 0;
		ret[1] = 0;
		ret[2] = 0;
	}
	else
	{
		ret[0] = vid.width;
		ret[1] = vid.height;
		ret[2] = 0;
	}
}




void PF_nonfatalobjerror (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*s;
	struct edict_s	*ed;
	eval_t *selfp;
	
	s = PF_VarString(prinst, 0, pr_globals);

	prinst->PR_StackTrace(prinst);

	selfp = prinst->FindGlobal(prinst, "self", PR_CURRENT);
	if (selfp && selfp->_int)
	{
		ed = PROG_TO_EDICT(prinst, selfp->_int);

		prinst->PR_PrintEdict(prinst, ed);


		if (developer.value)
			*prinst->pr_trace = 2;
		else
		{
			ED_Free (prinst, ed);
		}
	}

	Con_Printf ("======OBJECT ERROR======\n%s\n", s);
}






//float	isserver(void)  = #60;
void PF_isserver (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = sv.state != ss_dead;
}

//float	clientstate(void)  = #62;
void PF_clientstate (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = cls.state >= ca_connected ? 2 : 1;	//fit in with netquake	 (we never run a menu.dat dedicated)
}

//too specific to the prinst's builtins.
static void PF_Fixme (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf("\n");

	prinst->PR_RunError(prinst, "\nBuiltin %i not implemented.\nMenu is not compatable.", prinst->lastcalledbuiltinnumber);
	PR_BIError (prinst, "bulitin not implemented");
}



void PF_CL_precache_sound (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*str;
	
	str = PR_GetStringOfs(prinst, OFS_PARM0);

	if (S_PrecacheSound(str))
		G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	else
		G_INT(OFS_RETURN) = 0;
}


void PF_CL_is_cached_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*str;
	
	str = PR_GetStringOfs(prinst, OFS_PARM0);

//	if (Draw_IsCached)
//		G_FLOAT(OFS_RETURN) = !!Draw_IsCached(str);
//	else
		G_FLOAT(OFS_RETURN) = 1;
}

void PF_CL_precache_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*str;
	mpic_t	*pic;
	
	str = PR_GetStringOfs(prinst, OFS_PARM0);

	pic = Draw_SafeCachePic(str);

	if (pic)
		G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	else
		G_INT(OFS_RETURN) = 0;
}

void PF_CL_free_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*str;
	
	str = PR_GetStringOfs(prinst, OFS_PARM0);

	//we don't support this.
}


//float	drawcharacter(vector position, float character, vector scale, vector rgb, float alpha, float flag) = #454;
void PF_CL_drawcharacter (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	int chara = G_FLOAT(OFS_PARM1);
	float *size = G_VECTOR(OFS_PARM2);
	float *rgb = G_VECTOR(OFS_PARM3);
	float alpha = G_FLOAT(OFS_PARM4);
//	float flag = G_FLOAT(OFS_PARM5);

	const float fsize = 0.0625;
	float frow, fcol;

	if (!chara)
	{
		G_FLOAT(OFS_RETURN) = -1;	//was null..
		return;
	}

	chara &= 255;
	frow = (chara>>4)*fsize;
	fcol = (chara&15)*fsize;

	if (Draw_ImageColours)
		Draw_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	if (Draw_Image)
		Draw_Image(pos[0], pos[1], size[0], size[1], fcol, frow, fcol+fsize, frow+fsize, Draw_CachePic("conchars"));

	G_FLOAT(OFS_RETURN) = 1;
}
//float	drawstring(vector position, string text, vector scale, vector rgb, float alpha, float flag) = #455;
void PF_CL_drawstring (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	char *text = PR_GetStringOfs(prinst, OFS_PARM1);
	float *size = G_VECTOR(OFS_PARM2);
//	float *rgb = G_VECTOR(OFS_PARM3);
//	float alpha = G_FLOAT(OFS_PARM4);
//	float flag = G_FLOAT(OFS_PARM5);

	if (!text)
	{
		G_FLOAT(OFS_RETURN) = -1;	//was null..
		return;
	}

	while(*text)
	{
		G_FLOAT(OFS_PARM1) = *text++;
		PF_CL_drawcharacter(prinst, pr_globals);
		pos[0] += size[0];
	}
}
//float	drawpic(vector position, string pic, vector size, vector rgb, float alpha, float flag) = #456;
void PF_CL_drawpic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	char *picname = PR_GetStringOfs(prinst, OFS_PARM1);
	mpic_t *p = Draw_SafeCachePic(picname);
	float *size = G_VECTOR(OFS_PARM2);
	float *rgb = G_VECTOR(OFS_PARM3);
	float alpha = G_FLOAT(OFS_PARM4);
//	float flag = G_FLOAT(OFS_PARM5);

	if (Draw_ImageColours)
		Draw_ImageColours(rgb[0], rgb[1], rgb[2], alpha);
	if (Draw_Image)
		Draw_Image(pos[0], pos[1], size[0], size[1], 0, 0, 1, 1, p);

	G_FLOAT(OFS_RETURN) = 1;
}
//float	drawfill(vector position, vector size, vector rgb, float alpha, float flag) = #457;
void PF_CL_drawfill (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = G_VECTOR(OFS_PARM0);
	float *size = G_VECTOR(OFS_PARM1);
	float *rgb = G_VECTOR(OFS_PARM2);
	float alpha = G_FLOAT(OFS_PARM3);
#ifdef RGLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		glColor4f(rgb[0], rgb[1], rgb[2], alpha);

		glDisable(GL_TEXTURE_2D);

		glBegin(GL_QUADS);
		glVertex2f(pos[0],			pos[1]);
		glVertex2f(pos[0]+size[0],	pos[1]);
		glVertex2f(pos[0]+size[0],	pos[1]+size[1]);
		glVertex2f(pos[0],			pos[1]+size[1]);
		glEnd();

		glEnable(GL_TEXTURE_2D);
	}
#endif
	G_FLOAT(OFS_RETURN) = 1;
}
//void	drawsetcliparea(float x, float y, float width, float height) = #458;
void PF_CL_drawsetcliparea (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
		G_FLOAT(OFS_RETURN) = 1;
}
//void	drawresetcliparea(void) = #459;
void PF_CL_drawresetcliparea (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
		G_FLOAT(OFS_RETURN) = 1;
}
//vector  drawgetimagesize(string pic) = #460;
void PF_CL_drawgetimagesize (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *picname = PR_GetStringOfs(prinst, OFS_PARM0);
	mpic_t *p = Draw_SafeCachePic(picname);

	float *ret = G_VECTOR(OFS_RETURN);

	if (p)
	{
		ret[0] = p->width;
		ret[1] = p->height;
		ret[2] = 0;
	}
	else
	{
		ret[0] = 0;
		ret[1] = 0;
		ret[2] = 0;
	}
}

//void	setkeydest(float dest) 	= #601;
void PF_setkeydest (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	switch((int)G_FLOAT(OFS_PARM0))
	{
	case 0:
		// key_game
		key_dest = key_game;
		break;
	case 2:
		// key_menu
		key_dest = key_menu;
		m_state = m_menu_dat;
		break;
	case 1:
		// key_message
		// key_dest = key_message
		// break;
	default:
		PR_BIError (prinst, "PF_setkeydest: wrong destination %i !\n",(int)G_FLOAT(OFS_PARM0));
	}
}
//float	getkeydest(void)	= #602;
void PF_getkeydest (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	switch(key_dest)
	{
	case key_game:
		G_FLOAT(OFS_RETURN) = 0;
		break;
	case key_menu:
		if (m_state == m_menu_dat)
			G_FLOAT(OFS_RETURN) = 2;
		else
			G_FLOAT(OFS_RETURN) = 3;
		break;
	case 1:
		// key_message
		// key_dest = key_message
		// break;
	default:
		G_FLOAT(OFS_RETURN) = 3;
	}
}

//void	setmousetarget(float trg) = #603;
void PF_setmousetarget (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	extern int mouseusedforgui;
	switch ((int)G_FLOAT(OFS_PARM0))
	{
	case 1:
		mouseusedforgui = true;
		break;
	case 2:
		mouseusedforgui = false;
		break;
	default:
		PR_BIError(prinst, "PF_setmousetarget: not a valid destination\n");
	}
}

//float	getmousetarget(void)	  = #604;
void PF_getmousetarget (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
}

int MP_TranslateDPtoFTECodes(int code);
//string	keynumtostring(float keynum) = #609;
void PF_CL_keynumtostring (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int code = G_FLOAT(OFS_PARM0);
	char *keyname = PF_TempStr();

	code = MP_TranslateDPtoFTECodes (code);

	strcpy(keyname, Key_KeynumToString(code));
	RETURN_SSTRING(keyname);
}

int MP_TranslateDPtoFTECodes(int code);
//string	findkeysforcommand(string command) = #610;
void PF_CL_findkeysforcommand (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *cmdname = PR_GetStringOfs(prinst, OFS_PARM0);
	int keynums[2];
	char *keyname = PF_TempStr();

	M_FindKeysForCommand(cmdname, keynums);

	keyname[0] = '\0';

	strcat (keyname, va(" \'%i\'", MP_TranslateFTEtoDPCodes(keynums[0])));
	strcat (keyname, va(" \'%i\'", MP_TranslateFTEtoDPCodes(keynums[1])));

	RETURN_SSTRING(keyname);
}

//vector	getmousepos(void)  	= #66;
void PF_getmousepos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *ret = G_VECTOR(OFS_RETURN);
	extern int mousemove_x, mousemove_y;

	ret[0] = mousemove_x;
	ret[1] = mousemove_y;

	mousemove_x=0;
	mousemove_y=0;

/*	ret[0] = mousecursor_x;
	ret[1] = mousecursor_y;*/
	ret[2] = 0;
}


void PF_Remove_ (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	void *ed;
	
	ed = G_EDICT(prinst, OFS_PARM0);
/*
	if (ed->isfree)
	{
		Con_DPrintf("Tried removing free entity\n");
		return;
	}
*/
	ED_Free (prinst, ed);
}

static void PF_CopyEntity (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	void *in, *out;

	in = G_EDICT(prinst, OFS_PARM0);
	out = G_EDICT(prinst, OFS_PARM1);

	memcpy((char*)out+prinst->parms->edictsize, (char*)in+prinst->parms->edictsize, menuentsize-prinst->parms->edictsize);
}

void PF_gethostcachevalue (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	#pragma message ("PF_gethostcachevalue: stub")
	G_FLOAT(OFS_RETURN) = 0;
}

void PF_gethostcachestring (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	#pragma message ("PF_gethostcachestring: stub")
	G_INT(OFS_RETURN) = 0;
}

void PF_localsound (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *soundname = PR_GetStringOfs(prinst, OFS_PARM0);
	S_LocalSound (soundname);
}

#define skip1 PF_Fixme,
#define skip5 skip1 skip1 skip1 skip1 skip1
#define skip10 skip5 skip5
#define skip50 skip10 skip10 skip10 skip10 skip10
#define skip100 skip50 skip50

void PF_menu_checkextension (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//yeah, this is a stub... not sure what form extex
	G_FLOAT(OFS_RETURN) = 0;
}

void PF_gettime (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = *prinst->parms->gametime;
}

void PF_CL_precache_file (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

//entity	findchainstring(.string _field, string match) = #26;
void PF_menu_findchain (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	char *s, *t;
	edict_t *ent, *chain;	//note, all edicts share the common header, but don't use it's fields!
	eval_t *val;

	chain = (edict_t *) *prinst->parms->sv_edicts;

	f = G_INT(OFS_PARM0)+prinst->fieldadjust;
	f += prinst->parms->edictsize/4;
	s = PR_GetStringOfs(prinst, OFS_PARM1);

	for (i = 1; i < *prinst->parms->sv_num_edicts; i++)
	{
		ent = EDICT_NUM(prinst, i);
		if (ent->isfree)
			continue;
		t = *(string_t *)&((float*)ent)[f] + prinst->stringtable;
		if (!t)
			continue;
		if (strcmp(t, s))
			continue;

		val = prinst->GetEdictFieldValue(prinst, ent, "chain", NULL);
		if (val)
			val->edict = EDICT_TO_PROG(prinst, chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, chain);
}
//entity	findchainfloat(.float _field, float match) = #27;
void PF_menu_findchainfloat (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	float s;
	edict_t	*ent, *chain;	//note, all edicts share the common header, but don't use it's fields!
	eval_t *val;

	chain = (edict_t *) *prinst->parms->sv_edicts;

	f = G_INT(OFS_PARM0)+prinst->fieldadjust;
	f += prinst->parms->edictsize/4;
	s = G_FLOAT(OFS_PARM1);

	for (i = 1; i < *prinst->parms->sv_num_edicts; i++)
	{
		ent = EDICT_NUM(prinst, i);
		if (ent->isfree)
			continue;
		if (((float *)ent)[f] != s)
			continue;

		val = prinst->GetEdictFieldValue(prinst, ent, "chain", NULL);
		if (val)
			val->edict = EDICT_TO_PROG(prinst, chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, chain);
}

void PF_etof(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = G_EDICTNUM(prinst, OFS_PARM0);
}
void PF_ftoe(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int entnum = G_FLOAT(OFS_PARM0);

	RETURN_EDICT(prinst, EDICT_NUM(prinst, entnum));
}

void PF_IsNull(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int str = G_INT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = !str;
}

builtin_t menu_builtins[] = {
//0
	PF_Fixme,
	PF_menu_checkextension,				//void 	checkextension(string ext) = #1;
	PF_error,
	PF_nonfatalobjerror,
	PF_print,
	skip1				//void 	bprint(string text,...)	= #5;
	skip1				//void	sprint(float clientnum, string text,...) = #6;
	skip1				//void 	centerprint(string text,...) = #7;
	PF_normalize,		//vector	normalize(vector v) 	= #8;
	PF_vlen,			//float 	vlen(vector v)			= #9;
//10
	PF_vectoyaw,//10		//float  	vectoyaw(vector v)		= #10;
	PF_vectoangles,//11		//vector 	vectoangles(vector v)	= #11;
	PF_random,//12
	PF_localcmd,//13
	PF_cvar,//14
	PF_cvar_set,//15
	PF_developerprint,//16
	PF_ftos,//17
	PF_fabs,//18
	PF_vtos,//19

	PF_etos,//20
	PF_stof,//21
	PF_Spawn,//22
	PF_Remove_,//23
	PF_FindString,//24
	PF_FindFloat,//25
	PF_menu_findchain,//26			//entity	findchainstring(.string _field, string match) = #26;
	PF_menu_findchainfloat,//27		//entity	findchainfloat(.float _field, float match) = #27;
	PF_CL_precache_file,//28
	PF_CL_precache_sound,//29

//30
	PF_coredump,				//void	coredump(void) = #30;
	PF_traceon,				//void	traceon(void) = #31;
	PF_traceoff,				//void	traceoff(void) = #32;
	PF_eprint,				//void	eprint(entity e)  = #33;
	PF_rint,
	PF_floor,
	PF_ceil,
	PF_nextent,
	PF_Sin,
	PF_Cos,
//40
	PF_Sqrt,
	PF_randomvec,
	PF_registercvar,
	PF_min,
	PF_max,
	PF_bound,
	PF_pow,
	PF_CopyEntity,
	PF_fopen,
	PF_fclose,
//50
	PF_fgets,
	PF_fputs,
	PF_strlen,
	PF_strcat,
	PF_substring,
	PF_stov,
	PF_dupstring,
	PF_forgetstring,
	PF_Tokenize,
	PF_ArgV,
//60
	PF_isserver,
	skip1						//float	clientcount(void)  = #61;
	PF_clientstate,
	skip1						//void	clientcommand(float client, string s)  = #63;
	skip1						//void	changelevel(string map)  = #64;
	PF_localsound,
	PF_getmousepos,
	PF_gettime,
	PF_loadfromdata,
	PF_loadfromfile,
//70
	PF_mod,//0
	PF_cvar_string,//1
	PF_Fixme,//2				//void	crash(void)	= #72;
	PF_Fixme,//3				//void	stackdump(void) = #73;
	PF_search_begin,//4
	PF_search_end,//5
	PF_search_getsize ,//6
	PF_search_getfilename,//7
	PF_chr2str,//8
	PF_etof,//9				//float 	etof(entity ent) = #79;
//80
	PF_ftoe,//10
	PF_IsNull,
	skip1
	skip1
	skip1
	skip1
	skip1
	skip1
	skip1
	skip1
//90
	skip10
//100
	skip100
//200
	skip100
//300
	skip100
//400
	skip50
//450
	PF_Fixme,//0
	PF_CL_is_cached_pic,//1
	PF_CL_precache_pic,//2
	PF_CL_free_pic,//3
	PF_CL_drawcharacter,//4
	PF_CL_drawstring,//5
	PF_CL_drawpic,//6
	PF_CL_drawfill,//7
	PF_CL_drawsetcliparea,//8
	PF_CL_drawresetcliparea,//9

//460
	PF_CL_drawgetimagesize,//10
	skip1
	skip1
	skip1
	skip1
	skip1
	skip1
	skip1
	skip1
	skip1
//470
	skip10
//480
	skip10
//490
	skip10
//500
	skip100
//600
	skip1
	PF_setkeydest,
	PF_getkeydest,
	PF_setmousetarget,
	PF_getmousetarget,
	PF_callfunction,
	skip1				//void	writetofile(float fhandle, entity ent) = #606;
	PF_isfunction,
	PF_getresolution,
	PF_CL_keynumtostring,
	PF_CL_findkeysforcommand,
	PF_gethostcachevalue,
	PF_gethostcachestring,
	PF_parseentitydata			//void 	parseentitydata(entity ent, string data) = #613;
};
int menu_numbuiltins = sizeof(menu_builtins)/sizeof(menu_builtins[0]);




void M_Init_Internal (void);
void M_DeInit_Internal (void);

int inmenuprogs;
progfuncs_t *menuprogs;
progparms_t menuprogparms;
menuedict_t *menu_edicts;
int num_menu_edicts;

func_t mp_init_function;
func_t mp_shutdown_function;
func_t mp_draw_function;
func_t mp_keydown_function;
func_t mp_keyup_function;
func_t mp_toggle_function;

float *mp_time;

jmp_buf mp_abort;


void MP_Shutdown (void)
{
	extern int mouseusedforgui;
	func_t temp;
	if (!menuprogs)
		return;

	temp = mp_shutdown_function;
	mp_shutdown_function = 0;
	if (temp && !inmenuprogs)
		PR_ExecuteProgram(menuprogs, temp);

	PF_fclose_progs(menuprogs);
	search_close_progs(menuprogs, true);

	CloseProgs(menuprogs);
	menuprogs = NULL;

	key_dest = key_game;
	m_state = 0;

	mouseusedforgui = false;

	M_Init_Internal();

	if (inmenuprogs)	//something in the menu caused the problem, so...
	{
		inmenuprogs = 0;
		longjmp(mp_abort, 1);
	}
}

int COM_FileSize(char *path);
pbool QC_WriteFile(char *name, void *data, int len);
void *VARGS PR_Malloc(int size);	//these functions should be tracked by the library reliably, so there should be no need to track them ourselves.
void VARGS PR_Free(void *mem);

//Any menu builtin error or anything like that will come here.
void VARGS Menu_Abort (char *format, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, format);
	_vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	Con_Printf("Menu_Abort: %s\nShutting down menu.dat\n", string);


{
	static char buffer[1024*1024*8];
	int size = sizeof buffer;
	menuprogs->save_ents(menuprogs, buffer, &size, 3);
	COM_WriteFile("menucore.txt", buffer, size);
}

	MP_Shutdown();
}

double  menutime;
void MP_Init (void)
{
	if (!qrenderer)
	{
		return;
	}

	M_DeInit_Internal();


	menuprogparms.progsversion = PROGSTRUCT_VERSION;
	menuprogparms.ReadFile = COM_LoadStackFile;//char *(*ReadFile) (char *fname, void *buffer, int *len);
	menuprogparms.FileSize = COM_FileSize;//int (*FileSize) (char *fname);	//-1 if file does not exist
	menuprogparms.WriteFile = QC_WriteFile;//bool (*WriteFile) (char *name, void *data, int len);
	menuprogparms.printf = (void *)Con_Printf;//Con_Printf;//void (*printf) (char *, ...);
	menuprogparms.Sys_Error = Sys_Error;
	menuprogparms.Abort = Menu_Abort;
	menuprogparms.edictsize = sizeof(menuedict_t);

	menuprogparms.entspawn = NULL;//void (*entspawn) (struct edict_s *ent);	//ent has been spawned, but may not have all the extra variables (that may need to be set) set
	menuprogparms.entcanfree = NULL;//bool (*entcanfree) (struct edict_s *ent);	//return true to stop ent from being freed
	menuprogparms.stateop = NULL;//StateOp;//void (*stateop) (float var, func_t func);
	menuprogparms.cstateop = NULL;//CStateOp;
	menuprogparms.cwstateop = NULL;//CWStateOp;
	menuprogparms.thinktimeop = NULL;//ThinkTimeOp;

	//used when loading a game
	menuprogparms.builtinsfor = NULL;//builtin_t *(*builtinsfor) (int num);	//must return a pointer to the builtins that were used before the state was saved.
	menuprogparms.loadcompleate = NULL;//void (*loadcompleate) (int edictsize);	//notification to reset any pointers.

	menuprogparms.memalloc = PR_Malloc;//void *(*memalloc) (int size);	//small string allocation	malloced and freed randomly
	menuprogparms.memfree = PR_Free;//void (*memfree) (void * mem);


	menuprogparms.globalbuiltins = menu_builtins;//builtin_t *globalbuiltins;	//these are available to all progs
	menuprogparms.numglobalbuiltins = menu_numbuiltins;

	menuprogparms.autocompile = PR_COMPILECHANGED;//enum {PR_NOCOMPILE, PR_COMPILENEXIST, PR_COMPILECHANGED, PR_COMPILEALWAYS} autocompile;

	menuprogparms.gametime = &menutime;

	menuprogparms.sv_edicts = (edict_t **)&menu_edicts;
	menuprogparms.sv_num_edicts = &num_menu_edicts;

	menuprogparms.useeditor = NULL;//sorry... QCEditor;//void (*useeditor) (char *filename, int line, int nump, char **parms);

	menutime = Sys_DoubleTime();
	if (!menuprogs)
	{
		menuprogs = InitProgs(&menuprogparms);
		PR_Configure(menuprogs, NULL, -1, 1);
		if (PR_LoadProgs(menuprogs, "menu.dat", 10020, NULL, 0) < 0) //no per-progs builtins.
		{
			//failed to load or something
			M_Init_Internal();
			return;
		}
		if (setjmp(mp_abort))
		{
			M_Init_Internal();
			return;
		}
		inmenuprogs++;

		mp_time = (float*)PR_FindGlobal(menuprogs, "time", 0);
		if (mp_time)
			*mp_time = Sys_DoubleTime();

		menuentsize = PR_InitEnts(menuprogs, 512);


		//'world' edict
		EDICT_NUM(menuprogs, 0)->readonly = true;
		EDICT_NUM(menuprogs, 0)->isfree = false;


		mp_init_function = PR_FindFunction(menuprogs, "m_init", PR_ANY);
		mp_shutdown_function = PR_FindFunction(menuprogs, "m_shutdown", PR_ANY);
		mp_draw_function = PR_FindFunction(menuprogs, "m_draw", PR_ANY);
		mp_keydown_function = PR_FindFunction(menuprogs, "m_keydown", PR_ANY);
		mp_keyup_function = PR_FindFunction(menuprogs, "m_keyup", PR_ANY);
		mp_toggle_function = PR_FindFunction(menuprogs, "m_toggle", PR_ANY);

		if (mp_init_function)
			PR_ExecuteProgram(menuprogs, mp_init_function);
		inmenuprogs--;
	}
}

void MP_Draw(void)
{
	if (setjmp(mp_abort))
		return;

#ifdef RGLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		GL_TexEnv(GL_MODULATE);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
	}
#endif

	menutime = Sys_DoubleTime();
	if (mp_time)
		*mp_time = menutime;

	inmenuprogs++;
	if (mp_draw_function)
		PR_ExecuteProgram(menuprogs, mp_draw_function);
	inmenuprogs--;
}

int MP_TranslateFTEtoDPCodes(int code)
{
	switch(code)
	{
	case K_TAB:				return 9;
	case K_ENTER:			return 13;
	case K_ESCAPE:			return 27;
	case K_SPACE:			return 32;
	case K_BACKSPACE:		return 127;
	case K_UPARROW:			return 128;
	case K_DOWNARROW:		return 129;
	case K_LEFTARROW:		return 130;
	case K_RIGHTARROW:		return 131;
	case K_ALT:				return 132;
	case K_CTRL:			return 133;
	case K_SHIFT:			return 134;
	case K_F1:				return 135;
	case K_F2:				return 136;
	case K_F3:				return 137;
	case K_F4:				return 138;
	case K_F5:				return 139;
	case K_F6:				return 140;
	case K_F7:				return 141;
	case K_F8:				return 142;
	case K_F9:				return 143;
	case K_F10:				return 144;
	case K_F11:				return 145;
	case K_F12:				return 146;
	case K_INS:				return 147;
	case K_DEL:				return 148;
	case K_PGDN:			return 149;
	case K_PGUP:			return 150;
	case K_HOME:			return 151;
	case K_END:				return 152;
	case K_KP_HOME:			return 160;
	case K_KP_UPARROW:		return 161;
	case K_KP_PGUP:			return 162;
	case K_KP_LEFTARROW:	return 163;
	case K_KP_5:			return 164;
	case K_KP_RIGHTARROW:	return 165;
	case K_KP_END:			return 166;
	case K_KP_DOWNARROW:	return 167;
	case K_KP_PGDN:			return 168;
	case K_KP_ENTER:		return 169;
	case K_KP_INS:			return 170;
	case K_KP_DEL:			return 171;
	case K_KP_SLASH:		return 172;
	case K_KP_MINUS:		return 173;
	case K_KP_PLUS:			return 174;
	case K_PAUSE:			return 255;
	case K_JOY1:			return 768;
	case K_JOY2:			return 769;
	case K_JOY3:			return 770;
	case K_JOY4:			return 771;
	case K_AUX1:			return 772;
	case K_AUX2:			return 773;
	case K_AUX3:			return 774;
	case K_AUX4:			return 775;
	case K_AUX5:			return 776;
	case K_AUX6:			return 777;
	case K_AUX7:			return 778;
	case K_AUX8:			return 779;
	case K_AUX9:			return 780;
	case K_AUX10:			return 781;
	case K_AUX11:			return 782;
	case K_AUX12:			return 783;
	case K_AUX13:			return 784;
	case K_AUX14:			return 785;
	case K_AUX15:			return 786;
	case K_AUX16:			return 787;
	case K_AUX17:			return 788;
	case K_AUX18:			return 789;
	case K_AUX19:			return 790;
	case K_AUX20:			return 791;
	case K_AUX21:			return 792;
	case K_AUX22:			return 793;
	case K_AUX23:			return 794;
	case K_AUX24:			return 795;
	case K_AUX25:			return 796;
	case K_AUX26:			return 797;
	case K_AUX27:			return 798;
	case K_AUX28:			return 799;
	case K_AUX29:			return 800;
	case K_AUX30:			return 801;
	case K_AUX31:			return 802;
	case K_AUX32:			return 803;
	case K_MOUSE1:			return 512;
	case K_MOUSE2:			return 513;
	case K_MOUSE3:			return 514;
	case K_MOUSE4:			return 515;
	case K_MOUSE5:			return 516;
//	case K_MOUSE6:			return 517;
//	case K_MOUSE7:			return 518;
//	case K_MOUSE8:			return 519;
//	case K_MOUSE9:			return 520;
//	case K_MOUSE10:			return 521;
	case K_MWHEELDOWN:		return 515;//K_MOUSE4;
	case K_MWHEELUP:		return 516;//K_MOUSE5;
	default:				return code;
	}
}

int MP_TranslateDPtoFTECodes(int code)
{
	switch(code)
	{
	case 9:			return K_TAB;
	case 13:		return K_ENTER;
	case 27:		return K_ESCAPE;
	case 32:		return K_SPACE;
	case 127:		return K_BACKSPACE;
	case 128:		return K_UPARROW;
	case 129:		return K_DOWNARROW;
	case 130:		return K_LEFTARROW;
	case 131:		return K_RIGHTARROW;
	case 132:		return K_ALT;
	case 133:		return K_CTRL;
	case 134:		return K_SHIFT;
	case 135:		return K_F1;
	case 136:		return K_F2;
	case 137:		return K_F3;
	case 138:		return K_F4;
	case 139:		return K_F5;
	case 140:		return K_F6;
	case 141:		return K_F7;
	case 142:		return K_F8;
	case 143:		return K_F9;
	case 144:		return K_F10;
	case 145:		return K_F11;
	case 146:		return K_F12;
	case 147:		return K_INS;
	case 148:		return K_DEL;
	case 149:		return K_PGDN;
	case 150:		return K_PGUP;
	case 151:		return K_HOME;
	case 152:		return K_END;
	case 160:		return K_KP_HOME;
	case 161:		return K_KP_UPARROW;
	case 162:		return K_KP_PGUP;
	case 163:		return K_KP_LEFTARROW;
	case 164:		return K_KP_5;
	case 165:		return K_KP_RIGHTARROW;
	case 166:		return K_KP_END;
	case 167:		return K_KP_DOWNARROW;
	case 168:		return K_KP_PGDN;
	case 169:		return K_KP_ENTER;
	case 170:		return K_KP_INS;
	case 171:		return K_KP_DEL;
	case 172:		return K_KP_SLASH;
	case 173:		return K_KP_MINUS;
	case 174:		return K_KP_PLUS;
	case 255:		return K_PAUSE;

	case 768:		return K_JOY1;
	case 769:		return K_JOY2;
	case 770:		return K_JOY3;
	case 771:		return K_JOY4;
	case 772:		return K_AUX1;
	case 773:		return K_AUX2;
	case 774:		return K_AUX3;
	case 775:		return K_AUX4;
	case 776:		return K_AUX5;
	case 777:		return K_AUX6;
	case 778:		return K_AUX7;
	case 779:		return K_AUX8;
	case 780:		return K_AUX9;
	case 781:		return K_AUX10;
	case 782:		return K_AUX11;
	case 783:		return K_AUX12;
	case 784:		return K_AUX13;
	case 785:		return K_AUX14;
	case 786:		return K_AUX15;
	case 787:		return K_AUX16;
	case 788:		return K_AUX17;
	case 789:		return K_AUX18;
	case 790:		return K_AUX19;
	case 791:		return K_AUX20;
	case 792:		return K_AUX21;
	case 793:		return K_AUX22;
	case 794:		return K_AUX23;
	case 795:		return K_AUX24;
	case 796:		return K_AUX25;
	case 797:		return K_AUX26;
	case 798:		return K_AUX27;
	case 799:		return K_AUX28;
	case 800:		return K_AUX29;
	case 801:		return K_AUX30;
	case 802:		return K_AUX31;
	case 803:		return K_AUX32;
	case 512:		return K_MOUSE1;
	case 513:		return K_MOUSE2;
	case 514:		return K_MOUSE3;
//	case 515:		return K_MOUSE4;
//	case 516:		return K_MOUSE5;
	case 517:		return K_MOUSE6;
	case 518:		return K_MOUSE7;
	case 519:		return K_MOUSE8;
//	case 520:		return K_MOUSE9;
//	case 521:		return K_MOUSE10;
	case 515:		return K_MWHEELDOWN;//K_MOUSE4;
	case 516:		return K_MWHEELUP;//K_MOUSE5;
	default:		return code;
	}
}

void MP_Keydown(int key)
{
	if (setjmp(mp_abort))
		return;

	if (key == 'c')
	{
		extern int keydown[];
		if (keydown[K_CTRL])
		{
			MP_Shutdown();
			return;
		}
	}

	menutime = Sys_DoubleTime();
	if (mp_time)
		*mp_time = menutime;

	inmenuprogs++;
	if (mp_keydown_function)
	{
		void *pr_globals = PR_globals(menuprogs, PR_CURRENT);
		G_FLOAT(OFS_PARM0) = MP_TranslateFTEtoDPCodes(key);
		if (G_FLOAT(OFS_PARM0) > 127)
			G_FLOAT(OFS_PARM1) = 0;
		else
			G_FLOAT(OFS_PARM1) = G_FLOAT(OFS_PARM0);
		PR_ExecuteProgram(menuprogs, mp_keydown_function);
	}
	inmenuprogs--;
}

void MP_Keyup(int key)
{
	if (setjmp(mp_abort))
		return;

	menutime = Sys_DoubleTime();
	if (mp_time)
		*mp_time = menutime;

	inmenuprogs++;
	if (mp_keyup_function)
	{
		void *pr_globals = PR_globals(menuprogs, PR_CURRENT);
		G_FLOAT(OFS_PARM0) = MP_TranslateFTEtoDPCodes(key);
		if (G_FLOAT(OFS_PARM0) > 127)
			G_FLOAT(OFS_PARM1) = 0;
		else
			G_FLOAT(OFS_PARM1) = G_FLOAT(OFS_PARM0);
		PR_ExecuteProgram(menuprogs, mp_keyup_function);
	}
	inmenuprogs--;
}

qboolean MP_Toggle(void)
{
	if (!menuprogs)
		return false;

	if (setjmp(mp_abort))
		return false;

	menutime = Sys_DoubleTime();
	if (mp_time)
		*mp_time = menutime;

	inmenuprogs++;
	if (mp_toggle_function)
		PR_ExecuteProgram(menuprogs, mp_toggle_function);
	inmenuprogs--;

	return true;
}
#endif
