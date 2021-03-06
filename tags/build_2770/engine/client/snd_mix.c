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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "quakedef.h"

#ifndef NOSOUNDASM
#define NOSOUNDASM	//since channels per sound card went to 6 (portable_samplegroup_t was changed)
#endif

#define	PAINTBUFFER_SIZE	2048

portable_samplegroup_t paintbuffer[PAINTBUFFER_SIZE];

int 	*snd_p, snd_vol;
short	*snd_out;

static int paintskip[6][6] =
{
	{6},
	{1, 5},
	{1, 1, 4},
	{1, 1, 1, 3},
	{1, 1, 1, 1, 2},
	{1, 1, 1, 1, 1, 1}
};

static int chnskip[6][6] =
{
	{0},
	{1, -1},
	{1, 1, -2},
	{1, 1, 1, -3},
	{1, 1, 1, 1, -4},
	{1, 1, 1, 1, 1, -5}
};

void S_TransferPaintBuffer(soundcardinfo_t *sc, int endtime)
{
	int 	startidx, out_idx;
	int 	count;
	int 	outlimit;
	int 	*p;
	int 	*skip;
	int		*cskip;
	int		val;
	int		snd_vol;
	short	*pbuf;

	p = (int *) paintbuffer;
	skip = paintskip[sc->sn.numchannels-1];
	cskip = chnskip[sc->sn.numchannels-1];
	count = (endtime - sc->paintedtime) * sc->sn.numchannels;
	outlimit = sc->sn.samples; 
	startidx = out_idx = (sc->paintedtime * sc->sn.numchannels) % outlimit;
	snd_vol = volume.value*256;

	pbuf = sc->Lock(sc);
	if (!pbuf)
		return;

	if (sc->sn.samplebits == 16)
	{
		short *out = (short *) pbuf;
		while (count--)
		{
			val = (*p * snd_vol) >> 8;
			p += *skip;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			out[out_idx] = val;
			out_idx = (out_idx + 1) % outlimit;
			skip += *cskip;
			cskip += *cskip;
		}
		// Only do this for 1 sound card with 2 channels, because
		// this function is hacky
		if (sc == sndcardinfo && sc->sn.numchannels == 2)
		{
			if (out_idx <= startidx) // buffer looped
			{
				Media_RecordAudioFrame(out + startidx, (sc->sn.samples - startidx) / 2);
				Media_RecordAudioFrame(out, out_idx / 2);
			}
			else
				Media_RecordAudioFrame(out + startidx, (out_idx - startidx) / 2);
		}
	}
	else if (sc->sn.samplebits == 8)
	{
		unsigned char *out = (unsigned char *) pbuf;
		while (count--)
		{
			val = (*p * snd_vol) >> 8;
			p += *skip;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			out[out_idx] = (val>>8) + 128;
			out_idx = (out_idx + 1) % outlimit;
			skip += *cskip;
			cskip += *cskip;
		}
	}

	sc->Unlock(sc, pbuf);
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

void SND_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int endtime);
//void SND_PaintChannelFrom8Duel (channel_t *ch, sfxcache_t *sc, int endtime);
void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int endtime);
void SND_PaintChannelFrom8_4Speaker (channel_t *ch, sfxcache_t *sc, int count);
void SND_PaintChannelFrom16_4Speaker (channel_t *ch, sfxcache_t *sc, int count);
void SND_PaintChannelFrom8_6Speaker (channel_t *ch, sfxcache_t *sc, int count);
void SND_PaintChannelFrom16_6Speaker (channel_t *ch, sfxcache_t *sc, int count);
void SND_PaintChannelFrom8Stereo (channel_t *ch, sfxcache_t *sc, int count);
void SND_PaintChannelFrom16Stereo (channel_t *ch, sfxcache_t *sc, int count);

