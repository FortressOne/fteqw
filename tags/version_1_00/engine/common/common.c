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
// common.c -- misc functions used in client and server

#include "quakedef.h"

#include <ctype.h>
#include <errno.h>

// These 4 libraries required for the version command

#if defined(MINGW)
	#if defined(AVAIL_PNGLIB)  && !defined(SERVERONLY)
		#include "./mingw-libs/png.h"
	#endif
	#ifdef AVAIL_ZLIB
		#include "./mingw-libs/zlib.h"
	#endif
	#if defined(AVAIL_JPEGLIB) && !defined(SERVERONLY)
		#define JPEG_API VARGS
		//#include "./mingw-libs/jversion.h"
		#include "./mingw-libs/jpeglib.h"
	#endif
	#ifdef _SDL
		#include "./mingw-libs/SDL_version.h"
	#endif
#elif defined(_WIN32)
	#if defined(AVAIL_PNGLIB)  && !defined(SERVERONLY)
		#include "png.h"
	#endif
	#ifdef AVAIL_ZLIB
		#include "zlib.h"
	#endif
	#if defined(AVAIL_JPEGLIB) && !defined(SERVERONLY)
		#define JPEG_API VARGS
		//#include "jversion.h"
		#include "jpeglib.h"
	#endif
	#ifdef _SDL
		#include "SDL_version.h"
	#endif
#else
	#if defined(AVAIL_PNGLIB) && !defined(SERVERONLY)
		#include <png.h>
	#endif
	#ifdef AVAIL_ZLIB
		#include <zlib.h>
	#endif
	#if defined(AVAIL_JPEGLIB) && !defined(SERVERONLY)
		//#include <jversion.h>
		#include <jpeglib.h>
	#endif
	#ifdef _SDL
		#include <SDL_version.h>
	#endif
#endif

#undef malloc
#undef free


#define NUM_SAFE_ARGVS	6

usercmd_t nullcmd; // guarenteed to be zero

entity_state_t nullentitystate;	//this is the default state

static const char	*largv[MAX_NUM_ARGVS + NUM_SAFE_ARGVS + 1];
static char	*argvdummy = " ";

static char	*safeargvs[NUM_SAFE_ARGVS] =
	{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse"};

cvar_t	registered = CVARD("registered","0","Set if quake's pak1.pak is available");
cvar_t	gameversion = CVARFD("gameversion","", CVAR_SERVERINFO, "gamecode version for server browsers");
cvar_t	gameversion_min = CVARD("gameversion_min","", "gamecode version for server browsers");
cvar_t	gameversion_max = CVARD("gameversion_max","", "gamecode version for server browsers");
cvar_t	com_gamename = CVARD("com_gamename", "", "The game name used for dpmaster queries");
cvar_t	com_modname = CVARD("com_modname", "", "dpmaster information");
cvar_t	com_parseutf8 = CVARD("com_parseutf8", "0", "Interpret console messages/playernames/etc as UTF-8. Requires special fonts.");	//1 parse. 2 parse, but stop parsing that string if a char was malformed.

qboolean	com_modified;	// set true if using non-id files

qboolean		static_registered = true;	// only for startup check, then set

qboolean		msg_suppress_1 = false;

void COM_Path_f (void);
void COM_Dir_f (void);
void COM_Locate_f (void);


// if a packfile directory differs from this, it is assumed to be hacked
#define	PAK0_COUNT		339
#define	PAK0_CRC		52883

qboolean		standard_quake = true, rogue, hipnotic;

// this graphic needs to be in the pak file to use registered features
unsigned short pop[] =
{
 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
,0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000
,0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000
,0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600
,0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563
,0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564
,0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564
,0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563
,0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500
,0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200
,0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000
,0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.

*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}
void InsertLinkAfter (link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

void Q_strncpyz(char *d, const char *s, int n)
{
	int i;
	n--;
	if (n < 0)
		return;	//this could be an error

	for (i=0; *s; i++)
	{
		if (i == n)
			break;
		*d++ = *s++;
	}
	*d='\0';
}

//windows/linux have inconsistant snprintf
//this is an attempt to get them consistant and safe
//size is the total size of the buffer
void VARGS Q_snprintfz (char *dest, size_t size, char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr, fmt);
	vsnprintf (dest, size, fmt, argptr);
	va_end (argptr);

	dest[size-1] = 0;
}


#if 0
void Q_memset (void *dest, int fill, int count)
{
	int		i;

	if ( (((long)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill<<8) | (fill<<16) | (fill<<24);
		for (i=0 ; i<count ; i++)
			((int *)dest)[i] = fill;
	}
	else
		for (i=0 ; i<count ; i++)
			((qbyte *)dest)[i] = fill;
}

void Q_memcpy (void *dest, void *src, int count)
{
	int		i;

	if (( ( (long)dest | (long)src | count) & 3) == 0 )
	{
		count>>=2;
		for (i=0 ; i<count ; i++)
			((int *)dest)[i] = ((int *)src)[i];
	}
	else
		for (i=0 ; i<count ; i++)
			((qbyte *)dest)[i] = ((qbyte *)src)[i];
}

int Q_memcmp (void *m1, void *m2, int count)
{
	while(count)
	{
		count--;
		if (((qbyte *)m1)[count] != ((qbyte *)m2)[count])
			return -1;
	}
	return 0;
}

void Q_strcpy (char *dest, char *src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

void Q_strncpy (char *dest, char *src, int count)
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

int Q_strlen (char *str)
{
	int		count;

	count = 0;
	while (str[count])
		count++;

	return count;
}

char *Q_strrchr(char *s, char c)
{
    int len = Q_strlen(s);
    s += len;
    while (len--)
        if (*--s == c) return s;
    return 0;
}

void Q_strcat (char *dest, char *src)
{
	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
}

int Q_strcmp (char *s1, char *s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_strncmp (char *s1, char *s2, int count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_strncasecmp (char *s1, char *s2, int n)
{
	int		c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;		// strings not equal
		}
		if (!c1)
			return 0;		// strings are equal
//		s1++;
//		s2++;
	}

	return -1;
}

int Q_strcasecmp (char *s1, char *s2)
{
	return Q_strncasecmp (s1, s2, 99999);
}

#endif

char *Q_strlwr(char *s)
{
	char *ret=s;
	while(*s)
	{
		if (*s >= 'A' && *s <= 'Z')
			*s=*s-'A'+'a';
		s++;
	}

	return ret;
}

int wildcmp(const char *wild, const char *string)
{
	const char *cp=NULL, *mp=NULL;

	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?'))
		{
			return 0;
		}
		wild++;
		string++;
	}

	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild)	//a * at the end of the wild string matches anything the checked string has
			{
				string = strchr(string, '/');
				if (string && string[1])	/*don't match it if there's a / with something after it*/
					return 0;
				return 1;
			}
			mp = wild;
			cp = string+1;
		}
		else if ((*wild == *string) || (*wild == '?'))
		{
			wild++;
			string++;
		}
		else
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*')
	{
		wild++;
	}
	return !*wild;
}

// Q_ftoa: convert IEEE 754 float to a base-10 string with "infinite" decimal places
void Q_ftoa(char *str, float in)
{
	unsigned int i = *((int *)&in);

	int signbit = (i & 0x80000000) >> 31;
	int exp = (signed int)((i & 0x7F800000) >> 23) - 127;
	int mantissa = (i & 0x007FFFFF);

	if (exp == 128) // 255(NaN/Infinity bits) - 127(bias)
	{
		if (signbit)
		{
			*str = '-';
			str++;
		}
		if (mantissa == 0) // infinity
			strcpy(str, "1.#INF");
		else // NaN or indeterminate
			strcpy(str, "1.#NAN");
		return;
	}

	exp = -exp;
	exp = (int)(exp * 0.30102999957f); // convert base 2 to base 10
	exp += 8;

	if (exp <= 0)
		sprintf(str, "%.0f", in);
	else
	{
		char tstr[8];
		char *lsig = str - 1;
		sprintf(tstr, "%%.%if", exp);
		sprintf(str, tstr, in);
		// find last significant digit and trim
		while (*str)
		{
			if (*str >= '1' && *str <= '9')
				lsig = str;
			else if (*str == '.')
				lsig = str - 1;
			str++;
		}
		lsig[1] = '\0';
	}
}

int Q_atoi (const char *str)
{
	int		val;
	int		sign;
	int		c;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}

	return 0;
}


float Q_atof (const char *str)
{
	double	val;
	int		sign;
	int		c;
	int		decimal, total;

	while(*str == ' ')
		str++;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
			break;
		val = val*10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val*sign;
}

/*
============================================================================

					qbyte ORDER FUNCTIONS

============================================================================
*/

qboolean	bigendian;

short	(*BigShort) (short l);
short	(*LittleShort) (short l);
int	(*BigLong) (int l);
int	(*LittleLong) (int l);
float	(*BigFloat) (float l);
float	(*LittleFloat) (float l);

short   ShortSwap (short l)
{
	qbyte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short	ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	qbyte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float	f;
		qbyte	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

void COM_SwapLittleShortBlock (short *s, int size)
{
	if (size <= 0)
		return;

	if (!bigendian)
		return;

	while (size)
	{
		*s = ShortSwap(*s);
		s++;
		size--;
	}
}

void COM_CharBias (signed char *c, int size)
{
	if (size <= 0)
		return;

	while (size)
	{
		*c = (*(unsigned char *)c) - 128;
		c++;
		size--;
	}
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles qbyte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	qbyte	*buf;

#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error ("MSG_WriteChar: range error");
#endif

	buf = (qbyte*)SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	qbyte	*buf;

#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error ("MSG_WriteByte: range error");
#endif

	buf = (qbyte*)SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	qbyte	*buf;

#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Sys_Error ("MSG_WriteShort: range error");
#endif

	buf = (qbyte*)SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	qbyte	*buf;

	buf = (qbyte*)SZ_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float	f;
		int	l;
	} dat;


	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, Q_strlen(s)+1);
}

float MSG_FromCoord(coorddata c, int bytes)
{
	switch(bytes)
	{
	case 2:	//encode 1/8th precision, giving -4096 to 4096 map sizes
		return LittleShort(c.b2)/8.0f;
	case 4:
		return LittleFloat(c.f);
	default:
		Sys_Error("MSG_ToCoord: not a sane coordsize");
		return 0;
	}
}
coorddata MSG_ToCoord(float f, int bytes)	//return value should be treated as (char*)&ret;
{
	coorddata r;
	switch(bytes)
	{
	case 2:
		r.b4 = 0;
		if (f >= 0)
			r.b2 = LittleShort((short)(f*8+0.5f));
		else
			r.b2 = LittleShort((short)(f*8-0.5f));
		break;
	case 4:
		r.f = LittleFloat(f);
		break;
	default:
		Sys_Error("MSG_ToCoord: not a sane coordsize");
		r.b4 = 0;
	}

	return r;
}

coorddata MSG_ToAngle(float f, int bytes)	//return value is NOT byteswapped.
{
	coorddata r;
	switch(bytes)
	{
	case 1:
		r.b4 = 0;
		if (f >= 0)
			r.b[0] = (int)(f*(256.0f/360.0f) + 0.5f) & 255;
		else
			r.b[0] = (int)(f*(256.0f/360.0f) - 0.5f) & 255;
		break;
	case 2:
		r.b4 = 0;
		if (f >= 0)
			r.b2 = LittleShort((int)(f*(65536.0f/360.0f) + 0.5f) & 65535);
		else
			r.b2 = LittleShort((int)(f*(65536.0f/360.0f) - 0.5f) & 65535);
		break;
	case 4:
		r.f = LittleFloat(f);
		break;
	default:
		Sys_Error("MSG_ToCoord: not a sane coordsize");
		r.b4 = 0;
	}

	return r;
}

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	coorddata i = MSG_ToCoord(f, sb->prim.coordsize);
	SZ_Write (sb, (void*)&i, sb->prim.coordsize);
}

void MSG_WriteAngle16 (sizebuf_t *sb, float f)
{
	if (f >= 0)
		MSG_WriteShort (sb, (int)(f*(65536.0f/360.0f) + 0.5f) & 65535);
	else
		MSG_WriteShort (sb, (int)(f*(65536.0f/360.0f) - 0.5f) & 65535);
}
void MSG_WriteAngle8 (sizebuf_t *sb, float f)
{
	if (f >= 0)
		MSG_WriteByte (sb, (int)(f*(256.0f/360.0f) + 0.5f) & 255);
	else
		MSG_WriteByte (sb, (int)(f*(256.0f/360.0f) - 0.5f) & 255);
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	if (sb->prim.anglesize==2)
		MSG_WriteAngle16(sb, f);
	else if (sb->prim.anglesize==4)
		MSG_WriteFloat(sb, f);
	else
		MSG_WriteAngle8 (sb, f);
}

