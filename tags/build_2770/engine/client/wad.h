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
// wad.h

//===============
//   TYPES
//===============

#define	CMP_NONE		0
#define	CMP_LZSS		1

#define	TYP_NONE		0
#define	TYP_LABEL		1

#define	TYP_LUMPY		64				// 64 + grab command number
#define	TYP_PALETTE		64
#define	TYP_QTEX		65
#define	TYP_QPIC		66
#define	TYP_SOUND		67
#define	TYP_MIPTEX		68

//on disk representation of most q1 images.
typedef struct
{
	int			width, height;
	qbyte		data[4];			// variably sized
} qpic_t;

//this is what's actually used.
#define MPIC_ALPHA 1
typedef struct	//use this so we don't have to go slow over pics, and don't have to shift too much data around.
{
	unsigned int	width;	//keeps alignment (which is handy in 32bit modes)
	unsigned short	height;
	qbyte flags;
	qbyte pad;
	qbyte		data[4];			// variably sized
} mpic_t;


extern	mpic_t		*draw_disc;	// also used on sbar


typedef struct
{
	char		identification[4];		// should be WAD2 or 2DAW
	int			numlumps;
	int			infotableofs;
} wadinfo_t;

typedef struct
{
	int			filepos;
	int			disksize;
	int			size;					// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];				// must be null terminated
} lumpinfo_t;

extern	int			wad_numlumps;
extern	lumpinfo_t	*wad_lumps;
extern	qbyte		*wad_base;

void	W_LoadWadFile (char *filename);
void	W_CleanupName (char *in, char *out);
lumpinfo_t	*W_GetLumpinfo (char *name);
void	*W_GetLumpName (char *name);
void	*W_SafeGetLumpName (char *name);
void	*W_GetLumpNum (int num);
void Wads_Flush (void);

void SwapPic (qpic_t *pic);



void Mod_ParseWadsFromEntityLump(char *data);
qbyte *W_ConvertWAD3Texture(miptex_t *tex, int *width, int *height, qboolean *usesalpha);
void Mod_ParseInfoFromEntityLump(char *data);
qboolean Wad_NextDownload (void);
qbyte *W_GetTexture(char *name, int *width, int *height, qboolean *usesalpha);
