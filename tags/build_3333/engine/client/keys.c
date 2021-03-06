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
#ifdef _WIN32
#include "winquake.h"
#endif
/*

key up events are sent even if in console mode

*/
void Editor_Key(int key);

#define		KEY_MODIFIERSTATES 8
#define		MAXCMDLINE	256
char	key_lines[32][MAXCMDLINE];
int		key_linepos;
int		shift_down=false;
int		key_lastpress;

int		edit_line=0;
int		history_line=0;

keydest_t	key_dest;

int		key_count;			// incremented every key event

int con_mousedown[3];

char	*keybindings[K_MAX][KEY_MODIFIERSTATES];
qbyte	bindcmdlevel[K_MAX][KEY_MODIFIERSTATES];
qboolean	consolekeys[K_MAX];	// if true, can't be rebound while in console
qboolean	menubound[K_MAX];	// if true, can't be rebound while in menu
int		keyshift[K_MAX];		// key to map to if shift held down in console
int		key_repeats[K_MAX];	// if > 1, it is autorepeating
qboolean	keydown[K_MAX];

qboolean deltaused[K_MAX][KEY_MODIFIERSTATES];

void Con_Selectioncolour_Callback(struct cvar_s *var, char *oldvalue);

extern cvar_t con_displaypossibilities;
cvar_t con_selectioncolour = SCVARFC("con_selectioncolour", "0", CVAR_RENDERERCALLBACK, Con_Selectioncolour_Callback);
extern cvar_t cl_chatmode;

static int KeyModifier (qboolean shift, qboolean alt, qboolean ctrl)
{
	int stateset = 0;
	if (shift)
		stateset |= 1;
	if (alt)
		stateset |= 2;
	if (ctrl)
		stateset |= 4;

	return stateset;
}

typedef struct
{
	char	*name;
	int		keynum;
} keyname_t;

keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},
	
	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	
	{"KP_HOME",		K_KP_HOME},
	{"KP_UPARROW",	K_KP_UPARROW},
	{"KP_PGUP",		K_KP_PGUP},
	{"KP_LEFTARROW", K_KP_LEFTARROW},
	{"KP_5",		K_KP_5},
	{"KP_RIGHTARROW", K_KP_RIGHTARROW},
	{"KP_END",		K_KP_END},
	{"KP_DOWNARROW",	K_KP_DOWNARROW},
	{"KP_PGDN",		K_KP_PGDN},
	{"KP_ENTER",	K_KP_ENTER},
	{"KP_INS",		K_KP_INS},
	{"KP_DEL",		K_KP_DEL},
	{"KP_SLASH",	K_KP_SLASH},
	{"KP_MINUS",	K_KP_MINUS},
	{"KP_PLUS",		K_KP_PLUS},
	{"KP_NUMLOCK",	K_KP_NUMLOCK},
	{"KP_STAR",		K_KP_STAR},
	{"KP_EQUALS",	K_KP_EQUALS},

	//fuhquake compatible.
	{"KP_0",		K_KP_INS},
	{"KP_1",		K_KP_END},
	{"KP_2",		K_KP_DOWNARROW},
	{"KP_3",		K_KP_PGDN},
	{"KP_4",		K_KP_LEFTARROW},
	{"KP_6",		K_KP_RIGHTARROW},
	{"KP_7",		K_KP_HOME},
	{"KP_8",		K_KP_UPARROW},
	{"KP_9",		K_KP_PGUP},

	{"MOUSE1",	K_MOUSE1},
	{"MOUSE2",	K_MOUSE2},
	{"MOUSE3",	K_MOUSE3},
	{"MOUSE4",	K_MOUSE4},
	{"MOUSE5",	K_MOUSE5},
	{"MOUSE6",	K_MOUSE6},
	{"MOUSE7",	K_MOUSE7},
	{"MOUSE8",	K_MOUSE8},
	{"MOUSE9",	K_MOUSE9},
	{"MOUSE10",	K_MOUSE10},

	{"LWIN",	K_LWIN},
	{"RWIN",	K_RWIN},
	{"APP",		K_APP},

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},
	{"AUX17", K_AUX17},
	{"AUX18", K_AUX18},
	{"AUX19", K_AUX19},
	{"AUX20", K_AUX20},
	{"AUX21", K_AUX21},
	{"AUX22", K_AUX22},
	{"AUX23", K_AUX23},
	{"AUX24", K_AUX24},
	{"AUX25", K_AUX25},
	{"AUX26", K_AUX26},
	{"AUX27", K_AUX27},
	{"AUX28", K_AUX28},
	{"AUX29", K_AUX29},
	{"AUX30", K_AUX30},
	{"AUX31", K_AUX31},
	{"AUX32", K_AUX32},

	{"PAUSE", K_PAUSE},

	{"MWHEELUP", K_MWHEELUP},
	{"MWHEELDOWN", K_MWHEELDOWN},

	{"CAPSLOCK", K_CAPSLOCK},
	{"SCROLLLOCK", K_SCRLCK},

	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands

	{NULL,0}
};

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/