void MSG_WriteDeltaUsercmd (sizebuf_t *buf, usercmd_t *from, usercmd_t *cmd)
{
	int		bits;

//
// send the movement message
//
	bits = 0;
#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		if (cmd->angles[0] != from->angles[0])
			bits |= Q2CM_ANGLE1;
		if (cmd->angles[1] != from->angles[1])
			bits |= Q2CM_ANGLE2;
		if (cmd->angles[2] != from->angles[2])
			bits |= Q2CM_ANGLE3;
		if (cmd->forwardmove != from->forwardmove)
			bits |= Q2CM_FORWARD;
		if (cmd->sidemove != from->sidemove)
			bits |= Q2CM_SIDE;
		if (cmd->upmove != from->upmove)
			bits |= Q2CM_UP;
		if (cmd->buttons != from->buttons)
			bits |= Q2CM_BUTTONS;
		if (cmd->impulse != from->impulse)
			bits |= Q2CM_IMPULSE;

		MSG_WriteByte (buf, bits);

		if (bits & Q2CM_ANGLE1)
			MSG_WriteShort (buf, cmd->angles[0]);
		if (bits & Q2CM_ANGLE2)
			MSG_WriteShort (buf, cmd->angles[1]);
		if (bits & Q2CM_ANGLE3)
			MSG_WriteShort (buf, cmd->angles[2]);

		if (bits & Q2CM_FORWARD)
			MSG_WriteShort (buf, cmd->forwardmove);
		if (bits & Q2CM_SIDE)
	  		MSG_WriteShort (buf, cmd->sidemove);
		if (bits & Q2CM_UP)
			MSG_WriteShort (buf, cmd->upmove);

 		if (bits & Q2CM_BUTTONS)
	  		MSG_WriteByte (buf, cmd->buttons);
 		if (bits & Q2CM_IMPULSE)
			MSG_WriteByte (buf, cmd->impulse);
		MSG_WriteByte (buf, cmd->msec);

		MSG_WriteByte (buf, cmd->lightlevel);

	}
	else
#endif
	{
		if (cmd->angles[0] != from->angles[0])
			bits |= CM_ANGLE1;
		if (cmd->angles[1] != from->angles[1])
			bits |= CM_ANGLE2;
		if (cmd->angles[2] != from->angles[2])
			bits |= CM_ANGLE3;
		if (cmd->forwardmove != from->forwardmove)
			bits |= CM_FORWARD;
		if (cmd->sidemove != from->sidemove)
			bits |= CM_SIDE;
		if (cmd->upmove != from->upmove)
			bits |= CM_UP;
		if (cmd->buttons != from->buttons)
			bits |= CM_BUTTONS;
		if (cmd->impulse != from->impulse)
			bits |= CM_IMPULSE;

		MSG_WriteByte (buf, bits);

		if (bits & CM_ANGLE1)
			MSG_WriteShort (buf, cmd->angles[0]);
		if (bits & CM_ANGLE2)
			MSG_WriteShort (buf, cmd->angles[1]);
		if (bits & CM_ANGLE3)
			MSG_WriteShort (buf, cmd->angles[2]);

		if (bits & CM_FORWARD)
			MSG_WriteShort (buf, cmd->forwardmove);
		if (bits & CM_SIDE)
	  		MSG_WriteShort (buf, cmd->sidemove);
		if (bits & CM_UP)
			MSG_WriteShort (buf, cmd->upmove);

 		if (bits & CM_BUTTONS)
	  		MSG_WriteByte (buf, cmd->buttons);
 		if (bits & CM_IMPULSE)
			MSG_WriteByte (buf, cmd->impulse);
		MSG_WriteByte (buf, cmd->msec);
	}
}


//
// reading functions
//
int			msg_readcount;
qboolean	msg_badread;
struct netprim_s msg_nullnetprim;

void MSG_BeginReading (struct netprim_s prim)
{
	msg_readcount = 0;
	msg_badread = false;
	net_message.currentbit = 0;
	net_message.packing = SZ_RAWBYTES;
	net_message.prim = prim;
}

void MSG_ChangePrimitives(struct netprim_s prim)
{
	net_message.prim = prim;
}

int MSG_GetReadCount(void)
{
	return msg_readcount;
}


/*
============
MSG_ReadRawBytes
============
*/
static int MSG_ReadRawBytes(sizebuf_t *msg, int bits)
{
	int bitmask = 0;

	if (bits <= 8)
	{
		bitmask = (unsigned char)msg->data[msg_readcount];
		msg_readcount++;
		msg->currentbit += 8;
	}
	else if (bits <= 16)
	{
		bitmask = (unsigned short)(msg->data[msg_readcount]
			+ (msg->data[msg_readcount+1] << 8));
		msg_readcount += 2;
		msg->currentbit += 16;
	}
	else if (bits <= 32)
	{
		bitmask = msg->data[msg_readcount]
			+ (msg->data[msg_readcount+1] << 8)
			+ (msg->data[msg_readcount+2] << 16)
			+ (msg->data[msg_readcount+3] << 24);
		msg_readcount += 4;
		msg->currentbit += 32;
	}

	return bitmask;
}

/*
============
MSG_ReadRawBits
============
*/
static int MSG_ReadRawBits(sizebuf_t *msg, int bits)
 {
	int i;
	int val;
	int bitmask = 0;

	for(i=0 ; i<bits ; i++)
	{
		val = msg->data[msg->currentbit >> 3] >> (msg->currentbit & 7);
		msg->currentbit++;
		bitmask |= (val & 1) << i;
	}

	return bitmask;
}

#ifdef HUFFNETWORK
/*
============
MSG_ReadHuffBits
============
*/
static int MSG_ReadHuffBits(sizebuf_t *msg, int bits)
{
	int i;
	int val;
	int bitmask;
	int remaining = bits & 7;

	bitmask = MSG_ReadRawBits(msg, remaining);

	for (i=0 ; i<bits-remaining ; i+=8)
	{
		val = Huff_GetByte(msg->data, &msg->currentbit);
		bitmask |= val << (i + remaining);
	}

	msg_readcount = (msg->currentbit >> 3) + 1;

	return bitmask;
}
#endif

int MSG_ReadBits(int bits)
{
	int bitmask = 0;
	qboolean extend = false;

#ifdef PARANOID
	if (!bits || bits < -31 || bits > 32)
		Host_EndGame("MSG_ReadBits: bad bits %i", bits );
#endif

	if (bits < 0)
	{
		bits = -bits;
		extend = true;
	}

	switch(net_message.packing)
	{
	default:
	case SZ_BAD:
		Sys_Error("MSG_ReadBits: bad net_message.packing");
		break;
	case SZ_RAWBYTES:
		bitmask = MSG_ReadRawBytes(&net_message, bits);
		break;
	case SZ_RAWBITS:
		bitmask = MSG_ReadRawBits(&net_message, bits);
		break;
#ifdef HUFFNETWORK
	case SZ_HUFFMAN:
		bitmask = MSG_ReadHuffBits(&net_message, bits);
		break;
#endif
	}

	if (extend)
	{
		if(bitmask & (1 << (bits - 1)))
		{
			bitmask |= ~((1 << bits) - 1);
		}
	}

	return bitmask;
}

void MSG_ReadSkip(int bytes)
{
	if (net_message.packing!=SZ_RAWBYTES)
	{
		while (bytes > 4)
		{
			MSG_ReadBits(32);
			bytes-=4;
		}
		while (bytes > 0)
		{
			MSG_ReadBits(8);
			bytes--;
		}
	}
	if (msg_readcount+bytes > net_message.cursize)
	{
		msg_readcount = net_message.cursize;
		msg_badread = true;
		return;
	}
	msg_readcount += bytes;
}


// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar (void)
{
	int	c;

	if (net_message.packing!=SZ_RAWBYTES)
		return (signed char)MSG_ReadBits(8);

	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (signed char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadByte (void)
{
	unsigned char	c;

	if (net_message.packing!=SZ_RAWBYTES)
		return (unsigned char)MSG_ReadBits(8);

	if (msg_readcount+1 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (unsigned char)net_message.data[msg_readcount];
	msg_readcount++;

	return c;
}

int MSG_ReadShort (void)
{
	int	c;

	if (net_message.packing!=SZ_RAWBYTES)
		return (short)MSG_ReadBits(16);

	if (msg_readcount+2 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (short)(net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8));

	msg_readcount += 2;

	return c;
}

int MSG_ReadLong (void)
{
	int	c;

	if (net_message.packing!=SZ_RAWBYTES)
		return (int)MSG_ReadBits(32);

	if (msg_readcount+4 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = net_message.data[msg_readcount]
	+ (net_message.data[msg_readcount+1]<<8)
	+ (net_message.data[msg_readcount+2]<<16)
	+ (net_message.data[msg_readcount+3]<<24);

	msg_readcount += 4;

	return c;
}

float MSG_ReadFloat (void)
{
	union
	{
		qbyte	b[4];
		float	f;
		int	l;
	} dat;

	if (net_message.packing!=SZ_RAWBYTES)
	{
		dat.l = MSG_ReadBits(32);
		return dat.f;
	}

	if (msg_readcount+4 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	dat.b[0] =	net_message.data[msg_readcount];
	dat.b[1] =	net_message.data[msg_readcount+1];
	dat.b[2] =	net_message.data[msg_readcount+2];
	dat.b[3] =	net_message.data[msg_readcount+3];
	msg_readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;
}

char *MSG_ReadString (void)
{
	static char	string[2048];
	int		l,c;

	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (msg_badread || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);

	string[l] = 0;

	return string;
}

char *MSG_ReadStringLine (void)
{
	static char	string[2048];
	int		l,c;

	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (msg_badread || c == 0 || c == '\n')
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);

	string[l] = 0;

	return string;
}

float MSG_ReadCoord (void)
{
	coorddata c = {{0}};
	if (!net_message.prim.coordsize)
		net_message.prim.coordsize = 2;
	MSG_ReadData(&c, net_message.prim.coordsize);
	return MSG_FromCoord(c, net_message.prim.coordsize);
}

void MSG_ReadPos (vec3_t pos)
{
	pos[0] = MSG_ReadCoord();
	pos[1] = MSG_ReadCoord();
	pos[2] = MSG_ReadCoord();
}

#if defined(Q2SERVER) || !defined(SERVERONLY)
#define Q2NUMVERTEXNORMALS	162
vec3_t	bytedirs[Q2NUMVERTEXNORMALS] =
{
#include "../client/q2anorms.h"
};
#endif
#ifndef SERVERONLY
void MSG_ReadDir (vec3_t dir)
{
	int		b;

	b = MSG_ReadByte ();
	if (b >= Q2NUMVERTEXNORMALS)
	{
		CL_DumpPacket();
		Host_EndGame ("MSG_ReadDir: out of range");
	}
	VectorCopy (bytedirs[b], dir);
}
#endif
#ifdef Q2SERVER
void MSG_WriteDir (sizebuf_t *sb, vec3_t dir)
{
	int		i, best;
	float	d, bestd;

	if (!dir)
	{
		MSG_WriteByte (sb, 0);
		return;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<Q2NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	MSG_WriteByte (sb, best);
}
#endif

float MSG_ReadAngle16 (void)
{
	return MSG_ReadShort() * (360.0/65536);
}
float MSG_ReadAngle (void)
{
	if (!net_message.prim.anglesize)
		net_message.prim.anglesize = 1;

	switch(net_message.prim.anglesize)
	{
	case 2:
		return MSG_ReadAngle16();
	case 4:
		return MSG_ReadFloat();
	case 1:
		return MSG_ReadChar() * (360.0/256);
	default:
		Host_Error("Bad angle size\n");
		return 0;
	}
}

void MSG_ReadDeltaUsercmd (usercmd_t *from, usercmd_t *move)
{
	int bits;

	memcpy (move, from, sizeof(*move));

	bits = MSG_ReadByte ();

// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = MSG_ReadShort ();
	if (bits & CM_ANGLE2)
		move->angles[1] = MSG_ReadShort ();
	if (bits & CM_ANGLE3)
		move->angles[2] = MSG_ReadShort ();

// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = MSG_ReadShort ();
	if (bits & CM_SIDE)
		move->sidemove = MSG_ReadShort ();
	if (bits & CM_UP)
		move->upmove = MSG_ReadShort ();

// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadByte ();

	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte ();

// read time to run command
	move->msec = MSG_ReadByte ();
}

void MSGQ2_ReadDeltaUsercmd (usercmd_t *from, usercmd_t *move)
{
	int bits;

	memcpy (move, from, sizeof(*move));

	bits = MSG_ReadByte ();

// read current angles
	if (bits & Q2CM_ANGLE1)
		move->angles[0] = MSG_ReadShort ();
	if (bits & Q2CM_ANGLE2)
		move->angles[1] = MSG_ReadShort ();
	if (bits & Q2CM_ANGLE3)
		move->angles[2] = MSG_ReadShort ();

// read movement
	if (bits & Q2CM_FORWARD)
		move->forwardmove = MSG_ReadShort ();
	if (bits & Q2CM_SIDE)
		move->sidemove = MSG_ReadShort ();
	if (bits & Q2CM_UP)
		move->upmove = MSG_ReadShort ();

// read buttons
	if (bits & Q2CM_BUTTONS)
		move->buttons = MSG_ReadByte ();

	if (bits & Q2CM_IMPULSE)
		move->impulse = MSG_ReadByte ();

// read time to run command
	move->msec = MSG_ReadByte ();

	move->lightlevel = MSG_ReadByte ();
}

void MSG_ReadData (void *data, int len)
{
	int		i;

	for (i=0 ; i<len ; i++)
		((qbyte *)data)[i] = MSG_ReadByte ();
}


//===========================================================================

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
	buf->overflowed = false;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void	*data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Sys_Error ("SZ_GetSpace: overflow without allowoverflow set (%d)", buf->maxsize);

		if (length > buf->maxsize)
			Sys_Error ("SZ_GetSpace: %i is > full buffer size", length);

		Sys_Printf ("SZ_GetSpace: overflow (%i+%i bytes of %i)\n", buf->cursize, length, buf->maxsize);	// because Con_Printf may be redirected
		SZ_Clear (buf);
		buf->overflowed = true;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	Q_memcpy (SZ_GetSpace(buf,length),data,length);
}

void SZ_Print (sizebuf_t *buf, const char *data)
{
	int		len;

	len = Q_strlen(data)+1;

	if (!buf->cursize || buf->data[buf->cursize-1])
		Q_memcpy ((qbyte *)SZ_GetSpace(buf, len),data,len); // no trailing 0
	else
	{
		qbyte *msg;
		msg = (qbyte*)SZ_GetSpace(buf, len-1);
		if (msg == buf->data)	//whoops. SZ_GetSpace can return buf->data if it overflowed.
			msg++;
		Q_memcpy (msg-1,data,len); // write over trailing 0
	}
}


//============================================================================

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (const char *pathname)
{
	const char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return (char *)last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension (const char *in, char *out, int outlen)
{
	char *s;

	if (out != in)	//optimisation, most calls use the same buffer
		Q_strncpyz(out, in, outlen);

	s = out+strlen(out);

	while(*s != '/' && s != out)
	{
		if (*s == '.')
		{
			*s = 0;
			break;
		}

		s--;
	}
}

void COM_StripAllExtensions (char *in, char *out, int outlen)
{
	char *s;

	if (out != in)
		Q_strncpyz(out, in, outlen);

	s = out+strlen(out);

	while(*s != '/' && s != out)
	{
		if (*s == '.')
		{
			*s = 0;
		}

		s--;
	}
}

/*
============
COM_FileExtension
============
*/
char *COM_FileExtension (const char *in)
{
	static char exten[8];
	int		i;

	while (*in && *in != '.')
		in++;
	if (!*in)
		return "";
	in++;
	for (i=0 ; i<7 && *in ; i++,in++)
		exten[i] = *in;
	exten[i] = 0;
	return exten;
}

//Quake 2's tank model has a borked skin (or two).
void COM_CleanUpPath(char *str)
{
	char *dots;
	char *slash;
	int critisize = 0;
	for (dots = str; *dots; dots++)
	{
		if (*dots >= 'A' && *dots <= 'Z')
		{
			*dots = *dots - 'A' + 'a';
			critisize = 1;
		}
		else if (*dots == '\\')
		{
			*dots = '/';
			critisize = 2;
		}
	}
	while ((dots = strstr(str, "..")))
	{
		for (slash = dots-2; slash >= str; slash--)
		{
			if (*slash == '/')
			{
				memmove(slash, dots+2, strlen(dots+2)+1);
				critisize = 3;
				break;
			}
		}
	}
	while(*str == '/')
	{
		memmove(str, str+1, strlen(str+1)+1);
		critisize = 4;
	}
/*	if(critisize)
	{
		if (critisize == 1)	//not a biggy, so not red.
			Con_Printf("Please fix file case on your files\n");
		else if (critisize == 2)	//you're evil.
			Con_Printf("^1NEVER use backslash in a quake filename (we like portability)\n");
		else if (critisize == 3)	//compleatly stupid. The main reason why this function exists. Quake2 does it!
			Con_Printf("You realise that relative paths are a waste of space?\n");
		else if (critisize == 4)	//AAAAHHHHH! (consider sys_error instead)
			Con_Printf("^1AAAAAAAHHHH! An absolute path!\n");
	}
*/
}

/*
============
COM_FileBase
============
*/
void COM_FileBase (const char *in, char *out, int outlen)
{
	const char *s, *s2;

	s = in + strlen(in) - 1;

	while (s > in && *s != '.')
		s--;

	for (s2 = s ; s2 > in && *s2 && *s2 != '/' ; s2--)
		;

	if (s-s2 < 2)
	{
		if (s == s2)
			Q_strncpyz(out, in, outlen);
		else
			Q_strncpyz(out,"?model?", outlen);
	}
	else
	{
		s--;
		outlen--;
		if (outlen > s-s2)
			outlen = s-s2;
		Q_strncpyS (out,s2+1, outlen);
		out[outlen] = 0;
	}
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, char *extension, int maxlen)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	if (*extension != '.')
		Q_strncatz (path, ".", maxlen);
	Q_strncatz (path, extension, maxlen);
}




///=====================================

// This is the standard RGBI palette used in CGA text mode
consolecolours_t consolecolours[MAXCONCOLOURS] = {
	{0,    0,    0   }, // black
	{0,    0,    0.67}, // blue
	{0,    0.67, 0   }, // green
	{0,    0.67, 0.67}, // cyan
	{0.67, 0,    0   }, // red
	{0.67, 0,    0.67}, // magenta
	{0.67, 0.33, 0   }, // brown
	{0.67, 0.67, 0.67}, // light gray
	{0.33, 0.33, 0.33}, // dark gray
	{0.33, 0.33, 1   }, // light blue
	{0.33, 1,    0.33}, // light green
	{0.33, 1,    1   }, // light cyan
	{1,    0.33, 0.33}, // light red
	{1,    0.33, 1   }, // light magenta
	{1,    1,    0.33}, // yellow
	{1,    1,    1   }  // white
};

// This is for remapping the Q3 color codes to character masks, including ^9
conchar_t q3codemasks[MAXQ3COLOURS] = {
	0x00000000, // 0, black
	0x0c000000, // 1, red
	0x0a000000, // 2, green
	0x0e000000, // 3, yellow
	0x09000000, // 4, blue
	0x0b000000, // 5, cyan
	0x0d000000, // 6, magenta
	0x0f000000, // 7, white
	0x0f100000, // 8, half-alpha white (BX_COLOREDTEXT)
	0x07000000  // 9, "half-intensity" (BX_COLOREDTEXT)
};


//Strips out the flags
void COM_DeFunString(conchar_t *str, char *out, int outsize, qboolean ignoreflags)
{
	if (ignoreflags)
	{
		while(*str)
		{
			if (!--outsize)
				break;
			*out++ = (unsigned char)(*str++&255);
		}
		*out++ = 0;
	}
	else
	{
		int fl, d;
		//FIXME: TEST!

		fl = CON_WHITEMASK;
		while(*str)
		{
			if (!--outsize)
				break;
			if ((*str & CON_FLAGSMASK) != fl)
			{
				d = fl^(*str & CON_FLAGSMASK);
//				if (fl & CON_NONCLEARBG)	//not represented.
				if (d & CON_BLINKTEXT)
				{
					if (outsize<=2)
						break;
					outsize -= 2;
					*out++ = '^';
					*out++ = 'b';
				}
				if (d & CON_2NDCHARSETTEXT)
				{
					if (outsize<=2)
						break;
					outsize -= 2;
					*out++ = '^';
					*out++ = 'a';
				}
				if (d & CON_HALFALPHA)
				{
					if (outsize<=2)
						break;
					outsize -= 2;
					*out++ = '^';
					*out++ = 'h';
				}

				if (d & (CON_FGMASK | CON_BGMASK | CON_NONCLEARBG))
				{
					if (outsize<=4)
						break;
					outsize -= 4;

					d = (*str & CON_FLAGSMASK);
					*out++ = '^';
					*out++ = '&';
					if ((d & CON_FGMASK) == CON_WHITEMASK)
						*out++ = '-';
					else
						sprintf(out, "%X", d>>24);
					if (d & CON_NONCLEARBG)
					{
						sprintf(out, "%X", d>>28);
					}
					else
						*out++ = '-';
				}

				fl = (*str & CON_FLAGSMASK);
			}
			*out++ = (unsigned char)(*str++&255);
		}
		*out++ = 0;
	}
}

static int dehex(int i)
{
	if      (i >= '0' && i <= '9')
		return (i-'0');
	else if (i >= 'A' && i <= 'F')
		return (i-'A'+10);
	else
		return (i-'a'+10);
}

//Takes a q3-style fun string, and returns an expanded string-with-flags (actual return value is the null terminator)
//outsize parameter is in _BYTES_ (so sizeof is safe).
conchar_t *COM_ParseFunString(conchar_t defaultflags, const char *str, conchar_t *out, int outsize, qboolean keepmarkup)
{
	conchar_t extstack[4];
	int extstackdepth = 0;
	unsigned int uc, l;
	int utf8 = com_parseutf8.ival;

	conchar_t ext;

	outsize /= sizeof(conchar_t);
	if (!outsize)
		return out;
	//then outsize is decremented then checked before each write, so the trailing null has space

#if 0
	while(*str)
	{
		*out++ = CON_WHITEMASK|(unsigned char)*str++;
	}
	*out = 0;
	return out;
#endif

	if (*str == 1 || *str == 2)
	{
		if (com_parseutf8.ival)
			defaultflags = (defaultflags&~CON_FGMASK) | (COLOR_MAGENTA<<CON_FGSHIFT);
		else
			defaultflags |= CON_HIGHCHARSMASK;
		str++;
	}

	ext = defaultflags;

	while(*str)
	{
		if (*str & 0x80 && utf8)
		{	//check for utf-8

			//uc is the output unicode char
			uc = 0;
			//l is the length
			l = 0;

			if ((*str & 0xc0) == 0x80)
			{
				//one byte... malformed
				uc = '?';
			}
			else if ((*str & 0xe0) == 0xc0)
			{
				//two bytes
				if ((str[1] & 0xc0) == 0x80)
				{
					uc = ((str[0] & 0x1f)<<6) | (str[1] & 0x3f);
					if (uc > 0x7f)
						l = 2;
				}
			}
			else if ((*str & 0xf0) == 0xe0)
			{
				//three bytes
				if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80)
				{
					uc = ((str[0] & 0x0f)<<12) | ((str[1] & 0x3f)<<6) | ((str[2] & 0x3f)<<0);
					if (uc > 0x7ff)
						l = 3;
				}
			}
			else if ((*str & 0xf8) == 0xf0)
			{
				//four bytes
				if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80)
				{
					uc = ((str[0] & 0x07)<<18) | ((str[1] & 0x3f)<<12) | ((str[2] & 0x3f)<<6) | ((str[3] & 0x3f)<<0);
					if (uc > 0xffff)
						l = 4;
				}
			}
			else if ((*str & 0xfc) == 0xf8)
			{
				//five bytes
				if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
				{
					uc = ((str[0] & 0x03)<<24) | ((str[1] & 0x3f)<<18) | ((str[2] & 0x3f)<<12) | ((str[3] & 0x3f)<<6) | ((str[4] & 0x3f)<<0);
					if (uc > 0x1fffff)
						l = 5;
				}
			}
			else if ((*str & 0xfe) == 0xfc)
			{
				//six bytes
				if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
				{
					uc = ((str[0] & 0x01)<<30) | ((str[1] & 0x3f)<<24) | ((str[2] & 0x3f)<<18) | ((str[3] & 0x3f)<<12) | ((str[4] & 0x3f)<<6) | ((str[5] & 0x3f)<<0);
					if (uc > 0x3ffffff)
						l = 6;
				}
			}
			//0xfe and 0xff, while plausable leading bytes, are not permitted.

			if (l)
			{
				//note that we don't support utf-16 surrogates
				if (uc == 0xd800 || uc == 0xdb7f || uc == 0xdb80 || uc == 0xdbff || uc == 0xdc00 || uc == 0xdf80 || uc == 0xdfff)
					l = 0;
				//these are meant to be illegal too
				else if (uc == 0xfffe || uc == 0xffff)
					uc = '?';
				//too big for our data types
				else if (uc & ~CON_CHARMASK)
					l = 0;
			}
			else
				utf8 &= ~1;

			//l is set if we got a valid utf-8 byte sequence
			if (l)
			{
				if (!--outsize)
					break;
				*out++ = uc | ext;
				str += l;
				continue;
			}

			//malformed encoding we just drop through
			//if its just a malformed or overlong string, we end up with a chunk of 'red' chars.
		}
		if (*str == '^')
		{
			if (str[1] >= '0' && str[1] <= '9')
			{
				ext = q3codemasks[str[1]-'0'] | (ext&~CON_Q3MASK); //change colour only.
			}
			else if (str[1] == '&') // extended code
			{
				if (isextendedcode(str[2]) && isextendedcode(str[3]))
				{
					// foreground char
					if (str[2] == '-') // default for FG
						ext = (COLOR_WHITE << CON_FGSHIFT) | (ext&~CON_FGMASK);
					else if (str[2] >= 'A')
						ext = ((str[2] - ('A' - 10)) << CON_FGSHIFT) | (ext&~CON_FGMASK);
					else
						ext = ((str[2] - '0') << CON_FGSHIFT) | (ext&~CON_FGMASK);
					// background char
					if (str[3] == '-') // default (clear) for BG
						ext &= ~CON_BGMASK & ~CON_NONCLEARBG;
					else if (str[3] >= 'A')
						ext = ((str[3] - ('A' - 10)) << CON_BGSHIFT) | (ext&~CON_BGMASK) | CON_NONCLEARBG;
					else
						ext = ((str[3] - '0') << CON_BGSHIFT) | (ext&~CON_BGMASK) | CON_NONCLEARBG;

					if (!keepmarkup)
					{
						str += 4;
						continue;
					}
				}
				// else invalid code
				goto messedup;
			}
			else if (str[1] == 'a')
			{
				ext ^= CON_2NDCHARSETTEXT;
			}
			else if (str[1] == 'b')
			{
				ext ^= CON_BLINKTEXT;
			}
			else if (str[1] == 'd')
			{
				ext = defaultflags;
			}
			else if (str[1] == 'm')
			{
				if (com_parseutf8.ival)
				{
					if ((ext & CON_FGMASK) != (COLOR_MAGENTA<<CON_FGSHIFT))
						ext = (ext&~CON_FGMASK) | (COLOR_MAGENTA<<CON_FGSHIFT);
					else
						ext = (ext&~CON_FGMASK) | (CON_WHITEMASK);
				}
				else
					ext ^= CON_HIGHCHARSMASK;
			}
			else if (str[1] == 'h')
			{
				ext ^= CON_HALFALPHA;
			}
			else if (str[1] == 's')	//store on stack (it's great for names)
			{
				if (extstackdepth < sizeof(extstack)/sizeof(extstack[0]))
				{
					extstack[extstackdepth] = ext;
					extstackdepth++;
				}
			}
			else if (str[1] == 'r')	//restore from stack (it's great for names)
			{
				if (extstackdepth)
				{
					extstackdepth--;
					ext = extstack[extstackdepth];
				}
			}
			else if (str[1] == 'U')	//unicode (16bit) char ^Uxxxx
			{
				if (!keepmarkup)
				{
					uc = 0;
					uc |= dehex(str[2])<<12;
					uc |= dehex(str[3])<<8;
					uc |= dehex(str[4])<<4;
					uc |= dehex(str[5])<<0;

					if (!--outsize)
						break;
					*out++ = uc | (ext&~CON_HIGHCHARSMASK);
					str += 6;

					continue;
				}
			}
			else if (str[1] == '^')
			{
				if (keepmarkup)
				{
					if (!--outsize)
						break;
					if (com_parseutf8.ival)
						*out++ = (unsigned char)(*str) | ext;
					else
						*out++ = (unsigned char)(*str) | ext | 0xe000;
				}
				str++;

				if (*str)
					goto messedup;
				continue;
			}
			else
			{
				goto messedup;
			}

			if (!keepmarkup)
			{
				str+=2;
				continue;
			}
		}
		if (*str == '&' && str[1] == 'c')
		{
			// ezQuake color codes

			if (ishexcode(str[2]) && ishexcode(str[3]) && ishexcode(str[4]))
			{
				//we don't support the full 12bit colour depth (only 4-bit CGA)
				//so find the closest that we do support
				int best = 1;
				float bd = 255*255*255, d;
				int c;
				float r, g, b;
				if      (str[2] >= '0' && str[2] <= '9')
					r = (str[2]-'0') / (float)0xf;
				else if (str[2] >= 'A' && str[2] <= 'F')
					r = (str[2]-'A'+10) / (float)0xf;
				else
					r = (str[2]-'a'+10) / (float)0xf;
				if      (str[3] >= '0' && str[3] <= '9')
					g = (str[3]-'0') / (float)0xf;
				else if (str[3] >= 'A' && str[3] <= 'F')
					g = (str[3]-'A'+10) / (float)0xf;
				else
					g = (str[3]-'a'+10) / (float)0xf;
				if      (str[4] >= '0' && str[4] <= '9')
					b = (str[4]-'0') / (float)0xf;
				else if (str[4] >= 'A' && str[4] <= 'F')
					b = (str[4]-'A'+10) / (float)0xf;
				else
					b = (str[4]-'a'+10) / (float)0xf;

				for (c = 0; c < sizeof(consolecolours)/sizeof(consolecolours[0]); c++)
				{
					d = (consolecolours[c].fr-r)*(consolecolours[c].fr-r) +
						(consolecolours[c].fg-g)*(consolecolours[c].fg-g) +
						(consolecolours[c].fb-b)*(consolecolours[c].fb-b);
					if (d < bd)
					{
						best = c;
						bd = d;
					}
				}
				ext = (best << CON_FGSHIFT) | (ext&~CON_FGMASK);

				if (!keepmarkup)
				{
					str += 5;
					continue;
				}
			}
		}
messedup:
		if (!--outsize)
			break;
		if (utf8)
			*out++ = (unsigned char)(*str++) | ext;
		else
		{
			if (strchr("\n\r\t ", *str))
				*out++ = (unsigned char)(*str++) | (ext&~CON_HIGHCHARSMASK);
			else
				*out++ = (unsigned char)(*str++) | ext | 0xe000;
		}
	}
	*out = 0;
	return out;
}

int COM_FunStringLength(unsigned char *str)
{
	//FIXME:
	int len = 0;

	while(*str)
	{
		//fixme: utf8
		if (*str == '^')
		{
			str++;
			if (*str >= '0' && *str <= '9')
				str++;	//invisible
			else if (*str == '&') // extended code
			{
				if (isextendedcode(str[1]) && isextendedcode(str[2]))
				{
					str++;// foreground char
					str++; // background char
					str++;
					continue;
				}
				// else invalid code
				goto messedup;
			}
			else if (*str == 'a' || *str == 'b' || *str == 'h' || *str == 's' || *str == 'r')
				str++;	//invisible
			else if (*str == '^')
			{
				len++;	//double-code single-output
				str++;
			}
			else
			{
				len++;	//not recognised
				len++;
				str++;
			}
			continue;
		}
		if (*str == '&' && str[1] == 'c' && ishexcode(str[2]) && ishexcode(str[3]) && ishexcode(str[4]))
		{
			//ezquake colour codes
			str += 5;
			continue;
		}
messedup:
		len++;
		str++;
	}

	return len;
}


//============================================================================

#define TOKENSIZE sizeof(com_token)
char		com_token[TOKENSIZE];
int		com_argc;
const char	**com_argv;

com_tokentype_t com_tokentype;


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
#ifndef COM_Parse
char *COM_Parse (const char *data)
{
	int		c;
	int		len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= TOKENSIZE-1)
				return (char*)data;

			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return (char*)data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse a regular word
	do
	{
		if (len >= TOKENSIZE-1)
			return (char*)data;

		com_token[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	com_token[len] = 0;
	return (char*)data;
}
#endif

char *COM_ParseStringSet (const char *data)
{
	int	c;
	int	len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace and semicolons
	while ( (c = *data) <= ' ' || c == ';' )
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// parse a regular word
	do
	{
		if (len >= TOKENSIZE-1)
		{
			com_token[len] = 0;
			return (char*)data;
		}

		com_token[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32 && c != ';');

	com_token[len] = 0;
	return (char*)data;
}


char *COM_ParseOut (const char *data, char *out, int outlen)
{
	int		c;
	int		len;

	len = 0;
	out[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}

//skip / * comments
	if (c == '/' && data[1] == '*')
	{
		data+=2;
		while(*data)
		{
			if (*data == '*' && data[1] == '/')
			{
				data+=2;
				goto skipwhite;
			}
			data++;
		}
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= outlen-1)
			{
				out[len] = 0;
				return (char*)data;
			}

			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return (char*)data;
			}
			out[len] = c;
			len++;
		}
	}

// parse a regular word
	do
	{
		if (len >= outlen-1)
		{
			out[len] = 0;
			return (char*)data;
		}

		out[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	out[len] = 0;
	return (char*)data;
}

//same as COM_Parse, but parses two quotes next to each other as a single quote as part of the string
char *COM_StringParse (const char *data, char *token, unsigned int tokenlen, qboolean expandmacros, qboolean qctokenize)
{
	int		c;
	int		len;
	char *s;

	len = 0;
	token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data), (unsigned)c <= ' ' && c != '\n')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}
	if (c == '\n')
	{
		token[len++] = c;
		token[len] = 0;
		return (char*)data+1;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}

//skip / * comments
	if (c == '/' && data[1] == '*' && !qctokenize)
	{
		data+=2;
		while(*data)
		{
			if (*data == '*' && data[1] == '/')
			{
				data+=2;
				goto skipwhite;
			}
			data++;
		}
		goto skipwhite;
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= tokenlen-1)
			{
				token[len] = '\0';
				return (char*)data;
			}


			c = *data++;
			if (c=='\"')
			{
				c = *(data);
				if (c!='\"')
				{
					token[len] = 0;
					return (char*)data;
				}
				while (c=='\"')
				{
					token[len] = c;
					len++;
					data++;
					c = *(data+1);
				}
			}
			if (!c)
			{
				token[len] = 0;
				return (char*)data-1;
			}
			token[len] = c;
			len++;
		}
	}

	// handle quoted strings specially
	if (c == '\'' && qctokenize)
	{
		data++;
		while (1)
		{
			if (len >= tokenlen-1)
			{
				token[len] = '\0';
				return (char*)data;
			}


			c = *data++;
			if (c=='\'')
			{
				c = *(data);
				if (c!='\'')
				{
					token[len] = 0;
					return (char*)data;
				}
				while (c=='\'')
				{
					token[len] = c;
					len++;
					data++;
					c = *(data+1);
				}
			}
			if (!c)
			{
				token[len] = 0;
				return (char*)data;
			}
			token[len] = c;
			len++;
		}
	}

	if (qctokenize && (c == '\n' || c == '{' || c == '}' || c == ')' || c == '(' || c == ']' || c == '[' || c == '\'' || c == ':' || c == ',' || c == ';'))
	{
		// single character
		token[len++] = c;
		token[len] = 0;
		return (char*)data+1;
	}