void S_PaintChannels(soundcardinfo_t *sc, int endtime)
{
	int 	i, j;
	int 	end;
	channel_t *ch;
	sfxcache_t	*scache;
	sfx_t *s;
	int		ltime, count;

//	sc->rawstart += sc->paintedtime - sc->oldpaintedtime;
//	sc->oldpaintedtime = sc->paintedtime;
	while (sc->paintedtime < endtime)
	{
	// if paintbuffer is smaller than DMA buffer
		end = endtime;
		if (endtime - sc->paintedtime > PAINTBUFFER_SIZE)
			end = sc->paintedtime + PAINTBUFFER_SIZE;

	// clear the paint buffer
		Q_memset(paintbuffer, 0, (end - sc->paintedtime) * sizeof(portable_samplegroup_t));

	// paint in the channels.
		ch = sc->channel;
		for (i=0; i<sc->total_chans ; i++, ch++)
		{
			if (!ch->sfx)
				continue;
			if (!ch->vol[0] && !ch->vol[1] && !ch->vol[2] && !ch->vol[3] && !ch->vol[4] && !ch->vol[5])
				continue;

			scache = S_LoadSound (ch->sfx);
			if (!scache)				
				continue;

			if (ch->pos > scache->length)	//cache was flushed and gamedir changed.
			{
				ch->pos = scache->length;
				ch->end = scache->length;
			}


			ltime = sc->paintedtime;

			if (ch->sfx->decoder)
			{
				int len_diff;
				soundcardinfo_t *sndc;
#define qmax(x, y) (x>y)?(x):(y)
				len_diff = scache->length;
//				start = ch->end - scache->length;
//				samples = end - start;

				ch->sfx->decoder->decodemore(ch->sfx, 
					end - (ch->end - scache->length) + 1);
//						ch->pos + end-ltime+1);

				scache = S_LoadSound (ch->sfx);
				if (!scache)
					continue;
				len_diff = scache->length - len_diff;

				for (sndc = sndcardinfo; sndc; sndc=sndc->next)
				{
					for (j = 0; j < sndc->total_chans; j++)
						if (sndc->channel[j].sfx == ch->sfx)	//extend all of these.
							ch->end += len_diff;
				}
			}

			while (ltime < end)
			{	// paint up to end
				if (ch->end < end)
					count = ch->end - ltime;
				else
					count = end - ltime;

				if (count > 0)
				{	
					if (ch->pos < 0)	//delay the sound a little
					{
						if (count > -ch->pos)
							count = -ch->pos;
						ltime += count;
						ch->pos += count;
						continue;
					}

					if (scache->width == 1)
					{						
						if (scache->numchannels==2)
							SND_PaintChannelFrom8Stereo(ch, scache, count);
						else if (sc->sn.numchannels == 6)
							SND_PaintChannelFrom8_6Speaker(ch, scache, count);
						else if (sc->sn.numchannels == 4)
							SND_PaintChannelFrom8_4Speaker(ch, scache, count);
						else	
							SND_PaintChannelFrom8(ch, scache, count);
					}
					else
					{
						if (scache->numchannels==2)
							SND_PaintChannelFrom16Stereo(ch, scache, count);
						else if (sc->sn.numchannels == 6)
							SND_PaintChannelFrom16_6Speaker(ch, scache, count);
						else if (sc->sn.numchannels == 4)
							SND_PaintChannelFrom16_4Speaker(ch, scache, count);
						else
							SND_PaintChannelFrom16(ch, scache, count);
					}
					ltime += count;
				}

			// if at end of loop, restart
				if (ltime >= ch->end)
				{
					if (scache->loopstart >= 0)
					{
						if (scache->length == scache->loopstart)
							break;
						ch->pos = scache->loopstart;
						ch->end = ltime + scache->length - ch->pos;
						if (!scache->length)
						{
							scache->loopstart=-1;
							break;
						}
					}
					else if (ch->looping && scache->length)
					{
						ch->pos = 0;
						ch->end = ltime + scache->length - ch->pos;
					}
					else
					{	// channel just stopped	
						s = ch->sfx;
						ch->sfx = NULL;
						if (s->decoder)
						{							
							if (!S_IsPlayingSomewhere(s))
								s->decoder->abort(s);
						}						
						break;
					}
				}
			}			
		}

	// transfer out according to DMA format
		S_TransferPaintBuffer(sc, end);
		sc->paintedtime = end;
	}
}

//if	defined(NOSOUNDASM) || !id386

void SND_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int count)
{
	int 	data;
	signed char *sfx;
	int		i;

	if (ch->vol[0] > 255)
		ch->vol[0] = 255;
	if (ch->vol[1] > 255)
		ch->vol[1] = 255;
	
	sfx = (signed char *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		data = sfx[i];
		paintbuffer[i].s[0] += ch->vol[0] * data;
		paintbuffer[i].s[1] += ch->vol[1] * data;
	}
		
	ch->pos += count;
}

#if 0
void SND_PaintChannelFrom8Duel (channel_t *ch, sfxcache_t *sc, int count)
{
	signed char *sfx1, *sfx2;
	int		i;

	if (ch->vol[0] > 255)
		ch->vol[0] = 255;
	if (ch->vol[1] > 255)
		ch->vol[1] = 255;
		
	i = ch->pos - ch->delay[0];
	if (i < 0) i = 0;
	sfx1 = (signed char *)sc->data + i;
	i = ch->pos - ch->delay[1];
	if (i < 0) i = 0;
	sfx2 = (signed char *)sc->data + i;

	for (i=0 ; i<count ; i++)
	{
		paintbuffer[i].s[0] += ch->vol[0] * sfx1[i];
		paintbuffer[i].s[1] += ch->vol[1] * sfx2[i];
	}

	ch->pos += count;
}
#endif
//endif	// !id386