qboolean Cmd_IsCommand (char *line)
{
	char	command[128];
	char	*cmd, *s;
	int		i;

	s = line;

	for (i=0 ; i<127 ; i++)
		if (s[i] <= ' ' || s[i] == ';')
			break;
		else
			command[i] = s[i];
	command[i] = 0;

	cmd = Cmd_CompleteCommand (command, true, false, -1);
	if (!cmd  || strcmp (cmd, command) )
		return false;		// just a chat message
	return true;
}

#define COLUMNWIDTH 20
#define MINCOLUMNWIDTH 18

int PaddedPrint (char *s, int x)
{
	int	nextcolx = 0;

	if (x)
		nextcolx = (int)((x + COLUMNWIDTH)/COLUMNWIDTH)*COLUMNWIDTH;

	if (nextcolx > con_main.linewidth - MINCOLUMNWIDTH
		|| (x && nextcolx + strlen(s) >= con_main.linewidth))
	{
		Con_Printf ("\n");
		x=0;
	}

	if (x)
	{
		Con_Printf (" ");
		x++;
	}
	while (x % COLUMNWIDTH)
	{
		Con_Printf (" ");
		x++;
	}
	Con_Printf ("%s", s);
	x+=strlen(s);

	return x;
}


int con_commandmatch;
void CompleteCommand (qboolean force)
{
	int i;
	char	*cmd, *s;

	s = key_lines[edit_line]+1;
	if (*s == '\\' || *s == '/')
		s++;

	if (!force && con_displaypossibilities.value)
	{
		int x=0;
		for (i = 1; ; i++)
		{
			cmd = Cmd_CompleteCommand (s, true, true, i);
			if (!cmd)
				break;
			if (i == 1)
				Con_Printf("---------\n");
			x = PaddedPrint(cmd, x);
		}
		if (i != 1)
			Con_Printf("\n");
	}

	cmd = Cmd_CompleteCommand (s, true, true, 2);
	if (!cmd || force)
	{
		if (!force)
			cmd = Cmd_CompleteCommand (s, false, true, 1);
		else
			cmd = Cmd_CompleteCommand (s, true, true, con_commandmatch);
		if (cmd)
		{
			key_lines[edit_line][1] = '/';
			Q_strcpy (key_lines[edit_line]+2, cmd);
			key_linepos = Q_strlen(cmd)+2;

			s = key_lines[edit_line]+1;	//readjust to cope with the insertion of a /
			if (*s == '\\' || *s == '/')
				s++;

	//		if (strlen(cmd)>strlen(s))
			{
				cmd = Cmd_CompleteCommand (s, true, true, 0);
				if (cmd && !strcmp(s, cmd))	//also a compleate var
				{
					key_lines[edit_line][key_linepos] = ' ';
					key_linepos++;
				}
			}
			key_lines[edit_line][key_linepos] = 0;
			return;
		}
	}
	cmd = Cmd_CompleteCommand (s, false, true, 0);
	if (cmd)
	{
		i = key_lines[edit_line][1] == '/'?2:1;
		if (i != 2 || strcmp(key_lines[edit_line]+i, cmd))
		{	//if changed, compleate it
			key_lines[edit_line][1] = '/';
			Q_strcpy (key_lines[edit_line]+2, cmd);
			key_linepos = Q_strlen(cmd)+2;

			s = key_lines[edit_line]+1;	//readjust to cope with the insertion of a /
			if (*s == '\\' || *s == '/')
				s++;

			key_lines[edit_line][key_linepos] = 0;

			return;	//don't alter con_commandmatch if we compleated a tiny bit more
		}
	}

	con_commandmatch++;
	if (!Cmd_CompleteCommand(s, true, true, con_commandmatch))
		con_commandmatch = 1;
}

//lines typed at the main console enter here
void Con_ExecuteLine(console_t *con, char *line)
{
	con_commandmatch=1;
	if (line[0] == '\\' || line[0] == '/')
		Cbuf_AddText (line+1, RESTRICT_LOCAL);	// skip the >
	else if (cl_chatmode.value == 2 && Cmd_IsCommand(line))
		Cbuf_AddText (line, RESTRICT_LOCAL);	// valid command
#ifdef Q2CLIENT
	else if (cls.protocol == CP_QUAKE2)
		Cbuf_AddText (line, RESTRICT_LOCAL);	// send the command to the server via console, and let the server convert to chat
#endif
	else
	{	// convert to a chat message
		if (cl_chatmode.value == 1 || ((cls.state >= ca_connected && cl_chatmode.value == 2) && (strncmp(line, "say ", 4))))
		{
			if (keydown[K_CTRL])
				Cbuf_AddText ("say_team ", RESTRICT_LOCAL);
			else
				Cbuf_AddText ("say ", RESTRICT_LOCAL);
		}
		Cbuf_AddText (line, RESTRICT_LOCAL);	// skip the >
	}

	Cbuf_AddText ("\n", RESTRICT_LOCAL);
	Con_Printf ("]%s\n",line);
	if (cls.state == ca_disconnected)
		SCR_UpdateScreen ();	// force an update, because the command
									// may take some time
}

vec3_t sccolor;

void Con_Selectioncolour_Callback(struct cvar_s *var, char *oldvalue)
{
	if (qrenderer != QR_NONE && qrenderer != -1)
		SCR_StringToRGB(var->string, sccolor, 1);
}