// parse a regular word
	do
	{
		if (len >= tokenlen-1)
		{
			token[len] = '\0';
			return (char*)data;
		}

		token[len] = c;
		data++;
		len++;
		c = *data;
	} while ((unsigned)c>32 && !(qctokenize && (c == '\n' || c == '{' || c == '}' || c == ')' || c == '(' || c == ']' || c == '[' || c == '\'' || c == ':' || c == ',' || c == ';')));

	token[len] = 0;

	if (!expandmacros)
		return (char*)data;

	//now we check for macros.
	for (s = token, c= 0; c < len; c++, s++)	//this isn't a quoted token by the way.
	{
		if (*s == '$')
		{
			cvar_t *macro;
			char name[64];
			int i;

			for (i = 1; i < sizeof(name); i++)
			{
				if (((unsigned char*)s)[i] <= ' ' || s[i] == '$')
					break;
			}

			Q_strncpyz(name, s+1, i);
			i-=1;

			macro = Cvar_FindVar(name);
			if (macro)	//got one...
			{
				if (len+strlen(macro->string)-(i+1) >= tokenlen-1)	//give up.
				{
					token[len] = '\0';
					return (char*)data;
				}
				memmove(s+strlen(macro->string), s+i+1, len-c-i);
				memcpy(s, macro->string, strlen(macro->string));
				s+=strlen(macro->string);
				len+=strlen(macro->string)-(i+1);
			}
		}
	}

	return (char*)data;
}

#define DEFAULT_PUNCTUATION "(,{})(\':;=!><&|+"
char *COM_ParseToken (const char *data, const char *punctuation)
{
	int		c;
	int		len;

	if (!punctuation)
		punctuation = DEFAULT_PUNCTUATION;

	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		com_tokentype = TTP_UNKNOWN;
		return NULL;
	}

// skip whitespace
skipwhite:
	while ( (c = *(unsigned char*)data) <= ' ' && c != '\r' && c != '\n')
	{
		if (c == 0)
		{
			com_tokentype = TTP_UNKNOWN;
			return NULL;			// end of file;
		}
		data++;
	}

	//if windows, ignore the \r.
	if (c == '\r' && data[1] == '\n')
		c = *(unsigned char*)data++;

	if (c == '\r' || c == '\n')
	{
		com_tokentype = TTP_LINEENDING;
		com_token[0] = '\n';
		com_token[1] = '\0';
		data++;
		return (char*)data;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
		else if (data[1] == '*')
		{
			data+=2;
			while (*data && (*data != '*' || data[1] != '/'))
				data++;
			if (*data)
				data++;
			if (*data)
				data++;
			goto skipwhite;
		}
	}

