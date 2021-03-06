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

// the status bar is only redrawn if something has changed, but if anything
// does, the entire thing will be redrawn for the next vid.numpages frames.

#define	SBAR_HEIGHT		24

extern	int			sb_lines;			// scan lines to draw

void Sbar_Init (void);
struct player_info_s;
qboolean Sbar_UpdateTeamStatus(struct player_info_s *player, char *status);
#ifdef GLQUAKE
void Sbar_ReInit (void);
#endif

void Sbar_Changed (void);
// call whenever any of the client stats represented on the sbar changes

qboolean Sbar_ShouldDraw(void);
void Sbar_Draw (void);
void Sbar_DrawScoreboard (void);
// called every frame by screen

void Sbar_IntermissionOverlay (void);
// called each frame after the level has been completed

void Sbar_FinaleOverlay (void);
void Sbar_SortFrags (qboolean includespec, qboolean teamsort);

void Sbar_PQ_Team_New(unsigned int team, unsigned int shirt);
void Sbar_PQ_Team_Frags(unsigned int team, int frags);
void Sbar_PQ_Team_Reset(void);

void Sbar_Start (void);
void Sbar_Flush (void);
unsigned int	Sbar_ColorForMap (unsigned int m);

extern int scoreboardlines;
extern int fragsort[];