/*
// TODO: fix this to be used as the common coord function for selection logic
void GetSelectionCoords(int *selectcoords)
{
	extern cvar_t vid_conwidth, vid_conheight;
	extern int mousecursor_x, mousecursor_y;
	int xpos, ypos;

	// convert to console coords
	xpos = (int)(mousecursor_x*vid_conwidth.value)/vid.width;
	ypos = (int)(mousecursor_y*vid_conheight.value)/vid.height;
}
*/

void Key_ConsoleDrawSelectionBox(void)
{
	extern cvar_t vid_conwidth, vid_conheight;
	extern int mousecursor_x, mousecursor_y;
	int xpos, ypos, temp;
	int xpos2, ypos2;
	int yadj;
	
	if (!con_mousedown[2])
		return;

	xpos2 = con_mousedown[0];
	ypos2 = con_mousedown[1];

	xpos = (int)((mousecursor_x*vid_conwidth.value)/(vid.width*8));
	ypos = (int)((mousecursor_y*vid_conheight.value)/(vid.height*8));

	if (xpos2 < 1)
		xpos2 = 1;
	if (xpos < 1)
		xpos = 1;
	if (xpos2 > con_current->linewidth)
		xpos2 = con_current->linewidth;
	if (xpos > con_current->linewidth)
		xpos = con_current->linewidth;
	if (xpos2 > xpos)
	{
		temp = xpos;
		xpos = xpos2;
		xpos2 = temp;
	}
	xpos++;
	if (ypos2 > ypos)
	{
		temp = ypos;
		ypos = ypos2;
		ypos2 = temp;
	}
	ypos++;

	yadj = (con_current->vislines-22) % 8;

	Draw_FillRGB(xpos2*8, yadj+ypos2*8, (xpos - xpos2)*8, (ypos - ypos2)*8, sccolor[0], sccolor[1], sccolor[2]);
}

void Key_ConsoleRelease(int key)
{
	if (key == K_MOUSE1)
		con_mousedown[2] = false;
	if (key == K_MOUSE2 && con_mousedown[2])
	{
		extern cvar_t vid_conwidth, vid_conheight;
		extern int mousecursor_x, mousecursor_y;
		int xpos, ypos, temp;
		char *buf, *bufhead;
		int x, y;

		con_mousedown[2] = false;

		xpos = (int)((mousecursor_x*vid_conwidth.value)/(vid.width*8));
		ypos = (int)((mousecursor_y*vid_conheight.value)/(vid.height*8));

		if (con_mousedown[0] < 1)
			con_mousedown[0] = 1;
		if (xpos < 1)
			xpos = 1;
		if (con_mousedown[0] > con_current->linewidth)
			con_mousedown[0] = con_current->linewidth;
		if (xpos > con_current->linewidth)
			xpos = con_current->linewidth;
		if (con_mousedown[0] > xpos)
		{
			temp = xpos;
			xpos = con_mousedown[0];
			con_mousedown[0] = temp;
		}
		xpos++;
		if (con_mousedown[1] > ypos)
		{
			temp = ypos;
			ypos = con_mousedown[1];
			con_mousedown[1] = temp;
		}
		ypos++;

		ypos += con_current->display-((con_current->vislines-22)/8)+1;
		con_mousedown[1] += con_current->display-((con_current->vislines-22)/8)+1;
		if (con_current->display != con_current->current)
		{
			ypos++;
			con_mousedown[1]++;
		}

		con_mousedown[0]--;
		xpos--;

		temp = (ypos - con_mousedown[1]) * (xpos - con_mousedown[0] + 2) + 1;

		bufhead = buf = Z_Malloc(temp);
		for (y = con_mousedown[1]; y < ypos; y++)
		{
			if (y != con_mousedown[1])
			{
				while(buf > bufhead && buf[-1] == ' ')
					buf--;
				*buf++ = '\r';
				*buf++ = '\n';
			}

			for (x = con_mousedown[0]; x < xpos; x++)
				*buf++ = con_current->text[x + ((y%con_current->totallines)*con_current->linewidth)]&127;
		}
		while(buf > bufhead && buf[-1] == ' ')
			buf--;
		*buf++ = '\0';

		Sys_SaveClipboard(bufhead);
		Z_Free(bufhead);
	}
}