// handle quoted strings specially
	if (c == '\"')
	{
		com_tokentype = TTP_STRING;
		data++;
		while (1)
		{
			if (len >= TOKENSIZE-1)
			{
				com_token[len] = '\0';
				return (char*)data;
			}
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return (char*)data;
			}
			com_token[len] = c;
			len++;
		}
	}

	com_tokentype = TTP_UNKNOWN;

// parse single characters
	if (strchr(punctuation, c))
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return (char*)(data+1);
	}

// parse a regular word
	do
	{
		if (len >= TOKENSIZE-1)
			break;
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (strchr(punctuation, c))
			break;
	} while (c>32);

	com_token[len] = 0;
	return (char*)data;
}

char *COM_ParseCString (const char *data)
{
	int		c;
	int		len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= TOKENSIZE-2)
			{
				com_token[len] = '\0';
				return (char*)data;
			}

			c = *data++;
			if (!c)
			{
				com_token[len] = 0;
				return (char*)data;
			}
			if (c == '\\')
			{
				c = *data++;
				switch(c)
				{
				case 'n':
					c = '\n';
					break;
				case '\\':
					c = '\\';
					break;
				case '"':
					c = '"';
					com_token[len] = c;
					len++;
					continue;
				default:
					com_token[len] = 0;
					return (char*)data;
				}
			}
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return (char*)data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse a regular word
	do
	{
		if (len >= sizeof(com_token)-1)
			break;
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	com_token[len] = 0;
	return (char*)data;
}


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/

int COM_CheckNextParm (const char *parm, int last)
{
	int i = last+1;

	for ( ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP sometimes clears appkit vars.
		if (!Q_strcmp (parm,com_argv[i]))
			return i;
	}

	return 0;
}

int COM_CheckParm (const char *parm)
{
	return COM_CheckNextParm(parm, 0);
}

/*
===============
COM_ParsePlusSets

Looks for +set blah blah on the commandline, and creates cvars so that engine
functions may use the cvar before anything's loaded.
This isn't really needed, but might make some thing nicer.
===============
*/
void COM_ParsePlusSets (void)
{
	int i;
	for (i=1 ; i<com_argc-2 ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP sometimes clears appkit vars.
		if (!com_argv[i+1])
			continue;
		if (!com_argv[i+2])
			continue;

		if (*com_argv[i+1] == '-' || *com_argv[i+1] == '+')
			continue;	//erm
		if (*com_argv[i+2] == '-' || *com_argv[i+2] == '+')
			continue;	//erm

		if (!strcmp(com_argv[i], "+set"))
			Cvar_Get(com_argv[i+1], com_argv[i+2], 0, "Cvars set on commandline");
		else if (!strcmp(com_argv[i], "+seta"))
			Cvar_Get(com_argv[i+1], com_argv[i+2], CVAR_ARCHIVE, "Cvars set on commandline");
		i+=2;
	}
}

/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void COM_CheckRegistered (void)
{
	vfsfile_t	*h;
	unsigned short	check[128];
	int			i;

	h = FS_OpenVFS("gfx/pop.lmp", "rb", FS_GAME);
	static_registered = false;

	if (!h)
	{
		Con_TPrintf (TL_SHAREWAREVERSION);
#if 0//ndef SERVERONLY
// FIXME DEBUG -- only temporary
		if (com_modified)
			Sys_Error ("You must have the registered version to play QuakeWorld");
#endif
		return;
	}

	VFS_READ(h, check, sizeof(check));
	VFS_CLOSE(h);

	for (i=0 ; i<128 ; i++)
		if (pop[i] != (unsigned short)BigShort (check[i]))
		{
			Con_TPrintf (TL_SHAREWAREVERSION);
			return;
		}

	static_registered = true;
	Con_TPrintf (TL_REGISTEREDVERSION);
}



/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, const char **argv)	//not allowed to tprint
{
	qboolean	safe;
	int			i;
	size_t result;

	FILE *f;

	f = fopen(va("%s_p.txt", argv[0]), "rb");
	if (f)
	{
		char *buffer;
		int len;
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);

		buffer = (char*)malloc(len+1);
		result = fread(buffer, 1, len, f); // do something with result

		if (result != len)
			Con_Printf("COM_InitArgv() fread: Filename: %s, expected %i, result was %u (%s)\n",va("%s_p.txt", argv[0]),len,(unsigned int)result,strerror(errno));

		buffer[len] = '\0';

		while (*buffer && (argc < MAX_NUM_ARGVS))
		{
			while (*buffer && ((*buffer <= 32) || (*buffer > 126)))
				buffer++;

			if (*buffer)
			{
				argv[argc] = buffer;
				argc++;

				while (*buffer && ((*buffer > 32) && (*buffer <= 126)))
					buffer++;

				if (*buffer)
				{
					*buffer = 0;
					buffer++;
				}

			}
		}


		fclose(f);
	}

	safe = false;

	for (com_argc=0 ; (com_argc<MAX_NUM_ARGVS) && (com_argc < argc) ;
		 com_argc++)
	{
		largv[com_argc] = argv[com_argc];
		if (!Q_strcmp ("-safe", argv[com_argc]))
			safe = true;
	}

	if (safe)
	{
	// force all the safe-mode switches. Note that we reserved extra space in
	// case we need to add these, so we don't need an overflow check
		for (i=0 ; i<NUM_SAFE_ARGVS ; i++)
		{
			largv[com_argc] = safeargvs[i];
			com_argc++;
		}
	}

	largv[com_argc] = argvdummy;
	com_argv = largv;
}

/*
================
COM_AddParm

Adds the given string at the end of the current argument list
================
*/
void COM_AddParm (const char *parm)
{
	largv[com_argc++] = parm;
}