void SND_PaintChannelFrom8Stereo (channel_t *ch, sfxcache_t *sc, int count)
{
//	int 	data;
	signed char *sfx;
	int		i;

	if (ch->vol[0] > 255)
		ch->vol[0] = 255;
	if (ch->vol[1] > 255)
		ch->vol[1] = 255;
		
	sfx = (signed char *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{		
		paintbuffer[i].s[0] += ch->vol[0] * sfx[(i<<1)];		
		paintbuffer[i].s[1] += ch->vol[1] * sfx[(i<<1)+1];
	}
		
	ch->pos += count;
}

void SND_PaintChannelFrom8_4Speaker (channel_t *ch, sfxcache_t *sc, int count)
{
	signed char *sfx;
	int		i;

	if (ch->vol[0] > 255)
		ch->vol[0] = 255;
	if (ch->vol[1] > 255)
		ch->vol[1] = 255;
	if (ch->vol[2] > 255)
		ch->vol[2] = 255;
	if (ch->vol[3] > 255)
		ch->vol[3] = 255;

	sfx = (signed char *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		paintbuffer[i].s[0] += ch->vol[0] * sfx[i];
		paintbuffer[i].s[1] += ch->vol[1] * sfx[i];
		paintbuffer[i].s[2] += ch->vol[2] * sfx[i];
		paintbuffer[i].s[3] += ch->vol[3] * sfx[i];
	}

	ch->pos += count;
}

void SND_PaintChannelFrom8_6Speaker (channel_t *ch, sfxcache_t *sc, int count)
{
	signed char *sfx;
	int		i;

	if (ch->vol[0] > 255)
		ch->vol[0] = 255;
	if (ch->vol[1] > 255)
		ch->vol[1] = 255;
	if (ch->vol[2] > 255)
		ch->vol[2] = 255;
	if (ch->vol[3] > 255)
		ch->vol[3] = 255;
	if (ch->vol[4] > 255)
		ch->vol[4] = 255;
	if (ch->vol[5] > 255)
		ch->vol[5] = 255;

	sfx = (signed char *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{		
		paintbuffer[i].s[0] += ch->vol[0] * sfx[i];
		paintbuffer[i].s[1] += ch->vol[1] * sfx[i];
		paintbuffer[i].s[2] += ch->vol[2] * sfx[i];		
		paintbuffer[i].s[3] += ch->vol[3] * sfx[i];
		paintbuffer[i].s[4] += ch->vol[4] * sfx[i];		
		paintbuffer[i].s[5] += ch->vol[5] * sfx[i];
	}
		
	ch->pos += count;
}



void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count)
{
	int data;
	int left, right;
	int leftvol, rightvol;
	signed short *sfx;
	int	i;

	leftvol = ch->vol[0];
	rightvol = ch->vol[1];
	sfx = (signed short *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		data = sfx[i];
		left = (data * leftvol) >> 8;
		right = (data * rightvol) >> 8;
		paintbuffer[i].s[0] += left;
		paintbuffer[i].s[1] += right;
	}
	
	ch->pos += count;
}

void SND_PaintChannelFrom16Stereo (channel_t *ch, sfxcache_t *sc, int count)
{
	int leftvol, rightvol;
	signed short *sfx;
	int	i;

	leftvol = ch->vol[0];
	rightvol = ch->vol[1];
	sfx = (signed short *)sc->data + ch->pos*2;

	for (i=0 ; i<count ; i++)
	{
		paintbuffer[i].s[0] += (*sfx++ * leftvol) >> 8;
		paintbuffer[i].s[1] += (*sfx++ * rightvol) >> 8;
	}
	
	ch->pos += count;
}

void SND_PaintChannelFrom16_6Speaker (channel_t *ch, sfxcache_t *sc, int count)
{
	int vol[6];
	signed short *sfx;
	int	i;

	vol[0] = ch->vol[0];
	vol[1] = ch->vol[1];
	vol[2] = ch->vol[2];
	vol[3] = ch->vol[3];
	vol[4] = ch->vol[4];
	vol[5] = ch->vol[5];
	sfx = (signed short *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		paintbuffer[i].s[0] += (sfx[i] * vol[0]) >> 8;
		paintbuffer[i].s[1] += (sfx[i] * vol[1]) >> 8;
		paintbuffer[i].s[2] += (sfx[i] * vol[2]) >> 8;
		paintbuffer[i].s[3] += (sfx[i] * vol[3]) >> 8;
		paintbuffer[i].s[4] += (sfx[i] * vol[4]) >> 8;
		paintbuffer[i].s[5] += (sfx[i] * vol[5]) >> 8;
	}
	
	ch->pos += count;
}

void SND_PaintChannelFrom16_4Speaker (channel_t *ch, sfxcache_t *sc, int count)
{
	int vol[4];
	signed short *sfx;
	int	i;

	vol[0] = ch->vol[0];
	vol[1] = ch->vol[1];
	vol[2] = ch->vol[2];
	vol[3] = ch->vol[3];
	sfx = (signed short *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		paintbuffer[i].s[0] += (sfx[i] * vol[0]) >> 8;
		paintbuffer[i].s[1] += (sfx[i] * vol[1]) >> 8;
		paintbuffer[i].s[2] += (sfx[i] * vol[2]) >> 8;
		paintbuffer[i].s[3] += (sfx[i] * vol[3]) >> 8;
	}
	
	ch->pos += count;
}