/*
====================
Key_Console

Interactive line editing and console scrollback
====================
*/
void Key_Console (int key)
{
	char	*clipText;
	int upperconbound;

	if (con_current->redirect)
	{
		if (key == K_TAB)
		{	// command completion
			if (keydown[K_CTRL] || keydown[K_SHIFT])
			{
				Con_CycleConsole();
				return;
			}
		}
		con_current->redirect(con_current, key);
		return;
	}

	if ((key == K_MOUSE1 || key == K_MOUSE2))
	{
		extern cvar_t vid_conwidth, vid_conheight;
		extern int mousecursor_x, mousecursor_y;
		int xpos, ypos;
		xpos = (int)((mousecursor_x*vid_conwidth.value)/(vid.width*8));
		ypos = (int)((mousecursor_y*vid_conheight.value)/(vid.height*8));
		con_mousedown[0] = xpos;
		con_mousedown[1] = ypos;
		if (ypos == 0 && con_main.next)
		{
			console_t *con;
			for (con = &con_main; con; con = con->next)
			{
				if (con == &con_main)
					xpos -= 5;
				else
					xpos -= strlen(con->name)+1;
				if (xpos == -1)
					break;
				if (xpos < 0)
				{
					if (key == K_MOUSE2)
						Con_Destroy (con);
					else
						con_current = con;
					break;
				}
			}
		}
		else if (key == K_MOUSE2)
			con_mousedown[2] = true;
		else 
			con_mousedown[2] = false;

		return;
	}
	
	if (key == K_ENTER)
	{	// backslash text are commands, else chat
		int oldl = edit_line;
		edit_line = (edit_line + 1) & 31;
		history_line = edit_line;
		key_lines[edit_line][0] = ']';
		key_lines[edit_line][1] = '\0';
		key_linepos = 1;

		if (con_current->linebuffered)
			con_current->linebuffered(con_current, key_lines[oldl]+1);

		return;
	}

	if (key == K_SPACE && con_current->commandcompletion)
	{
		if (keydown[K_CTRL] && Cmd_CompleteCommand(key_lines[edit_line]+1, true, true, con_current->commandcompletion))
		{
			CompleteCommand (true);
			return;
		}
	}

	if (key == K_TAB)
	{	// command completion
		if (keydown[K_CTRL] || keydown[K_SHIFT])
		{
			Con_CycleConsole();
			return;
		}

		if (con_current->commandcompletion)
			CompleteCommand (false);
		return;
	}
	if (key != K_SHIFT)
		con_commandmatch=1;
	
	if (key == K_LEFTARROW)
	{
		if (key_linepos > 1)
			key_linepos--;
		return;
	}
	if (key == K_RIGHTARROW)
	{
		if (key_lines[edit_line][key_linepos])
		{
			key_linepos++;
			return;
		}
		else
			key = ' ';
	}

	if (key == K_DEL)
	{
		if (strlen(key_lines[edit_line]+key_linepos))
		{
			memmove(key_lines[edit_line]+key_linepos, key_lines[edit_line]+key_linepos+1, strlen(key_lines[edit_line]+key_linepos+1)+1);
			return;
		}
		else
			key = K_BACKSPACE;
	}

	if (key == K_BACKSPACE)
	{
		if (key_linepos > 1)
		{
			memmove(key_lines[edit_line]+key_linepos-1, key_lines[edit_line]+key_linepos, strlen(key_lines[edit_line]+key_linepos)+1);
			key_linepos--;
		}
		return;
	}

	if (key == K_UPARROW)
	{
		do
		{
			history_line = (history_line - 1) & 31;
		} while (history_line != edit_line
				&& !key_lines[history_line][1]);
		if (history_line == edit_line)
			history_line = (edit_line+1)&31;
		Q_strcpy(key_lines[edit_line], key_lines[history_line]);
		key_linepos = Q_strlen(key_lines[edit_line]);

		key_lines[edit_line][0] = ']';
		return;
	}

	if (key == K_DOWNARROW)
	{
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = ']';
			key_lines[edit_line][1] = '\0';
			key_linepos=1;
			return;
		}
		do
		{
			history_line = (history_line + 1) & 31;
		}
		while (history_line != edit_line
			&& !key_lines[history_line][1]);
		if (history_line == edit_line)
		{
			key_lines[edit_line][0] = ']';
			key_linepos = 1;
		}
		else
		{
			Q_strcpy(key_lines[edit_line], key_lines[history_line]);
			key_linepos = Q_strlen(key_lines[edit_line]);
		}
		return;
	}

	upperconbound = con_current->current - con_current->totallines + 1;

	if (key == K_PGUP || key==K_MWHEELUP)
	{
		// con_current->vislines actually contains the height of the console in
		// pixels (and not the number of lines). It's 22 pixels larger
		// than the text area to include borders I guess... weird shit.
		// - Molgrum
		if (keydown[K_CTRL])
		{
			if (con_current->display == con_current->current)
			{
				con_current->display -= ( ( con_current->vislines - 22 ) / 8 );
			}
			else
			{
				con_current->display -= ( ( con_current->vislines - 30 ) / 8 );
			}
		}
		else
			con_current->display -= 2;
		
		if (con_current->display < upperconbound)
			con_current->display = upperconbound;
		return;
	}

	if (key == K_PGDN || key==K_MWHEELDOWN)
	{
		// con_current->vislines actually contains the height of the console in
		// pixels (and not the number of lines). It's 22 pixels larger
		// than the text area to include borders I guess... weird shit.
		// - Molgrum
		if (keydown[K_CTRL])
			con_current->display += ( ( con_current->vislines - 30 ) / 8 );
		else
			con_current->display += 2;
		
		if (con_current->display < upperconbound)
			con_current->display = upperconbound;
		
		// Changed this to prevent the following scenario: PGUP, ENTER, PGDN ,(you'll
		// see the ^^^^ indicator), PGDN, (the indicator disappears but the console position
		// is still the same).
		// - Molgrum
		if (con_current->display >= ( con_current->current - 1 ))
			con_current->display = con_current->current;
		
		return;
	}

	if (key == K_HOME)
	{
		if (keydown[K_CTRL])
			con_current->display = upperconbound;
		else
			key_linepos = 1;
		return;
	}

	if (key == K_END)
	{
		if (keydown[K_CTRL])
			con_current->display = con_current->current;
		else
			key_linepos = strlen(key_lines[edit_line]);
		return;
	}

	if (((key=='C' || key=='c') && keydown[K_CTRL]) || (keydown[K_CTRL] && key == K_INS))
	{
		Sys_SaveClipboard(key_lines[edit_line]+1);
		return;
	}

	if (((key=='V' || key=='v') && keydown[K_CTRL]) || (keydown[K_SHIFT] && key == K_INS))
	{
		clipText = Sys_GetClipboard();
		if (clipText)
		{
			int i;
			int len;
			len = strlen(clipText);
			if (len + strlen(key_lines[edit_line]) > MAXCMDLINE - 1)
				len = MAXCMDLINE - 1 - strlen(key_lines[edit_line]);
			if (len > 0)
			{	// insert the string
				memmove (key_lines[edit_line] + key_linepos + len,
					key_lines[edit_line] + key_linepos, strlen(key_lines[edit_line]) - key_linepos + 1);
				memcpy (key_lines[edit_line] + key_linepos, clipText, len);
				for (i = 0; i < len; i++)
				{
					if (key_lines[edit_line][key_linepos+i] == '\r')
						key_lines[edit_line][key_linepos+i] = ' ';
					else if (key_lines[edit_line][key_linepos+i] == '\n')
						key_lines[edit_line][key_linepos+i] = ';';
				}
				key_linepos += len;
			}
			Sys_CloseClipboard(clipText);
		}
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (keydown[K_CTRL]) {
		if (key >= '0' && key <= '9')
				key = key - '0' + 0x12;	// yellow number
		else switch (key) {
			case '[': key = 0x10; break;
			case ']': key = 0x11; break;
			case 'g': key = 0x86; break;
			case 'r': key = 0x87; break;
			case 'y': key = 0x88; break;
			case 'b': key = 0x89; break;
			case '(': key = 0x80; break;
			case '=': key = 0x81; break;
			case ')': key = 0x82; break;
			case 'a': key = 0x83; break;
			case '<': key = 0x1d; break;
			case '-': key = 0x1e; break;
			case '>': key = 0x1f; break;
			case ',': key = 0x1c; break;
			case '.': key = 0x9c; break;
			case 'B': key = 0x8b; break;
			case 'C': key = 0x8d; break;
			case 'n': key = '\r'; break;
		}
	}

	if (keydown[K_ALT])
		key |= 128;		// red char


		
	if (key_linepos < MAXCMDLINE-1)
	{
		memmove(key_lines[edit_line]+key_linepos+1, key_lines[edit_line]+key_linepos, strlen(key_lines[edit_line]+key_linepos)+1);
		key_lines[edit_line][key_linepos] = key;
		key_linepos++;
//		key_lines[edit_line][key_linepos] = 0;
	}

}