/*
=======================
COM_Version_f
======================
*/
void COM_Version_f (void)
{
	Con_Printf("%s\n", version_string());

	Con_TPrintf (TL_EXEDATETIME, __DATE__, __TIME__);

#ifdef SVNREVISION
	#define STRINGIFY2(arg) #arg
	#define STRINGIFY(arg) STRINGIFY2(arg)
	Con_Printf("SVN Revision: %s\n",STRINGIFY(SVNREVISION));
#endif

#ifdef _DEBUG
	Con_Printf("debug build\n");
#endif
#ifdef MINIMAL
	Con_Printf("minimal build\n");
#endif
#ifdef CLIENTONLY
	Con_Printf("client-only build\n");
#endif
#ifdef SERVERONLY
	Con_Printf("dedicated server build\n");
#endif

#ifdef GLQUAKE
	Con_Printf("OpenGL available\n");
#endif
#ifdef D3DQUAKE
	Con_Printf("Direct3D available\n");
#endif

#ifdef _SDL
	Con_Printf("SDL version: %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
#endif

// Don't print both as a 64bit MinGW built client
#if defined(__MINGW32__)
	Con_Printf("Compiled with MinGW32/64 version: %i.%i\n",__MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
#endif

#ifdef __CYGWIN__
	Con_Printf("Compiled with Cygwin\n");
#endif

#ifdef __GNUC__
	Con_Printf("Compiled with GCC version: %i.%i.%i (%s)\n",__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__, __VERSION__);

	#ifdef __OPTIMIZE__
		#ifdef __OPTIMIZE_SIZE__
			Con_Printf("Optimized for size\n");
		#else
			Con_Printf("Optimized for speed\n");
		#endif
	#endif

	#ifdef __NO_INLINE__
		Con_Printf("GCC Optimization: Functions currently not inlined into their callers\n");
	#else
		Con_Printf("GCC Optimization: Functions currently inlined into their callers\n");
	#endif
#endif

#ifdef _WIN64
		Con_Printf("Compiled for 64bit windows\n");
#endif
#if defined(_M_AMD64) || defined(__amd64__)
		Con_Printf("Compiled for AMD64 compatible cpus\n");
#endif

#ifdef _M_IX86
	Con_Printf("x86 optimized for: ");

	if (_M_IX86 == 600) { Con_Printf("Blend or Pentium Pro, Pentium II and Pentium III"); }
	else if (_M_IX86 == 500) { Con_Printf("Pentium"); }
	else if (_M_IX86 == 400) { Con_Printf("486"); }
	else if (_M_IX86 == 300) { Con_Printf("386"); }
	else
	{
		Con_Printf("Unknown (%i)\n",_M_IX86);
	}

	Con_Printf("\n");
#endif

#ifdef _M_IX86_FP
	if (_M_IX86_FP == 0) { Con_Printf("SSE & SSE2 instructions disabled\n"); }
	else if (_M_IX86_FP == 1) { Con_Printf("SSE instructions enabled\n"); }
	else if (_M_IX86_FP == 2) { Con_Printf("SSE2 instructions enabled\n"); }
	else
	{
		Con_Printf("Unknown Arch specified: %i\n",_M_IX86_FP);
	}
#endif

#ifdef _MSC_VER
	if (_MSC_VER == 600) { Con_Printf("C Compiler version 6.0\n"); }
	else if (_MSC_VER == 700) { Con_Printf("C/C++ compiler version 7.0\n"); }
	else if (_MSC_VER == 800) { Con_Printf("Visual C++, Windows, version 1.0 or Visual C++, 32-bit, version 1.0\n"); }
	else if (_MSC_VER == 900) { Con_Printf("Visual C++, Windows, version 2.0 or Visual C++, 32-bit, version 2.x\n"); }
	else if (_MSC_VER == 1000) { Con_Printf("Visual C++, 32-bit, version 4.0\n"); }
	else if (_MSC_VER == 1020) { Con_Printf("Visual C++, 32-bit, version 4.2\n"); }
	else if (_MSC_VER == 1100) { Con_Printf("Visual C++, 32-bit, version 5.0\n"); }
	else if (_MSC_VER == 1200) { Con_Printf("Visual C++, 32-bit, version 6.0\n"); }
	else if (_MSC_VER == 1300) { Con_Printf("Visual C++, version 7.0\n"); }
	else if (_MSC_VER == 1310) { Con_Printf("Visual C++ 2003, version 7.1\n"); }
	else if (_MSC_VER == 1400) { Con_Printf("Visual C++ 2005, version 8.0\n"); }
	else if (_MSC_VER == 1500) { Con_Printf("Visual C++ 2008, version 9.0\n"); }
	else if (_MSC_VER == 1600) { Con_Printf("Visual C++ 2010, version 10.0\n"); }
	else
	{
#ifdef _MSC_BUILD
		Con_Printf("Unknown Microsoft C++ compiler: %i %i %i\n",_MSC_VER, _MSC_FULL_VER, _MSC_BUILD);
#else
		Con_Printf("Unknown Microsoft C++ compiler: %i %i\n",_MSC_VER, _MSC_FULL_VER);
#endif
	}
#endif

	//print out which libraries are disabled
#ifndef AVAIL_ZLIB
	Con_Printf("zlib disabled\n");
#else
	Con_Printf("zlib: %s\n", ZLIB_VERSION);
#endif



	//but print client ones only if we're not dedicated
#ifndef SERVERONLY
#ifndef AVAIL_PNGLIB
	Con_Printf("libpng disabled\n");
#else
	Con_Printf("libPNG %s -%s", PNG_LIBPNG_VER_STRING, PNG_HEADER_VERSION_STRING);
#endif
#ifndef AVAIL_JPEGLIB
	Con_Printf("libjpeg disabled\n");
#else
	Con_Printf("libjpeg: %i (%d series)\n", JPEG_LIB_VERSION, ( JPEG_LIB_VERSION / 10 ) );
#endif
#ifndef AVAIL_OGGVORBIS
	Con_Printf("libvorbis disabled\n");
#endif
#ifndef AVAIL_FREETYPE
	Con_Printf("freetype2 disabled\n");
#endif
#ifndef AVAIL_OPENAL
	Con_Printf("openal disabled\n");
#endif

#ifdef _WIN32
	#ifndef AVAIL_DINPUT
		Con_Printf("DirectInput disabled\n");
	#endif
	#ifndef AVAIL_DSOUND
		Con_Printf("DirectSound disabled\n");
	#endif
	#ifndef AVAIL_D3D
		Con_Printf("Direct3D disabled\n");
	#endif
	#ifndef AVAIL_DDRAW
		Con_Printf("DirectDraw disabled\n");
	#endif
#endif
#endif
}

/*
================
COM_Init
================
*/
void COM_Init (void)
{
	qbyte	swaptest[2] = {1,0};

// set the qbyte swapping variables in a portable manner
	if ( *(short *)swaptest == 1)
	{
		bigendian = false;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendian = true;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}

	Cmd_AddCommand ("path", COM_Path_f);		//prints a list of current search paths.
	Cmd_AddCommand ("dir", COM_Dir_f);			//q3 like
	Cmd_AddCommand ("flocate", COM_Locate_f);	//prints the pak or whatever where this file can be found.
	Cmd_AddCommand ("version", COM_Version_f);	//prints the pak or whatever where this file can be found.
	
	Cmd_AddCommand ("crashme", (void*)1); //debugging feature, makes it jump to an invalid address

	COM_InitFilesystem ();

	COM_CheckRegistered ();
	if (static_registered)
		registered.string = "1";
	else
		registered.string = "0";

	Cvar_Register (&registered, "Copy protection");
	Cvar_Register (&gameversion, "Gamecode");
	Cvar_Register (&gameversion_min, "Gamecode");
	Cvar_Register (&gameversion_max, "Gamecode");
	Cvar_Register (&com_parseutf8, "Internationalisation");
	com_parseutf8.ival = 1;




	nullentitystate.colormod[0] = 32;
	nullentitystate.colormod[1] = 32;
	nullentitystate.colormod[2] = 32;
	nullentitystate.trans = 255;
	nullentitystate.scale = 16;
	nullentitystate.abslight = 255;
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char	*VARGS va(char *format, ...)
{
#define VA_BUFFERS 2 //power of two
	va_list		argptr;
	static char		string[VA_BUFFERS][1024];
	static int bufnum;

	bufnum++;
	bufnum &= (VA_BUFFERS-1);

	va_start (argptr, format);
	vsnprintf (string[bufnum],sizeof(string[bufnum])-1, format,argptr);
	va_end (argptr);

	return string[bufnum];
}


/// just for debugging
int	memsearch (qbyte *start, int count, int search)
{
	int		i;

	for (i=0 ; i<count ; i++)
		if (start[i] == search)
			return i;
	return -1;
}

struct effectinfo_s
{
	struct effectinfo_s *next;
	int index;

	char name[1];
};
static struct effectinfo_s *effectinfo;
static unsigned int lasteffectinfoid;

void COM_Effectinfo_Clear(void)
{
	struct effectinfo_s *n;
	while(effectinfo)
	{
		n = effectinfo->next;
		Z_Free(effectinfo);
		effectinfo = n;
	}
	lasteffectinfoid = 0;
}

int COM_Effectinfo_Add(const char *effectname)
{
	struct effectinfo_s *n;
	for (n = effectinfo; n; n = n->next)
	{
		if (!strcmp(effectname, n->name))
			return 0;	//already known
	}


	n = Z_Malloc(sizeof(*n) + strlen(effectname));
	n->next = effectinfo;
	n->index = ++lasteffectinfoid;
	effectinfo = n;
	strcpy(n->name, effectname);

	return n->index;
}

void COM_Effectinfo_Reload(void)
{
	int i;
	char *f, *buf;
	static const char *dpnames[] =
	{
		"TE_GUNSHOT",
		"TE_GUNSHOTQUAD",
		"TE_SPIKE",
		"TE_SPIKEQUAD",
		"TE_SUPERSPIKE",
		"TE_SUPERSPIKEQUAD",
		"TE_WIZSPIKE",
		"TE_KNIGHTSPIKE",
		"TE_EXPLOSION",
		"TE_EXPLOSIONQUAD",
		"TE_TAREXPLOSION",
		"TE_TELEPORT",
		"TE_LAVASPLASH",
		"TE_SMALLFLASH",
		"TE_FLAMEJET",
		"EF_FLAME",
		"TE_BLOOD",
		"TE_SPARK",
		"TE_PLASMABURN",
		"TE_TEI_G3",
		"TE_TEI_SMOKE",
		"TE_TEI_BIGEXPLOSION",
		"TE_TEI_PLASMAHIT",
		"EF_STARDUST",
		"TR_ROCKET",
		"TR_GRENADE",
		"TR_BLOOD",
		"TR_WIZSPIKE",
		"TR_SLIGHTBLOOD",
		"TR_KNIGHTSPIKE",
		"TR_VORESPIKE",
		"TR_NEHAHRASMOKE",
		"TR_NEXUIZPLASMA",
		"TR_GLOWTRAIL",
		"SVC_PARTICLE",
		NULL
	};

	COM_Effectinfo_Clear();

	for (i = 0; dpnames[i]; i++)
		COM_Effectinfo_Add(dpnames[i]);


	FS_LoadFile("effectinfo.txt", (void **)&f);
	if (!f)
		return;
	buf = f;
	while (*f)
	{
		f = COM_ParseToken(f, NULL);
		if (strcmp(com_token, "\n"))
		{
			if (!strcmp(com_token, "effect"))
			{
				f = COM_ParseToken(f, NULL);

				COM_Effectinfo_Add(com_token);
			}

			do
			{
				f = COM_ParseToken(f, NULL);
			} while(*f && strcmp(com_token, "\n"));
		}
	}
	FS_FreeFile(buf);
}

unsigned int COM_Effectinfo_ForName(const char *efname)
{
	struct effectinfo_s *e;

	if (!effectinfo)
		COM_Effectinfo_Reload();

	for (e = effectinfo; e; e = e->next)
	{
		if (!strcmp(efname, e->name))
			return e->index;
	}
	return COM_Effectinfo_Add(efname);
}

char *COM_Effectinfo_ForNumber(unsigned int efnum)
{
	struct effectinfo_s *e;

	if (!effectinfo)
		COM_Effectinfo_Reload();

	for (e = effectinfo; e; e = e->next)
	{
		if (e->index == efnum)
			return e->name;
	}
	return "";
}

/*************************************************************************/

/*remaps map checksums from known non-cheat GPL maps to authentic id1 maps*/
unsigned int COM_RemapMapChecksum(unsigned int checksum)
{
	static const struct {
		char *name;
		unsigned int gpl2;
		unsigned int id11;
		unsigned int id12;
	} sums[] =
	{
		{"maps/start.bsp", -603735309, 714749795, 493454459},

		{"maps/e1m1.bsp", -1213097692, 523840258, -1391994750},
		{"maps/e1m2.bsp", -2134038629, 1561595172, 1729102119},
		{"maps/e1m3.bsp", 526593427, 1008794158, 893792842},
		{"maps/e1m4.bsp", -1218723400, -442162482, -304478603},
		{"maps/e1m5.bsp", 1709090059, 1856217547, -1473504118},
		{"maps/e1m6.bsp", 1014375998, 1304756164, 738207971},
		{"maps/e1m7.bsp", 1375393448, -1396746908, -1747518694},
		{"maps/e1m8.bsp", 1470379688, -163803419, 79095617},

		{"maps/e2m1.bsp", -1725230579, -797758554, -587894734},
		{"maps/e2m2.bsp", -1573837115, -355822557, -1349116595},
		{"maps/e2m3.bsp", 156655662, 1203005272, -57072303},
		{"maps/e2m4.bsp", -1530012474, -1629664024, -1021928503},
		{"maps/e3m5.bsp", -594001393, -1405673977, -1854273999},
		{"maps/e2m6.bsp", 1041933133, 583875451, -1851573375},
		{"maps/e2m7.bsp", -1583122652, 1814005234, 2051006488},

		{"maps/e3m1.bsp", -1118143869, -457270773, -1867379423},
		{"maps/e3m2.bsp", -469484146, 723435606, -1670613704},
		{"maps/e3m3.bsp", -300762423, -540030088, -1009754856},
		{"maps/e3m4.bsp", -214067894, 1107310161, -1317466952},
		{"maps/e3m5.bsp", -594001393, -1405673977, -1854273999},
		{"maps/e3m6.bsp", -1664550468, 1631142730, 767655416},
		{"maps/e3m7.bsp", 781051658, -1513131760, 272220593},

		{"maps/e4m1.bsp", 1548541253, 1254243660, -1141873840},
		{"maps/e4m2.bsp", -1400585206, 92253388, -472296},
		{"maps/e4m3.bsp", -1230693918, 1961442781, 1505685644},
		{"maps/e4m4.bsp", 842253404, -374904516, 758847551},
		{"maps/e4m5.bsp", -439098147, 389110272, 1771890676},
		{"maps/e4m6.bsp", 1518024640, 1714857656, 102825880},
		{"maps/e4m7.bsp", -381063035, -585362206, -1645477460},
		{"maps/e4m8.bsp", 844770132, 1063417045, 1018457175},

		{"maps/gpl_dm1.bsp", 2100781454, -1548219590, -976758093},
		{"maps/gpl_dm2.bsp", 2066969664, 392410074, 1710634548},
		{"maps/gpl_dm3.bsp", -1859681874, 2060033246, 367136248},
		{"maps/gpl_dm4.bsp", -1015750775, 326737183, -1670388545},
		{"maps/gpl_dm5.bsp", 2009758949, 766929852, -1339209475},
		{"maps/gpl_dm6.bsp", 537693021, 247150701, 1376311851},

		{"maps/end.bsp", -124054866, -1503553320, -1143688027}
	};
	unsigned int i;
	for (i = 0; i < sizeof(sums)/sizeof(sums[0]); i++)
	{
		if (checksum == sums[i].gpl2)
			return sums[i].id12;
	}
	return checksum;
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
char *Info_ValueForKey (char *s, const char *key)
{
	char	pkey[1024];
	static	char value[4][1024];	// use two buffers so compares
								// work without stomping on each other
	static	int	valueindex;
	char	*o;

	valueindex = (valueindex + 1) % 4;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				*value[valueindex]='\0';
				return value[valueindex];
			}
			*o++ = *s++;
			if (o+2 >= pkey+sizeof(pkey))	//hrm. hackers at work..
			{
				*value[valueindex]='\0';
				return value[valueindex];
			}
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				*value[valueindex]='\0';
				return value[valueindex];
			}
			*o++ = *s++;

			if (o+2 >= value[valueindex]+sizeof(value[valueindex]))	//hrm. hackers at work..
			{
				*value[valueindex]='\0';
				return value[valueindex];
			}
		}
		*o = 0;

		if (!strcmp (key, pkey) )
			return value[valueindex];

		if (!*s)
		{
			*value[valueindex]='\0';
			return value[valueindex];
		}
		s++;
	}
}

char *Info_KeyForNumber (char *s, int num)
{
	static char	pkey[1024];
	char	*o;

	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				*pkey='\0';
				return pkey;
			}
			*o++ = *s++;
			if (o+2 >= pkey+sizeof(pkey))	//hrm. hackers at work..
			{
				*pkey='\0';
				return pkey;
			}
		}
		*o = 0;
		s++;

		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				*pkey='\0';
				return pkey;
			}
			s++;
		}

		if (!num--)
			return pkey;	//found the right one

		if (!*s)
		{
			*pkey='\0';
			return pkey;
		}
		s++;
	}
}

void Info_RemoveKey (char *s, const char *key)
{
	char	*start;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	if (strstr (key, "\\"))
	{
		Con_TPrintf (TL_KEYHASSLASH);
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			//strip out the value by copying the next string over the top of this one
			//(we were using strcpy, but valgrind moaned)
			while(*s)
				*start++ = *s++;
			*start = 0;
			return;
		}

		if (!*s)
			return;
	}

}

void Info_RemovePrefixedKeys (char *start, char prefix)
{
	char	*s;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	s = start;

	while (1)
	{
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (pkey[0] == prefix)
		{
			Info_RemoveKey (start, pkey);
			s = start;
		}

		if (!*s)
			return;
	}

}

void Info_RemoveNonStarKeys (char *start)
{
	char	*s;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	s = start;

	while (1)
	{
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (pkey[0] != '*')
		{
			Info_RemoveKey (start, pkey);
			s = start;
		}

		if (!*s)
			return;
	}

}

void Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize)
{
	char	newv[1024], *v;
	int		c;
#ifdef SERVERONLY
	extern cvar_t sv_highchars;
#endif

	if (strstr (key, "\\") || strstr (value, "\\") )
	{
		Con_TPrintf (TL_KEYHASSLASH);
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"") )
	{
		Con_TPrintf (TL_KEYHASQUOTE);
		return;
	}

	if (strlen(key) >= MAX_INFO_KEY)// || strlen(value) >= MAX_INFO_KEY)
	{
		Con_TPrintf (TL_KEYTOOLONG);
		return;
	}

	// this next line is kinda trippy
	if (*(v = Info_ValueForKey(s, key)))
	{
		// key exists, make sure we have enough room for new value, if we don't,
		// don't change it!
		if (strlen(value) - strlen(v) + strlen(s) + 1 > maxsize)
		{
			if (*Info_ValueForKey(s, "*ver"))	//quick hack to kill off unneeded info on overflow. We can't simply increase the quantity of this stuff.
			{
				Info_RemoveKey(s, "*ver");
				Info_SetValueForStarKey (s, key, value, maxsize);
				return;
			}
			Con_TPrintf (TL_INFOSTRINGTOOLONG);
			return;
		}
	}
	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	snprintf (newv, sizeof(newv), "\\%s\\%s", key, value);

	if ((int)(strlen(newv) + strlen(s) + 1) > maxsize)
	{
		Con_TPrintf (TL_INFOSTRINGTOOLONG);
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newv;
	while (*v)
	{
		c = (unsigned char)*v++;
#ifndef SERVERONLY
		// client only allows highbits on name
//		if (stricmp(key, "name") != 0) {
//			c &= 127;
//			if (c < 32 || c > 127)
//				continue;
//			// auto lowercase team
//			if (stricmp(key, "team") == 0)
//				c = tolower(c);
//		}
#else
		if (!sv_highchars.value) {
			c &= 127;
			if (c < 32 || c > 127)
				continue;
		}
#endif
//		c &= 127;		// strip high bits
		if (c > 13) // && c < 127)
			*s++ = c;
	}
	*s = 0;
}