//============================================================================

qboolean	chat_team;
char		chat_buffer[MAXCMDLINE];
int			chat_bufferlen = 0;

void Key_Message (int key)
{

	if (key == K_ENTER)
	{
		if (chat_buffer[0])
		{	//send it straight into the command.
			Cmd_TokenizeString(va("%s %s", chat_team?"say_team":"say", chat_buffer), true, false);
			CL_Say(chat_team, "");
		}

		key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key == K_ESCAPE)
	{
		key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}

	if (key < 32 || key > 127)
		return;	// non printable

	if (key == K_BACKSPACE)
	{
		if (chat_bufferlen)
		{
			chat_bufferlen--;
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}

	if (chat_bufferlen == sizeof(chat_buffer)-1)
		return; // all full

	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
}

//============================================================================

char *Key_GetBinding(int keynum)
{
	if (keynum >= 0 && keynum < K_MAX)
		return keybindings[keynum][0];
	return NULL;
}

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int Key_StringToKeynum (char *str, int *modifier)
{
	keyname_t	*kn;
	char *underscore;

	if (!strnicmp(str, "std_", 4))
		*modifier = 0;
	else
	{
		*modifier = 0;
		while(1)
		{
			underscore = strchr(str, '_');
			if (!underscore || !underscore[1])
				break;	//nothing afterwards or no underscore.
			if (!strnicmp(str, "shift_", 6))
				*modifier |= 1;
			else if (!strnicmp(str, "alt_", 4))
				*modifier |= 2;
			else if (!strnicmp(str, "ctrl_", 5))
				*modifier |= 4;
			else
				break;
			str = underscore+1;	//next char.
		}
		if (!*modifier)
			*modifier = ~0;
	}
	
	if (!str || !str[0])
		return -1;
	if (!str[1])	//single char.
		return str[0];

	if (!strncmp(str, "K_", 2))
		str+=2;

	for (kn=keynames ; kn->name ; kn++)
	{
		if (!Q_strcasecmp(str,kn->name))
			return kn->keynum;
	}
	if (atoi(str))	//assume ascii code. (prepend with a 0 if needed)
		return atoi(str);
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
char *Key_KeynumToString (int keynum)
{
	keyname_t	*kn;	
	static	char	tinystr[2];
	
	if (keynum == -1)
		return "<KEY NOT FOUND>";
	if (keynum > 32 && keynum < 127)
	{	// printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}
	
	for (kn=keynames ; kn->name ; kn++)
		if (keynum == kn->keynum)
			return kn->name;

	{
		if (keynum < 10)	//don't let it be a single character
			return va("0%i", keynum);
		return va("%i", keynum);
	}

	return "<UNKNOWN KEYNUM>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int keynum, int modifier, char *binding, int level)
{
	char	*newc;
	int		l;

	if (modifier == ~0)	//all of the possibilities.
	{
		for (l = 0; l < KEY_MODIFIERSTATES; l++)
			Key_SetBinding(keynum, l, binding, level);
		return;
	}
			
	if (keynum == -1)
		return;

// free old bindings
	if (keybindings[keynum][modifier])
	{
		Z_Free (keybindings[keynum][modifier]);
		keybindings[keynum][modifier] = NULL;
	}


	if (!binding)
	{
		keybindings[keynum][modifier] = NULL;
		return;
	}
// allocate memory for new binding
	l = Q_strlen (binding);	
	newc = Z_Malloc (l+1);
	Q_strcpy (newc, binding);
	newc[l] = 0;
	keybindings[keynum][modifier] = newc;
	bindcmdlevel[keynum][modifier] = level;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int		b, modifier;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1), &modifier);
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding (b, modifier, NULL, Cmd_ExecLevel);
}

void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i<K_MAX ; i++)
		if (keybindings[i])
			Key_SetBinding (i, ~0, NULL, Cmd_ExecLevel);
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int			i, c, b, modifier;
	char		cmd[1024];
	
	c = Cmd_Argc();

	if (c < 2)
	{
		Con_Printf ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1), &modifier);
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keybindings[b][0])
			Con_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b][0] );
		else
			Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
		return;
	}

	if (c > 3)
	{
		Cmd_ShiftArgs(1, Cmd_ExecLevel==RESTRICT_LOCAL);
		Key_SetBinding (b, modifier, Cmd_Args(), Cmd_ExecLevel);
		return;
	}
	
// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++)
	{
		Q_strncatz (cmd, Cmd_Argv(i), sizeof(cmd));
		if (i != (c-1))
			Q_strncatz (cmd, " ", sizeof(cmd));
	}

	Key_SetBinding (b, modifier, cmd, Cmd_ExecLevel);
}

void Key_BindLevel_f (void)
{
	int			i, c, b, modifier;
	char		cmd[1024];
	
	c = Cmd_Argc();

	if (c != 2 && c != 3)
	{
		Con_Printf ("bindat <key> [<level> <command>] : attach a command to a key for a specific level of access\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1), &modifier);
	if (b==-1)
	{
		Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keybindings[b])
			Con_Printf ("\"%s\" (%i)= \"%s\"\n", Cmd_Argv(1), bindcmdlevel[b][modifier], keybindings[b][modifier] );
		else
			Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
		return;
	}

	if (Cmd_IsInsecure())
	{
		Con_Printf("Server attempted usage of bindat\n");
		return;
	}

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=3 ; i< c ; i++)
	{
		Q_strncatz (cmd, Cmd_Argv(i), sizeof(cmd));
		if (i != (c-1))
			Q_strncatz (cmd, " ", sizeof(cmd));
	}

	Key_SetBinding (b, modifier, cmd, atoi(Cmd_Argv(2)));
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (vfsfile_t *f)
{
	char *s;
	int		i, m;
	char *binding, *base;

	char prefix[128];

	for (i=0 ; i<K_MAX ; i++)	//we rebind the key with all modifiers to get the standard bind, then change the specific ones.
	{						//this does two things, it normally allows us to skip 7 of the 8 possibilities
		base = keybindings[i][0];	//plus we can use the config with other clients.
		if (!base)
			base = "";
		for (m = 0; m < KEY_MODIFIERSTATES; m++)
		{
			binding = keybindings[i][m];
			if (!binding)
				binding = "";
			if (strcmp(binding, base) || (m==0 && keybindings[i][0]) || bindcmdlevel[i][m] != bindcmdlevel[i][0])
			{
				*prefix = '\0';
				if (m & 4)
					strcat(prefix, "CTRL_");
				if (m & 2)
					strcat(prefix, "ALT_");
				if (m & 1)
					strcat(prefix, "SHIFT_");

				if (bindcmdlevel[i][m] != bindcmdlevel[i][0])
				{
					if (i == ';')
						s = va("bindlevel \"%s%s\" %i \"%s\"\n", prefix, Key_KeynumToString(i), bindcmdlevel[i][m], keybindings[i][m]);
					else if (i == '\"')
						s = va("bindlevel \"%s%s\" %i \"%s\"\n", prefix, "\"\"", bindcmdlevel[i][m], keybindings[i][m]);
					else
						s = va("bindlevel %s%s %i \"%s\"\n", prefix, Key_KeynumToString(i), bindcmdlevel[i][m], keybindings[i][m]);
				}
				else
				{
					if (i == ';')
						s = va("bind \"%s%s\" \"%s\"\n", prefix, Key_KeynumToString(i), keybindings[i][m]);
					else if (i == '\"')
						s = va("bind \"%s%s\" \"%s\"\n", prefix, "\"\"", keybindings[i][m]);
					else
						s = va("bind %s%s \"%s\"\n", prefix, Key_KeynumToString(i), keybindings[i][m]);
				}
				VFS_WRITE(f, s, strlen(s));
			}
		}
	}
}


/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
	int		i;

	for (i=0 ; i<32 ; i++)
	{
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}
	key_linepos = 1;
	
//
// init ascii characters in console mode
//
	for (i=32 ; i<128 ; i++)
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_DEL] = true;
	consolekeys[K_HOME] = true;
	consolekeys[K_END] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
	consolekeys[K_CTRL] = true;
	consolekeys[K_ALT] = true;
	consolekeys['`'] = false;
	consolekeys['~'] = false;

	for (i=K_MOUSE1 ; i<K_MOUSE10 ; i++)
	{
		consolekeys[i] = true;
	}
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;

	for (i=0 ; i<K_MAX ; i++)
		keyshift[i] = i;
	for (i='a' ; i<='z' ; i++)
		keyshift[i] = i - 'a' + 'A';
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';

	menubound[K_ESCAPE] = true;
	for (i=0 ; i<12 ; i++)
		menubound[K_F1+i] = true;