void Info_SetValueForKey (char *s, const char *key, const char *value, int maxsize)
{
	if (key[0] == '*')
	{
		Con_TPrintf (TL_STARKEYPROTECTED);
		return;
	}

	Info_SetValueForStarKey (s, key, value, maxsize);
}

void Info_Print (char *s)
{
	char	key[1024];
	char	value[1024];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Con_Printf ("%s", key);

		if (!*s)
		{
			Con_TPrintf (TL_KEYHASNOVALUE);
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Con_Printf ("%s\n", value);
	}
}

void Info_WriteToFile(vfsfile_t *f, char *info, char *commandname, int cvarflags)
{
	char *command;
	char *value;
	cvar_t *var;

	while(*info == '\\')
	{
		command = info+1;
		value = strchr(command, '\\');
		info = strchr(value+1, '\\');
		if (!info)	//eot..
			info = value+strlen(value);

		if (*command == '*')	//unsettable, so don't write it for later setting.
			continue;

		var = Cvar_FindVar(command);
		if (var && var->flags & cvarflags)
			continue;	//this is saved via a cvar.

		VFS_WRITE(f, commandname, strlen(commandname));
		VFS_WRITE(f, " ", 1);
		VFS_WRITE(f, command, value-command);
		VFS_WRITE(f, " ", 1);
		VFS_WRITE(f, value+1, info-(value+1));
		VFS_WRITE(f, "\n", 1);
	}
}


static qbyte chktbl[1024 + 4] = {
0x78,0xd2,0x94,0xe3,0x41,0xec,0xd6,0xd5,0xcb,0xfc,0xdb,0x8a,0x4b,0xcc,0x85,0x01,
0x23,0xd2,0xe5,0xf2,0x29,0xa7,0x45,0x94,0x4a,0x62,0xe3,0xa5,0x6f,0x3f,0xe1,0x7a,
0x64,0xed,0x5c,0x99,0x29,0x87,0xa8,0x78,0x59,0x0d,0xaa,0x0f,0x25,0x0a,0x5c,0x58,
0xfb,0x00,0xa7,0xa8,0x8a,0x1d,0x86,0x80,0xc5,0x1f,0xd2,0x28,0x69,0x71,0x58,0xc3,
0x51,0x90,0xe1,0xf8,0x6a,0xf3,0x8f,0xb0,0x68,0xdf,0x95,0x40,0x5c,0xe4,0x24,0x6b,
0x29,0x19,0x71,0x3f,0x42,0x63,0x6c,0x48,0xe7,0xad,0xa8,0x4b,0x91,0x8f,0x42,0x36,
0x34,0xe7,0x32,0x55,0x59,0x2d,0x36,0x38,0x38,0x59,0x9b,0x08,0x16,0x4d,0x8d,0xf8,
0x0a,0xa4,0x52,0x01,0xbb,0x52,0xa9,0xfd,0x40,0x18,0x97,0x37,0xff,0xc9,0x82,0x27,
0xb2,0x64,0x60,0xce,0x00,0xd9,0x04,0xf0,0x9e,0x99,0xbd,0xce,0x8f,0x90,0x4a,0xdd,
0xe1,0xec,0x19,0x14,0xb1,0xfb,0xca,0x1e,0x98,0x0f,0xd4,0xcb,0x80,0xd6,0x05,0x63,
0xfd,0xa0,0x74,0xa6,0x86,0xf6,0x19,0x98,0x76,0x27,0x68,0xf7,0xe9,0x09,0x9a,0xf2,
0x2e,0x42,0xe1,0xbe,0x64,0x48,0x2a,0x74,0x30,0xbb,0x07,0xcc,0x1f,0xd4,0x91,0x9d,
0xac,0x55,0x53,0x25,0xb9,0x64,0xf7,0x58,0x4c,0x34,0x16,0xbc,0xf6,0x12,0x2b,0x65,
0x68,0x25,0x2e,0x29,0x1f,0xbb,0xb9,0xee,0x6d,0x0c,0x8e,0xbb,0xd2,0x5f,0x1d,0x8f,
0xc1,0x39,0xf9,0x8d,0xc0,0x39,0x75,0xcf,0x25,0x17,0xbe,0x96,0xaf,0x98,0x9f,0x5f,
0x65,0x15,0xc4,0x62,0xf8,0x55,0xfc,0xab,0x54,0xcf,0xdc,0x14,0x06,0xc8,0xfc,0x42,
0xd3,0xf0,0xad,0x10,0x08,0xcd,0xd4,0x11,0xbb,0xca,0x67,0xc6,0x48,0x5f,0x9d,0x59,
0xe3,0xe8,0x53,0x67,0x27,0x2d,0x34,0x9e,0x9e,0x24,0x29,0xdb,0x69,0x99,0x86,0xf9,
0x20,0xb5,0xbb,0x5b,0xb0,0xf9,0xc3,0x67,0xad,0x1c,0x9c,0xf7,0xcc,0xef,0xce,0x69,
0xe0,0x26,0x8f,0x79,0xbd,0xca,0x10,0x17,0xda,0xa9,0x88,0x57,0x9b,0x15,0x24,0xba,
0x84,0xd0,0xeb,0x4d,0x14,0xf5,0xfc,0xe6,0x51,0x6c,0x6f,0x64,0x6b,0x73,0xec,0x85,
0xf1,0x6f,0xe1,0x67,0x25,0x10,0x77,0x32,0x9e,0x85,0x6e,0x69,0xb1,0x83,0x00,0xe4,
0x13,0xa4,0x45,0x34,0x3b,0x40,0xff,0x41,0x82,0x89,0x79,0x57,0xfd,0xd2,0x8e,0xe8,
0xfc,0x1d,0x19,0x21,0x12,0x00,0xd7,0x66,0xe5,0xc7,0x10,0x1d,0xcb,0x75,0xe8,0xfa,
0xb6,0xee,0x7b,0x2f,0x1a,0x25,0x24,0xb9,0x9f,0x1d,0x78,0xfb,0x84,0xd0,0x17,0x05,
0x71,0xb3,0xc8,0x18,0xff,0x62,0xee,0xed,0x53,0xab,0x78,0xd3,0x65,0x2d,0xbb,0xc7,
0xc1,0xe7,0x70,0xa2,0x43,0x2c,0x7c,0xc7,0x16,0x04,0xd2,0x45,0xd5,0x6b,0x6c,0x7a,
0x5e,0xa1,0x50,0x2e,0x31,0x5b,0xcc,0xe8,0x65,0x8b,0x16,0x85,0xbf,0x82,0x83,0xfb,
0xde,0x9f,0x36,0x48,0x32,0x79,0xd6,0x9b,0xfb,0x52,0x45,0xbf,0x43,0xf7,0x0b,0x0b,
0x19,0x19,0x31,0xc3,0x85,0xec,0x1d,0x8c,0x20,0xf0,0x3a,0xfa,0x80,0x4d,0x2c,0x7d,
0xac,0x60,0x09,0xc0,0x40,0xee,0xb9,0xeb,0x13,0x5b,0xe8,0x2b,0xb1,0x20,0xf0,0xce,
0x4c,0xbd,0xc6,0x04,0x86,0x70,0xc6,0x33,0xc3,0x15,0x0f,0x65,0x19,0xfd,0xc2,0xd3,

// map checksum goes here
0x00,0x00,0x00,0x00
};

#if 0

static qbyte chkbuf[16 + 60 + 4];

static unsigned last_mapchecksum = 0;


/*
====================
COM_BlockSequenceCheckByte

For proxy protecting
====================
*/
qbyte	COM_BlockSequenceCheckByte (qbyte *base, int length, int sequence, unsigned mapchecksum)
{
	int		checksum;
	qbyte	*p;

	if (last_mapchecksum != mapchecksum) {
		last_mapchecksum = mapchecksum;
		chktbl[1024] = (mapchecksum & 0xff000000) >> 24;
		chktbl[1025] = (mapchecksum & 0x00ff0000) >> 16;
		chktbl[1026] = (mapchecksum & 0x0000ff00) >> 8;
		chktbl[1027] = (mapchecksum & 0x000000ff);

		Com_BlockFullChecksum (chktbl, sizeof(chktbl), chkbuf);
	}

	p = chktbl + (sequence % (sizeof(chktbl) - 8));

	if (length > 60)
		length = 60;
	memcpy (chkbuf + 16, base, length);

	length += 16;

	chkbuf[length] = (sequence & 0xff) ^ p[0];
	chkbuf[length+1] = p[1];
	chkbuf[length+2] = ((sequence>>8) & 0xff) ^ p[2];
	chkbuf[length+3] = p[3];

	length += 4;

	checksum = LittleLong(Com_BlockChecksum (chkbuf, length));

	checksum &= 0xff;

	return checksum;
}
#endif

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
qbyte	COM_BlockSequenceCRCByte (qbyte *base, int length, int sequence)
{
	unsigned short crc;
	qbyte	*p;
	qbyte chkb[60 + 4];

	p = chktbl + (sequence % (sizeof(chktbl) - 8));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = (sequence & 0xff) ^ p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = ((sequence>>8) & 0xff) ^ p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = QCRC_Block(chkb, length);

	crc &= 0xff;

	return crc;
}