//
// register our functions
//
	Cmd_AddCommand ("bind",Key_Bind_f);
	Cmd_AddCommand ("bindlevel",Key_BindLevel_f);
	Cmd_AddCommand ("unbind",Key_Unbind_f);
	Cmd_AddCommand ("unbindall",Key_Unbindall_f);

	Cvar_Register (&con_selectioncolour, "Console variables");
}

qboolean Key_MouseShouldBeFree(void)
{
	//returns if the mouse should be a cursor or if it should go to the menu

	//if true, the input code is expected to return mouse cursor positions rather than deltas

	extern int mouseusedforgui;
	if (mouseusedforgui)	//I don't like this
		return true;

	if (key_dest == key_menu)
	{
		if (m_state == m_complex || m_state == m_plugin)
			return true;
	}
	if (key_dest == key_console)
		return true;
	if (key_dest == key_game && cls.state < ca_connected)
		return true;

#ifdef VM_UI
	if (UI_MenuState())
		return true;
#endif


	return false;
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event (int key, qboolean down)
{
	char	*kb;
	char	cmd[1024];
	int keystate, oldstate;

//	Con_Printf ("%i : %i\n", key, down); //@@@

	oldstate = KeyModifier(keydown[K_SHIFT], keydown[K_ALT], keydown[K_CTRL]);

	keydown[key] = down;

	if (key == K_SHIFT || key == K_ALT || key == K_CTRL)
	{
		int k;

		keystate = KeyModifier(keydown[K_SHIFT], keydown[K_ALT], keydown[K_CTRL]);

		for (k = 0; k < K_MAX; k++)
		{	//go through the old state removing all depressed keys. they are all up now.

			if (k == K_SHIFT || k == K_ALT || k == K_CTRL)
				continue;

			if (deltaused[k][oldstate])
			{
				if (keybindings[k][oldstate] == keybindings[k][keystate] || !strcmp(keybindings[k][oldstate], keybindings[k][keystate]))
				{	//bindings match. skip this key
//					Con_Printf ("keeping bind %i\n", k); //@@@
					deltaused[k][oldstate] = false;
					deltaused[k][keystate] = true;
					continue;
				}

//				Con_Printf ("removing bind %i\n", k); //@@@

				deltaused[k][oldstate] = false;

				kb = keybindings[k][oldstate];
				if (kb && kb[0] == '+')
				{
					sprintf (cmd, "-%s %i\n", kb+1, k+oldstate*256);
					Cbuf_AddText (cmd, bindcmdlevel[k][oldstate]);
				}
				if (keyshift[k] != k)
				{
					kb = keybindings[keyshift[k]][oldstate];
					if (kb && kb[0] == '+')
					{
						sprintf (cmd, "-%s %i\n", kb+1, k+oldstate*256);
						Cbuf_AddText (cmd, bindcmdlevel[k][oldstate]);
					}
				}
			}
			if (keydown[k] && (key_dest != key_console && key_dest != key_message))
			{
				deltaused[k][keystate] = true;

//				Con_Printf ("adding bind %i\n", k); //@@@

				kb = keybindings[k][keystate];
				if (kb)
				{
					if (kb[0] == '+')
					{	// button commands add keynum as a parm
						sprintf (cmd, "%s %i\n", kb, k+keystate*256);
						Cbuf_AddText (cmd, bindcmdlevel[k][keystate]);
					}
					else
					{
						Cbuf_AddText (kb, bindcmdlevel[k][keystate]);
						Cbuf_AddText ("\n", bindcmdlevel[k][keystate]);
					}
				}
			}
		}

		keystate = oldstate = 0;
	}
	else
		keystate = oldstate;

	if (!down)
		key_repeats[key] = 0;

	key_lastpress = key;
	key_count++;
	if (key_count <= 0)
	{
		return;		// just catching keys for Con_NotifyBox
	}

// update auto-repeat status
	if (down)
	{
		key_repeats[key]++;
		/*
		if (key != K_BACKSPACE 
			&& key != K_DEL
			&& key != K_PAUSE 
			&& key != K_PGUP 
			&& key != K_PGDN
			&& key != K_LEFTARROW 
			&& key != K_RIGHTARROW
			&& key != K_UPARROW 
			&& key != K_DOWNARROW
			&& key_repeats[key] > 1)
			return;	// ignore most autorepeats
		*/
			
//		if (key >= 200 && !keybindings[key])	//is this too annoying?
//			Con_Printf ("%s is unbound, hit F4 to set.\n", Key_KeynumToString (key) );
	}

	if (key == K_SHIFT)
		shift_down = down;

	//yes, csqc is allowed to steal the escape key.
	if (key != '`' && key != '~')
	if (key_dest == key_game)
	{
#ifdef CSQC_DAT
		if (CSQC_KeyPress(key, down))	//give csqc a chance to handle it.
			return;
#endif
#ifdef VM_CG
		if (CG_KeyPress(key, down))
			return;
#endif
	}

//
// handle escape specialy, so the user can never unbind it
//
	if (key == K_ESCAPE)
	{
#ifdef VM_UI
#ifdef TEXTEDITOR
		if (key_dest == key_game)
#endif
		{
			if (UI_KeyPress(key, down))	//Allow the UI to see the escape key. It is possible that a developer may get stuck at a menu.
				return;
		}
#endif

		if (!down)
		{
			if (key_dest == key_menu)
				M_Keyup (key);
			return;
		}
		switch (key_dest)
		{
		case key_message:
			Key_Message (key);
			break;
		case key_menu:
			M_Keydown (key);
			break;
#ifdef TEXTEDITOR
		case key_editor:
			Editor_Key (key);
			break;
#endif
		case key_game:
			if (Media_PlayingFullScreen())
			{
				Media_PlayFilm("");
				break;
			}
		case key_console:
			M_ToggleMenu_f ();
			break;
		default:
			Sys_Error ("Bad key_dest");
		}
		return;
	}

//
// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the keynum as a parameter, so multiple
// downs can be matched with ups
//
	if (!down)
	{
		switch (key_dest)
		{
		case key_menu:
			M_Keyup (key);
			break;
		case key_console:
			Key_ConsoleRelease(key);
			break;
		default:
			break;
		}

		if (!deltaused[key][keystate])	//this wasn't down, so don't make it leave down state.
			return;
		deltaused[key][keystate] = false;

		kb = keybindings[key][keystate];
		if (kb && kb[0] == '+')
		{
			sprintf (cmd, "-%s %i\n", kb+1, key+oldstate*256);
			Cbuf_AddText (cmd, bindcmdlevel[key][keystate]);
		}
		if (keyshift[key] != key)
		{
			kb = keybindings[keyshift[key]][keystate];
			if (kb && kb[0] == '+')
			{
				sprintf (cmd, "-%s %i\n", kb+1, key+oldstate*256);
				Cbuf_AddText (cmd, bindcmdlevel[key][keystate]);
			}
		}
		return;
	}

//
// during demo playback, most keys bring up the main menu
//

	if (cls.demoplayback && cls.demoplayback != DPB_MVD && cls.demoplayback != DPB_EZTV && down && consolekeys[key] && key != K_TAB && key_dest == key_game)
	{
		M_ToggleMenu_f ();
		return;
	}

//
// if not a consolekey, send to the interpreter no matter what mode is
//
#ifdef VM_UI
	if (key != '`' && key != '~')
	if (key_dest == key_game || !down)
	{
		if (UI_KeyPress(key, down) && down)	//UI is allowed to take these keydowns. Keyups are always maintained.
			return;
	}
#endif

	if ( (key_dest == key_menu && menubound[key])
	|| (key_dest == key_console && !consolekeys[key])
	|| (key_dest == key_game && ( cls.state == ca_active || !consolekeys[key] ) ) )
	{
		deltaused[key][keystate] = true;
		kb = keybindings[key][keystate];
		if (kb)
		{
			if (kb[0] == '+')
			{	// button commands add keynum as a parm
				sprintf (cmd, "%s %i\n", kb, key+oldstate*256);
				Cbuf_AddText (cmd, bindcmdlevel[key][keystate]);
			}
			else
			{
				Cbuf_AddText (kb, bindcmdlevel[key][keystate]);
				Cbuf_AddText ("\n", bindcmdlevel[key][keystate]);
			}
		}

		return;
	}

	if (shift_down)
		key = keyshift[key];

	if (!down)
	{
		switch (key_dest)
		{
		case key_menu:
			M_Keyup (key);
			break;
		default:
			break;
		}
		return;		// other systems only care about key down events
	}

	switch (key_dest)
	{
	case key_message:
		Key_Message (key);
		break;
	case key_menu:
		M_Keydown (key);
		break;
#ifdef TEXTEDITOR
	case key_editor:
		Editor_Key (key);
		break;
#endif
	case key_game:
	case key_console:
		Key_Console (key);
		break;
	default:
		Sys_Error ("Bad key_dest");
	}
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	for (i=0 ; i<K_MAX ; i++)
	{
		keydown[i] = false;
		key_repeats[i] = false;
	}
}