#if defined(Q2CLIENT) || defined(Q2SERVER)
static qbyte q2chktbl[1024] = {
0x84, 0x47, 0x51, 0xc1, 0x93, 0x22, 0x21, 0x24, 0x2f, 0x66, 0x60, 0x4d, 0xb0, 0x7c, 0xda,
0x88, 0x54, 0x15, 0x2b, 0xc6, 0x6c, 0x89, 0xc5, 0x9d, 0x48, 0xee, 0xe6, 0x8a, 0xb5, 0xf4,
0xcb, 0xfb, 0xf1, 0x0c, 0x2e, 0xa0, 0xd7, 0xc9, 0x1f, 0xd6, 0x06, 0x9a, 0x09, 0x41, 0x54,
0x67, 0x46, 0xc7, 0x74, 0xe3, 0xc8, 0xb6, 0x5d, 0xa6, 0x36, 0xc4, 0xab, 0x2c, 0x7e, 0x85,
0xa8, 0xa4, 0xa6, 0x4d, 0x96, 0x19, 0x19, 0x9a, 0xcc, 0xd8, 0xac, 0x39, 0x5e, 0x3c, 0xf2,
0xf5, 0x5a, 0x72, 0xe5, 0xa9, 0xd1, 0xb3, 0x23, 0x82, 0x6f, 0x29, 0xcb, 0xd1, 0xcc, 0x71,
0xfb, 0xea, 0x92, 0xeb, 0x1c, 0xca, 0x4c, 0x70, 0xfe, 0x4d, 0xc9, 0x67, 0x43, 0x47, 0x94,
0xb9, 0x47, 0xbc, 0x3f, 0x01, 0xab, 0x7b, 0xa6, 0xe2, 0x76, 0xef, 0x5a, 0x7a, 0x29, 0x0b,
0x51, 0x54, 0x67, 0xd8, 0x1c, 0x14, 0x3e, 0x29, 0xec, 0xe9, 0x2d, 0x48, 0x67, 0xff, 0xed,
0x54, 0x4f, 0x48, 0xc0, 0xaa, 0x61, 0xf7, 0x78, 0x12, 0x03, 0x7a, 0x9e, 0x8b, 0xcf, 0x83,
0x7b, 0xae, 0xca, 0x7b, 0xd9, 0xe9, 0x53, 0x2a, 0xeb, 0xd2, 0xd8, 0xcd, 0xa3, 0x10, 0x25,
0x78, 0x5a, 0xb5, 0x23, 0x06, 0x93, 0xb7, 0x84, 0xd2, 0xbd, 0x96, 0x75, 0xa5, 0x5e, 0xcf,
0x4e, 0xe9, 0x50, 0xa1, 0xe6, 0x9d, 0xb1, 0xe3, 0x85, 0x66, 0x28, 0x4e, 0x43, 0xdc, 0x6e,
0xbb, 0x33, 0x9e, 0xf3, 0x0d, 0x00, 0xc1, 0xcf, 0x67, 0x34, 0x06, 0x7c, 0x71, 0xe3, 0x63,
0xb7, 0xb7, 0xdf, 0x92, 0xc4, 0xc2, 0x25, 0x5c, 0xff, 0xc3, 0x6e, 0xfc, 0xaa, 0x1e, 0x2a,
0x48, 0x11, 0x1c, 0x36, 0x68, 0x78, 0x86, 0x79, 0x30, 0xc3, 0xd6, 0xde, 0xbc, 0x3a, 0x2a,
0x6d, 0x1e, 0x46, 0xdd, 0xe0, 0x80, 0x1e, 0x44, 0x3b, 0x6f, 0xaf, 0x31, 0xda, 0xa2, 0xbd,
0x77, 0x06, 0x56, 0xc0, 0xb7, 0x92, 0x4b, 0x37, 0xc0, 0xfc, 0xc2, 0xd5, 0xfb, 0xa8, 0xda,
0xf5, 0x57, 0xa8, 0x18, 0xc0, 0xdf, 0xe7, 0xaa, 0x2a, 0xe0, 0x7c, 0x6f, 0x77, 0xb1, 0x26,
0xba, 0xf9, 0x2e, 0x1d, 0x16, 0xcb, 0xb8, 0xa2, 0x44, 0xd5, 0x2f, 0x1a, 0x79, 0x74, 0x87,
0x4b, 0x00, 0xc9, 0x4a, 0x3a, 0x65, 0x8f, 0xe6, 0x5d, 0xe5, 0x0a, 0x77, 0xd8, 0x1a, 0x14,
0x41, 0x75, 0xb1, 0xe2, 0x50, 0x2c, 0x93, 0x38, 0x2b, 0x6d, 0xf3, 0xf6, 0xdb, 0x1f, 0xcd,
0xff, 0x14, 0x70, 0xe7, 0x16, 0xe8, 0x3d, 0xf0, 0xe3, 0xbc, 0x5e, 0xb6, 0x3f, 0xcc, 0x81,
0x24, 0x67, 0xf3, 0x97, 0x3b, 0xfe, 0x3a, 0x96, 0x85, 0xdf, 0xe4, 0x6e, 0x3c, 0x85, 0x05,
0x0e, 0xa3, 0x2b, 0x07, 0xc8, 0xbf, 0xe5, 0x13, 0x82, 0x62, 0x08, 0x61, 0x69, 0x4b, 0x47,
0x62, 0x73, 0x44, 0x64, 0x8e, 0xe2, 0x91, 0xa6, 0x9a, 0xb7, 0xe9, 0x04, 0xb6, 0x54, 0x0c,
0xc5, 0xa9, 0x47, 0xa6, 0xc9, 0x08, 0xfe, 0x4e, 0xa6, 0xcc, 0x8a, 0x5b, 0x90, 0x6f, 0x2b,
0x3f, 0xb6, 0x0a, 0x96, 0xc0, 0x78, 0x58, 0x3c, 0x76, 0x6d, 0x94, 0x1a, 0xe4, 0x4e, 0xb8,
0x38, 0xbb, 0xf5, 0xeb, 0x29, 0xd8, 0xb0, 0xf3, 0x15, 0x1e, 0x99, 0x96, 0x3c, 0x5d, 0x63,
0xd5, 0xb1, 0xad, 0x52, 0xb8, 0x55, 0x70, 0x75, 0x3e, 0x1a, 0xd5, 0xda, 0xf6, 0x7a, 0x48,
0x7d, 0x44, 0x41, 0xf9, 0x11, 0xce, 0xd7, 0xca, 0xa5, 0x3d, 0x7a, 0x79, 0x7e, 0x7d, 0x25,
0x1b, 0x77, 0xbc, 0xf7, 0xc7, 0x0f, 0x84, 0x95, 0x10, 0x92, 0x67, 0x15, 0x11, 0x5a, 0x5e,
0x41, 0x66, 0x0f, 0x38, 0x03, 0xb2, 0xf1, 0x5d, 0xf8, 0xab, 0xc0, 0x02, 0x76, 0x84, 0x28,
0xf4, 0x9d, 0x56, 0x46, 0x60, 0x20, 0xdb, 0x68, 0xa7, 0xbb, 0xee, 0xac, 0x15, 0x01, 0x2f,
0x20, 0x09, 0xdb, 0xc0, 0x16, 0xa1, 0x89, 0xf9, 0x94, 0x59, 0x00, 0xc1, 0x76, 0xbf, 0xc1,
0x4d, 0x5d, 0x2d, 0xa9, 0x85, 0x2c, 0xd6, 0xd3, 0x14, 0xcc, 0x02, 0xc3, 0xc2, 0xfa, 0x6b,
0xb7, 0xa6, 0xef, 0xdd, 0x12, 0x26, 0xa4, 0x63, 0xe3, 0x62, 0xbd, 0x56, 0x8a, 0x52, 0x2b,
0xb9, 0xdf, 0x09, 0xbc, 0x0e, 0x97, 0xa9, 0xb0, 0x82, 0x46, 0x08, 0xd5, 0x1a, 0x8e, 0x1b,
0xa7, 0x90, 0x98, 0xb9, 0xbb, 0x3c, 0x17, 0x9a, 0xf2, 0x82, 0xba, 0x64, 0x0a, 0x7f, 0xca,
0x5a, 0x8c, 0x7c, 0xd3, 0x79, 0x09, 0x5b, 0x26, 0xbb, 0xbd, 0x25, 0xdf, 0x3d, 0x6f, 0x9a,
0x8f, 0xee, 0x21, 0x66, 0xb0, 0x8d, 0x84, 0x4c, 0x91, 0x45, 0xd4, 0x77, 0x4f, 0xb3, 0x8c,
0xbc, 0xa8, 0x99, 0xaa, 0x19, 0x53, 0x7c, 0x02, 0x87, 0xbb, 0x0b, 0x7c, 0x1a, 0x2d, 0xdf,
0x48, 0x44, 0x06, 0xd6, 0x7d, 0x0c, 0x2d, 0x35, 0x76, 0xae, 0xc4, 0x5f, 0x71, 0x85, 0x97,
0xc4, 0x3d, 0xef, 0x52, 0xbe, 0x00, 0xe4, 0xcd, 0x49, 0xd1, 0xd1, 0x1c, 0x3c, 0xd0, 0x1c,
0x42, 0xaf, 0xd4, 0xbd, 0x58, 0x34, 0x07, 0x32, 0xee, 0xb9, 0xb5, 0xea, 0xff, 0xd7, 0x8c,
0x0d, 0x2e, 0x2f, 0xaf, 0x87, 0xbb, 0xe6, 0x52, 0x71, 0x22, 0xf5, 0x25, 0x17, 0xa1, 0x82,
0x04, 0xc2, 0x4a, 0xbd, 0x57, 0xc6, 0xab, 0xc8, 0x35, 0x0c, 0x3c, 0xd9, 0xc2, 0x43, 0xdb,
0x27, 0x92, 0xcf, 0xb8, 0x25, 0x60, 0xfa, 0x21, 0x3b, 0x04, 0x52, 0xc8, 0x96, 0xba, 0x74,
0xe3, 0x67, 0x3e, 0x8e, 0x8d, 0x61, 0x90, 0x92, 0x59, 0xb6, 0x1a, 0x1c, 0x5e, 0x21, 0xc1,
0x65, 0xe5, 0xa6, 0x34, 0x05, 0x6f, 0xc5, 0x60, 0xb1, 0x83, 0xc1, 0xd5, 0xd5, 0xed, 0xd9,
0xc7, 0x11, 0x7b, 0x49, 0x7a, 0xf9, 0xf9, 0x84, 0x47, 0x9b, 0xe2, 0xa5, 0x82, 0xe0, 0xc2,
0x88, 0xd0, 0xb2, 0x58, 0x88, 0x7f, 0x45, 0x09, 0x67, 0x74, 0x61, 0xbf, 0xe6, 0x40, 0xe2,
0x9d, 0xc2, 0x47, 0x05, 0x89, 0xed, 0xcb, 0xbb, 0xb7, 0x27, 0xe7, 0xdc, 0x7a, 0xfd, 0xbf,
0xa8, 0xd0, 0xaa, 0x10, 0x39, 0x3c, 0x20, 0xf0, 0xd3, 0x6e, 0xb1, 0x72, 0xf8, 0xe6, 0x0f,
0xef, 0x37, 0xe5, 0x09, 0x33, 0x5a, 0x83, 0x43, 0x80, 0x4f, 0x65, 0x2f, 0x7c, 0x8c, 0x6a,
0xa0, 0x82, 0x0c, 0xd4, 0xd4, 0xfa, 0x81, 0x60, 0x3d, 0xdf, 0x06, 0xf1, 0x5f, 0x08, 0x0d,
0x6d, 0x43, 0xf2, 0xe3, 0x11, 0x7d, 0x80, 0x32, 0xc5, 0xfb, 0xc5, 0xd9, 0x27, 0xec, 0xc6,
0x4e, 0x65, 0x27, 0x76, 0x87, 0xa6, 0xee, 0xee, 0xd7, 0x8b, 0xd1, 0xa0, 0x5c, 0xb0, 0x42,
0x13, 0x0e, 0x95, 0x4a, 0xf2, 0x06, 0xc6, 0x43, 0x33, 0xf4, 0xc7, 0xf8, 0xe7, 0x1f, 0xdd,
0xe4, 0x46, 0x4a, 0x70, 0x39, 0x6c, 0xd0, 0xed, 0xca, 0xbe, 0x60, 0x3b, 0xd1, 0x7b, 0x57,
0x48, 0xe5, 0x3a, 0x79, 0xc1, 0x69, 0x33, 0x53, 0x1b, 0x80, 0xb8, 0x91, 0x7d, 0xb4, 0xf6,
0x17, 0x1a, 0x1d, 0x5a, 0x32, 0xd6, 0xcc, 0x71, 0x29, 0x3f, 0x28, 0xbb, 0xf3, 0x5e, 0x71,
0xb8, 0x43, 0xaf, 0xf8, 0xb9, 0x64, 0xef, 0xc4, 0xa5, 0x6c, 0x08, 0x53, 0xc7, 0x00, 0x10,
0x39, 0x4f, 0xdd, 0xe4, 0xb6, 0x19, 0x27, 0xfb, 0xb8, 0xf5, 0x32, 0x73, 0xe5, 0xcb, 0x32
};

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
qbyte	Q2COM_BlockSequenceCRCByte (qbyte *base, int length, int sequence)
{
	int		n;
	qbyte	*p;
	int		x;
	qbyte chkb[60 + 4];
	unsigned short crc;


	if (sequence < 0)
		Sys_Error("sequence < 0, this shouldn't happen\n");

	p = q2chktbl + (sequence % (sizeof(q2chktbl) - 4));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = QCRC_Block(chkb, length);

	for (x=0, n=0; n<length; n++)
		x += chkb[n];

	crc = (crc ^ x) & 0xff;

	return crc;
}

#endif

#ifdef _WIN32
// don't use these functions in MSVC8
#if (_MSC_VER < 1400)
int VARGS linuxlike_snprintf(char *buffer, int size, const char *format, ...)
{
#undef _vsnprintf
	int ret;
	va_list		argptr;

	if (size <= 0)
		return 0;
	size--;

	va_start (argptr, format);
	ret = _vsnprintf (buffer,size, format,argptr);
	va_end (argptr);

	buffer[size] = '\0';

	return ret;
}

int VARGS linuxlike_vsnprintf(char *buffer, int size, const char *format, va_list argptr)
{
#undef _vsnprintf
	int ret;

	if (size <= 0)
		return 0;
	size--;

	ret = _vsnprintf (buffer,size, format,argptr);

	buffer[size] = '\0';

	return ret;
}
#else
int VARGS linuxlike_snprintf_vc8(char *buffer, int size, const char *format, ...)
{
	int ret;
	va_list		argptr;

	va_start (argptr, format);
	ret = vsnprintf_s (buffer,size, _TRUNCATE, format,argptr);
	va_end (argptr);

	return ret;
}
#endif

#endif

// libSDL.a and libSDLmain.a mingw32 libs use this function for some reason, just here to shut gcc up
#ifdef _MINGW_VFPRINTF
int __mingw_vfprintf (FILE *__stream, const char *__format, __VALIST __local_argv)
{
  return vfprintf( __stream, __format, __local_argv );
}
#endif

int version_number(void)
{
	int base = FTE_VER_MAJOR * 10000 + FTE_VER_MINOR * 100;

#ifdef OFFICIAL_RELEASE
	base -= 1;
#endif

	return base;
}

char *version_string(void)
{
	static char s[128];
	static qboolean done;

	if (!done)
	{
#ifdef OFFICIAL_RELEASE
		Q_snprintfz(s, sizeof(s), "%s v%i.%02i", DISTRIBUTION, FTE_VER_MAJOR, FTE_VER_MINOR);
#elif defined(SVNREVISION)
#define STRINGIFY2(arg) #arg
#define STRINGIFY(arg) STRINGIFY2(arg)
		Q_snprintfz(s, sizeof(s), "%s SVN %s", DISTRIBUTION, STRINGIFY(SVNREVISION));
#undef STRINGIFY
#undef STRINGIFY2
#else
		Q_snprintfz(s, sizeof(s), "%s build %s", DISTRIBUTION, __DATE__);
#endif
		done = true;
	}

	return s;
}
