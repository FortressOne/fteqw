
//a note about dedicated servers:
//In the server-side gamecode, a couple of q1 extensions require knowing something about models.
//So we load models serverside, if required.

//things we need:
//tag/bone names and indexes so we can have reasonable modding with tags. :)
//tag/bone positions so we can shoot from the actual gun or other funky stuff
//vertex positions so we can trace against the mesh rather than the bbox.

//we use the gl renderer's model code because it supports more sorts of models than the sw renderer. Sad but true.




#include "quakedef.h"
#include "glquake.h"
#if defined(GLQUAKE) || defined(D3DQUAKE)

#ifdef _WIN32
	#include <malloc.h>
#else
	#include <alloca.h>
#endif

#define MAX_BONES 256

#include "com_mesh.h"

//FIXME
typedef struct
{
	float		scale[3];	// multiply qbyte verts by this
	float		translate[3];	// then add this
	char		name[16];	// frame name from grabbing
	dtrivertx_t	verts[1];	// variable sized
} dmd2aliasframe_t;



extern cvar_t gl_part_flame, r_fullbrightSkins, r_fb_models;
extern cvar_t r_noaliasshadows;



extern char	loadname[32];	// for hunk tags


extern cvar_t gl_ati_truform;
extern cvar_t r_vertexdlights;
extern cvar_t mod_md3flags;
extern cvar_t r_skin_overlays;

#ifndef SERVERONLY
static hashtable_t skincolourmapped;

//changes vertex lighting values
#if 0
static void R_GAliasApplyLighting(mesh_t *mesh, vec3_t org, vec3_t angles, float *colormod)
{
	int l, v;
	vec3_t rel;
	vec3_t dir;
	float dot, d, a;

	if (mesh->colors4f_array)
	{
		float l;
		int temp;
		int i;
		avec4_t *colours = mesh->colors4f_array;
		vec3_t *normals = mesh->normals_array;
		vec3_t ambient, shade;
		qbyte alphab = bound(0, colormod[3], 1);
		if (!mesh->normals_array)
		{
			mesh->colors4f_array = NULL;
			return;
		}

		VectorCopy(ambientlight, ambient);
		VectorCopy(shadelight, shade);

		for (i = 0; i < 3; i++)
		{
			ambient[i] *= colormod[i];
			shade[i] *= colormod[i];
		}


		for (i = mesh->numvertexes-1; i >= 0; i--)
		{
			l = DotProduct(normals[i], shadevector);

			temp = l*ambient[0]+shade[0];
			colours[i][0] = temp;

			temp = l*ambient[1]+shade[1];
			colours[i][1] = temp;

			temp = l*ambient[2]+shade[2];
			colours[i][2] = temp;

			colours[i][3] = alphab;
		}
	}

	if (r_vertexdlights.value && mesh->colors4f_array)
	{
		//don't include world lights
		for (l=rtlights_first ; l<RTL_FIRST; l++)
		{
			if (cl_dlights[l].radius)
			{
				VectorSubtract (cl_dlights[l].origin,
								org,
								dir);
				if (Length(dir)>cl_dlights[l].radius+mesh->radius)	//far out man!
					continue;

				rel[0] = -DotProduct(dir, currententity->axis[0]);
				rel[1] = -DotProduct(dir, currententity->axis[1]);	//quake's crazy.
				rel[2] = -DotProduct(dir, currententity->axis[2]);
	/*
				glBegin(GL_LINES);
				glVertex3f(0,0,0);
				glVertex3f(rel[0],rel[1],rel[2]);
				glEnd();
	*/
				for (v = 0; v < mesh->numvertexes; v++)
				{
					VectorSubtract(mesh->xyz_array[v], rel, dir);
					dot = DotProduct(dir, mesh->normals_array[v]);
					if (dot>0)
					{
						d = DotProduct(dir, dir);
						a = 1/d;
						if (a>0)
						{
							a *= 10000000*dot/sqrt(d);
							mesh->colors4f_array[v][0] += a*cl_dlights[l].color[0];
							mesh->colors4f_array[v][1] += a*cl_dlights[l].color[1];
							mesh->colors4f_array[v][2] += a*cl_dlights[l].color[2];
						}
	//					else
	//						mesh->colors4f_array[v][1] = 1;
					}
	//				else
	//					mesh->colors4f_array[v][2] = 1;
				}
			}
		}
	}
}
#endif

void GL_GAliasFlushSkinCache(void)
{
	int i;
	bucket_t *b;
	for (i = 0; i < skincolourmapped.numbuckets; i++)
	{
		while((b = skincolourmapped.bucket[i]))
		{
			skincolourmapped.bucket[i] = b->next;
			BZ_Free(b->data);
		}
	}
	if (skincolourmapped.bucket)
		BZ_Free(skincolourmapped.bucket);
	skincolourmapped.bucket = NULL;
	skincolourmapped.numbuckets = 0;
}

static texnums_t *GL_ChooseSkin(galiasinfo_t *inf, model_t *model, int surfnum, entity_t *e)
{
	galiasskin_t *skins;
	texnums_t *texnums;
	int frame;
	unsigned int subframe;
	extern int cl_playerindex;	//so I don't have to strcmp

	unsigned int tc, bc, pc;
	qboolean forced;

	if (e->skinnum >= 100 && e->skinnum < 110)
	{
		shader_t *s;
		s = R_RegisterSkin(va("gfx/skin%d.lmp", e->skinnum));
		if (!TEXVALID(s->defaulttextures.base))
			s->defaulttextures.base = R_LoadHiResTexture(va("gfx/skin%d.lmp", e->skinnum), NULL, 0);
		s->defaulttextures.shader = s;
		return &s->defaulttextures;
	}


	if ((e->model->engineflags & MDLF_NOTREPLACEMENTS) && !ruleset_allow_sensative_texture_replacements.ival)
		forced = true;
	else
		forced = false;

	if (!gl_nocolors.ival || forced)
	{
		if (e->scoreboard)
		{
			if (!e->scoreboard->skin)
				Skin_Find(e->scoreboard);
			tc = e->scoreboard->ttopcolor;
			bc = e->scoreboard->tbottomcolor;
			pc = e->scoreboard->h2playerclass;
		}
		else
		{
			tc = 1;
			bc = 1;
			pc = 0;
		}

		if (forced || tc != 1 || bc != 1 || (e->scoreboard && e->scoreboard->skin))
		{
			int			inwidth, inheight;
			int			tinwidth, tinheight;
			char *skinname;
			qbyte	*original;
			galiascolourmapped_t *cm;
			char hashname[512];

//			if (e->scoreboard->skin->cachedbpp

	/*		if (cls.protocol == CP_QUAKE2)
			{
				if (e->scoreboard && e->scoreboard->skin)
					snprintf(hashname, sizeof(hashname), "%s$%s$%i", modelname, e->scoreboard->skin->name, surfnum);
				else
					snprintf(hashname, sizeof(hashname), "%s$%i", modelname, surfnum);
				skinname = hashname;
			}
			else */
			{
				if (e->scoreboard && e->scoreboard->skin)
				{
					snprintf(hashname, sizeof(hashname), "%s$%s$%i", model->name, e->scoreboard->skin->name, surfnum);
					skinname = hashname;
				}
				else if (surfnum)
				{
					snprintf(hashname, sizeof(hashname), "%s$%i", model->name, surfnum);
					skinname = hashname;
				}
				else
					skinname = model->name;
			}

			if (!skincolourmapped.numbuckets)
			{
				void *buckets = BZ_Malloc(Hash_BytesForBuckets(256));
				memset(buckets, 0, Hash_BytesForBuckets(256));
				Hash_InitTable(&skincolourmapped, 256, buckets);
			}

			if (!inf->numskins)
			{
				skins = NULL;
				subframe = 0;
				texnums = NULL;
			}
			else
			{
				skins = (galiasskin_t*)((char *)inf + inf->ofsskins);
				if (!skins->texnums)
				{
					skins = NULL;
					subframe = 0;
					texnums = NULL;
				}
				else
				{
					if (e->skinnum >= 0 && e->skinnum < inf->numskins)
						skins += e->skinnum;

					subframe = cl.time*skins->skinspeed;
					subframe = subframe%skins->texnums;

					texnums = (texnums_t*)((char *)skins + skins->ofstexnums + subframe*sizeof(texnums_t));
				}
			}

			for (cm = Hash_Get(&skincolourmapped, skinname); cm; cm = Hash_GetNext(&skincolourmapped, skinname, cm))
			{
				if (cm->tcolour == tc && cm->bcolour == bc && cm->skinnum == e->skinnum && cm->subframe == subframe && cm->pclass == pc)
				{
					return &cm->texnum;
				}
			}

			//colourmap isn't present yet.
			cm = BZ_Malloc(sizeof(*cm));
			Q_strncpyz(cm->name, skinname, sizeof(cm->name));
			Hash_Add(&skincolourmapped, cm->name, cm, &cm->bucket);
			cm->tcolour = tc;
			cm->bcolour = bc;
			cm->pclass = pc;
			cm->skinnum = e->skinnum;
			cm->subframe = subframe;
			cm->texnum.fullbright = r_nulltex;
			cm->texnum.base = r_nulltex;
			cm->texnum.loweroverlay = r_nulltex;
			cm->texnum.upperoverlay = r_nulltex;
			cm->texnum.shader = texnums?texnums->shader:R_RegisterSkin(skinname);

			if (!texnums)
			{	//load just the skin
				if (e->scoreboard && e->scoreboard->skin)
				{
					if (cls.protocol == CP_QUAKE2)
					{
						original = Skin_Cache32(e->scoreboard->skin);
						if (original)
						{
							inwidth = e->scoreboard->skin->width;
							inheight = e->scoreboard->skin->height;
							cm->texnum.base = R_LoadTexture32(e->scoreboard->skin->name, inwidth, inheight, (unsigned int*)original, IF_NOALPHA|IF_NOGAMMA);
							return &cm->texnum;
						}
					}
					else
					{
						original = Skin_Cache8(e->scoreboard->skin);
						if (original)
						{
							inwidth = e->scoreboard->skin->width;
							inheight = e->scoreboard->skin->height;
							cm->texnum.base = R_LoadTexture8(e->scoreboard->skin->name, inwidth, inheight, original, IF_NOALPHA|IF_NOGAMMA, 1);
							return &cm->texnum;
						}
					}

					if (TEXVALID(e->scoreboard->skin->tex_base))
					{
						texnums = &cm->texnum;
						texnums->loweroverlay = e->scoreboard->skin->tex_lower;
						texnums->upperoverlay = e->scoreboard->skin->tex_upper;
						texnums->base = e->scoreboard->skin->tex_base;
						return texnums;
					}

					cm->texnum.base = R_LoadHiResTexture(e->scoreboard->skin->name, "skins", IF_NOALPHA);
					return &cm->texnum;
				}
				return NULL;
			}

			cm->texnum.bump = texnums[cm->skinnum].bump;	//can't colour bumpmapping
			if (cls.protocol != CP_QUAKE2 && ((!texnums || (model==cl.model_precache[cl_playerindex] || model==cl.model_precache_vwep[0])) && e->scoreboard && e->scoreboard->skin))
			{
				original = Skin_Cache8(e->scoreboard->skin);
				inwidth = e->scoreboard->skin->width;
				inheight = e->scoreboard->skin->height;

				if (!original && TEXVALID(e->scoreboard->skin->tex_base))
				{
					texnums = &cm->texnum;
					texnums->loweroverlay = e->scoreboard->skin->tex_lower;
					texnums->upperoverlay = e->scoreboard->skin->tex_upper;
					texnums->base = e->scoreboard->skin->tex_base;
					return texnums;
				}
			}
			else
			{
				original = NULL;
				inwidth = 0;
				inheight = 0;
			}
			if (!original)
			{
				if (skins->ofstexels)
				{
					original = (qbyte *)skins + skins->ofstexels;
					inwidth = skins->skinwidth;
					inheight = skins->skinheight;
				}
				else
				{
					original = NULL;
					inwidth = 0;
					inheight = 0;
				}
			}
			tinwidth = skins->skinwidth;
			tinheight = skins->skinheight;
			if (original)
			{
				int i, j;
				unsigned translate32[256];
				static unsigned	pixels[512*512];
				unsigned	*out;
				unsigned	frac, fracstep;

				unsigned	scaled_width, scaled_height;
				qbyte		*inrow;

				texnums = &cm->texnum;

				texnums->base = r_nulltex;
				texnums->fullbright = r_nulltex;

				scaled_width = gl_max_size.value < 512 ? gl_max_size.value : 512;
				scaled_height = gl_max_size.value < 512 ? gl_max_size.value : 512;

				//handle the case of an external skin being smaller than the texture that its meant to replace
				//(to support the evil hackage of the padding on the outside of common qw skins)
				if (tinwidth > inwidth)
					tinwidth = inwidth;
				if (tinheight > inheight)
					tinheight = inheight;

				//don't make scaled width any larger than it needs to be
				for (i = 0; i < 10; i++)
				{
					scaled_width = (1<<i);
					if (scaled_width >= tinwidth)
						break;	//its covered
				}
				if (scaled_width > gl_max_size.value)
					scaled_width = gl_max_size.value;	//whoops, we made it too big

				for (i = 0; i < 10; i++)
				{
					scaled_height = (1<<i);
					if (scaled_height >= tinheight)
						break;	//its covered
				}
				if (scaled_height > gl_max_size.value)
					scaled_height = gl_max_size.value;	//whoops, we made it too big

				if (scaled_width < 4)
					scaled_width = 4;
				if (scaled_height < 4)
					scaled_height = 4;

				if (h2playertranslations && pc)
				{
					unsigned int color_offsets[5] = {2*14*256,0,1*14*256,2*14*256,2*14*256};
					unsigned char *colorA, *colorB, *sourceA, *sourceB;
					colorA = h2playertranslations + 256 + color_offsets[pc-1];
					colorB = colorA + 256;
					sourceA = colorB + (tc * 256);
					sourceB = colorB + (bc * 256);
					for(i=0;i<256;i++)
					{
						translate32[i] = d_8to24rgbtable[i];
						if (tc > 0 && (colorA[i] != 255))
							translate32[i] = d_8to24rgbtable[sourceA[i]];
						if (bc > 0 && (colorB[i] != 255))
							translate32[i] = d_8to24rgbtable[sourceB[i]];
					}
					translate32[0] = 0;
				}
				else
				{
					for (i=0 ; i<256 ; i++)
						translate32[i] = d_8to24rgbtable[i];

					for (i = 0; i < 16; i++)
					{
						if (tc >= 16)
						{
							//assumption: row 0 is pure white.
							*((unsigned char*)&translate32[TOP_RANGE+i]+0) = (((tc&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[i]+0))>>8;
							*((unsigned char*)&translate32[TOP_RANGE+i]+1) = (((tc&0x00ff00)>> 8)**((unsigned char*)&d_8to24rgbtable[i]+1))>>8;
							*((unsigned char*)&translate32[TOP_RANGE+i]+2) = (((tc&0x0000ff)>> 0)**((unsigned char*)&d_8to24rgbtable[i]+2))>>8;
							*((unsigned char*)&translate32[TOP_RANGE+i]+3) = 0xff;
						}
						else
						{
							if (tc < 8)
								translate32[TOP_RANGE+i] = d_8to24rgbtable[(tc<<4)+i];
							else
								translate32[TOP_RANGE+i] = d_8to24rgbtable[(tc<<4)+15-i];
						}
						if (bc >= 16)
						{
							*((unsigned char*)&translate32[BOTTOM_RANGE+i]+0) = (((bc&0xff0000)>>16)**((unsigned char*)&d_8to24rgbtable[i]+0))>>8;
							*((unsigned char*)&translate32[BOTTOM_RANGE+i]+1) = (((bc&0x00ff00)>> 8)**((unsigned char*)&d_8to24rgbtable[i]+1))>>8;
							*((unsigned char*)&translate32[BOTTOM_RANGE+i]+2) = (((bc&0x0000ff)>> 0)**((unsigned char*)&d_8to24rgbtable[i]+2))>>8;
							*((unsigned char*)&translate32[BOTTOM_RANGE+i]+3) = 0xff;
						}
						else
						{
							if (bc < 8)
								translate32[BOTTOM_RANGE+i] = d_8to24rgbtable[(bc<<4)+i];
							else
								translate32[BOTTOM_RANGE+i] = d_8to24rgbtable[(bc<<4)+15-i];
						}
					}
				}

				out = pixels;
				fracstep = tinwidth*0x10000/scaled_width;
				for (i=0 ; i<scaled_height ; i++, out += scaled_width)
				{
					inrow = original + inwidth*(i*inheight/scaled_height);
					frac = fracstep >> 1;
					for (j=0 ; j<scaled_width ; j+=4)
					{
						out[j] = translate32[inrow[frac>>16]];
						frac += fracstep;
						out[j+1] = translate32[inrow[frac>>16]];
						frac += fracstep;
						out[j+2] = translate32[inrow[frac>>16]];
						frac += fracstep;
						out[j+3] = translate32[inrow[frac>>16]];
						frac += fracstep;
					}
				}
				if (qrenderer == QR_OPENGL)
				{
					texnums->base = R_AllocNewTexture(scaled_width, scaled_height);
					R_Upload(texnums->base, "", h2playertranslations?TF_RGBA32:TF_RGBX32, pixels, NULL, scaled_width, scaled_height, IF_NOMIPMAP);
				}
				else
				{
					texnums->base = R_LoadTexture(NULL, scaled_width, scaled_height, h2playertranslations?TF_RGBA32:TF_RGBX32, pixels, 0);
				}

				if (!h2playertranslations)
				{
					//now do the fullbrights.
					out = pixels;
					fracstep = tinwidth*0x10000/scaled_width;
					for (i=0 ; i<scaled_height ; i++, out += scaled_width)
					{
						inrow = original + inwidth*(i*inheight/scaled_height);
						frac = fracstep >> 1;
						for (j=0 ; j<scaled_width ; j+=1)
						{
							if (inrow[frac>>16] < 255-vid.fullbright)
								((char *) (&out[j]))[3] = 0;	//alpha 0
							frac += fracstep;
						}
					}
					if (qrenderer == QR_OPENGL)
					{
						texnums->fullbright = R_AllocNewTexture(scaled_width, scaled_height);
						R_Upload(texnums->fullbright, "", TF_RGBA32, pixels, NULL, scaled_width, scaled_height, IF_NOMIPMAP);
					}
					else
					{
						texnums->fullbright = R_LoadTexture(NULL, scaled_width, scaled_height, h2playertranslations?TF_RGBA32:TF_RGBX32, pixels, 0);
					}
				}
			}
			else
			{
				skins = (galiasskin_t*)((char *)inf + inf->ofsskins);
				if (e->skinnum >= 0 && e->skinnum < inf->numskins)
					skins += e->skinnum;

				if (!inf->numskins || !skins->texnums)
					return NULL;

				frame = cl.time*skins->skinspeed;
				frame = frame%skins->texnums;
				texnums = (texnums_t*)((char *)skins + skins->ofstexnums + frame*sizeof(texnums_t));
				memcpy(&cm->texnum, texnums, sizeof(cm->texnum));
			}
			return &cm->texnum;
		}
	}

	if (!inf->numskins)
		return NULL;

	skins = (galiasskin_t*)((char *)inf + inf->ofsskins);
	if (e->skinnum >= 0 && e->skinnum < inf->numskins)
		skins += e->skinnum;
	else
	{
		Con_DPrintf("Skin number out of range\n");
		if (!inf->numskins)
			return NULL;
	}

	if (!skins->texnums)
		return NULL;

	frame = cl.time*skins->skinspeed;
	frame = frame%skins->texnums;
	texnums = (texnums_t*)((char *)skins + skins->ofstexnums + frame*sizeof(texnums_t));

	return texnums;
}

#if defined(RTLIGHTS) && defined(GLQUAKE)
static int numFacing;
static qbyte *triangleFacing;
static void R_CalcFacing(mesh_t *mesh, vec3_t lightpos)
{
	float *v1, *v2, *v3;
	vec3_t d1, d2, norm;

	int i;

	index_t *indexes = mesh->indexes;
	int numtris = mesh->numindexes/3;


	if (numFacing < numtris)
	{
		if (triangleFacing)
			BZ_Free(triangleFacing);
		triangleFacing = BZ_Malloc(sizeof(*triangleFacing)*numtris);
		numFacing = numtris;
	}

	for (i = 0; i < numtris; i++, indexes+=3)
	{
		v1 = (float *)(mesh->xyz_array + indexes[0]);
		v2 = (float *)(mesh->xyz_array + indexes[1]);
		v3 = (float *)(mesh->xyz_array + indexes[2]);

		VectorSubtract(v1, v2, d1);
		VectorSubtract(v3, v2, d2);
		CrossProduct(d1, d2, norm);

		triangleFacing[i] = (( lightpos[0] - v1[0] ) * norm[0] + ( lightpos[1] - v1[1] ) * norm[1] + ( lightpos[2] - v1[2] ) * norm[2]) > 0;
	}
}

#define PROJECTION_DISTANCE 30000
static int numProjectedShadowVerts;
static vec3_t *ProjectedShadowVerts;
static void R_ProjectShadowVolume(mesh_t *mesh, vec3_t lightpos)
{
	int numverts = mesh->numvertexes;
	int i;
	vecV_t *input = mesh->xyz_array;
	vec3_t *projected;
	if (numProjectedShadowVerts < numverts)
	{
		if (ProjectedShadowVerts)
			BZ_Free(ProjectedShadowVerts);
		ProjectedShadowVerts = BZ_Malloc(sizeof(*ProjectedShadowVerts)*numverts);
		numProjectedShadowVerts = numverts;
	}
	projected = ProjectedShadowVerts;
	for (i = 0; i < numverts; i++)
	{
		projected[i][0] = input[i][0] + (input[i][0]-lightpos[0])*PROJECTION_DISTANCE;
		projected[i][1] = input[i][1] + (input[i][1]-lightpos[1])*PROJECTION_DISTANCE;
		projected[i][2] = input[i][2] + (input[i][2]-lightpos[2])*PROJECTION_DISTANCE;
	}
}

static void R_DrawShadowVolume(mesh_t *mesh)
{
	int t;
	vec3_t *proj = ProjectedShadowVerts;
	vecV_t *verts = mesh->xyz_array;
	index_t *indexes = mesh->indexes;
	int *neighbours = mesh->trneighbors;
	int numtris = mesh->numindexes/3;

	qglBegin(GL_TRIANGLES);
	for (t = 0; t < numtris; t++)
	{
		if (triangleFacing[t])
		{
			//draw front
			qglVertex3fv(verts[indexes[t*3+0]]);
			qglVertex3fv(verts[indexes[t*3+1]]);
			qglVertex3fv(verts[indexes[t*3+2]]);

			//draw back
			qglVertex3fv(proj[indexes[t*3+1]]);
			qglVertex3fv(proj[indexes[t*3+0]]);
			qglVertex3fv(proj[indexes[t*3+2]]);

			//draw side caps
			if (neighbours[t*3+0] < 0 || !triangleFacing[neighbours[t*3+0]])
			{
				qglVertex3fv(verts[indexes[t*3+1]]);
				qglVertex3fv(verts[indexes[t*3+0]]);
				qglVertex3fv(proj [indexes[t*3+0]]);
				qglVertex3fv(verts[indexes[t*3+1]]);
				qglVertex3fv(proj [indexes[t*3+0]]);
				qglVertex3fv(proj [indexes[t*3+1]]);
			}

			if (neighbours[t*3+1] < 0 || !triangleFacing[neighbours[t*3+1]])
			{
				qglVertex3fv(verts[indexes[t*3+2]]);
				qglVertex3fv(verts[indexes[t*3+1]]);
				qglVertex3fv(proj [indexes[t*3+1]]);
				qglVertex3fv(verts[indexes[t*3+2]]);
				qglVertex3fv(proj [indexes[t*3+1]]);
				qglVertex3fv(proj [indexes[t*3+2]]);
			}

			if (neighbours[t*3+2] < 0 || !triangleFacing[neighbours[t*3+2]])
			{
				qglVertex3fv(verts[indexes[t*3+0]]);
				qglVertex3fv(verts[indexes[t*3+2]]);
				qglVertex3fv(proj [indexes[t*3+2]]);
				qglVertex3fv(verts[indexes[t*3+0]]);
				qglVertex3fv(proj [indexes[t*3+2]]);
				qglVertex3fv(proj [indexes[t*3+0]]);
			}
		}
	}
	qglEnd();
}
#endif

//true if no shading is to be used.
static qboolean R_CalcModelLighting(entity_t *e, model_t *clmodel)
{
	vec3_t lightdir;
	int i;
	vec3_t dist;
	float add;
	vec3_t shadelight, ambientlight;

	if (e->light_known)
		return e->light_known-1;

	e->light_dir[0] = 0; e->light_dir[1] = 1; e->light_dir[2] = 0;
	if (clmodel->engineflags & MDLF_FLAME || r_fullbright.ival)
	{
		e->light_avg[0] = e->light_avg[1] = e->light_avg[2] = 1;
		e->light_range[0] = e->light_range[1] = e->light_range[2] = 0;
		e->light_known = 2;
		return e->light_known-1;
	}
	if ((e->drawflags & MLS_MASKIN) == MLS_FULLBRIGHT || (e->flags & Q2RF_FULLBRIGHT))
	{
		e->light_avg[0] = e->light_avg[1] = e->light_avg[2] = 1;
		e->light_range[0] = e->light_range[1] = e->light_range[2] = 0;
		e->light_known = 2;
		return e->light_known-1;
	}

	if (!(r_refdef.flags & Q2RDF_NOWORLDMODEL))
	{
		if (e->flags & Q2RF_WEAPONMODEL)
		{
			cl.worldmodel->funcs.LightPointValues(cl.worldmodel, r_refdef.vieworg, shadelight, ambientlight, lightdir);
			for (i = 0; i < 3; i++)
			{	/*viewmodels may not be pure black*/
				if (ambientlight[i] < 24)
					ambientlight[i] = 24;
			}
		}
		else
		{
			vec3_t center;
			#if 0 /*hexen2*/
			VectorAvg(clmodel->mins, clmodel->maxs, center);
			VectorAdd(e->origin, center, center);
			#else
			VectorCopy(e->origin, center);
			center[2] += 8;
			#endif
			cl.worldmodel->funcs.LightPointValues(cl.worldmodel, center, shadelight, ambientlight, lightdir);
		}
	}
	else
	{
		ambientlight[0] = ambientlight[1] = ambientlight[2] = shadelight[0] = shadelight[1] = shadelight[2] = 128;
		lightdir[0] = 0;
		lightdir[1] = 1;
		lightdir[2] = 1;
	}

	if (!r_vertexdlights.ival && r_dynamic.ival)
	{
		//don't do world lights, although that might be funny
		for (i=rtlights_first; i<RTL_FIRST; i++)
		{
			if (cl_dlights[i].radius)
			{
				VectorSubtract (e->origin,
								cl_dlights[i].origin,
								dist);
				add = cl_dlights[i].radius - Length(dist);

				if (add > 0) {
					add*=5;
					ambientlight[0] += add * cl_dlights[i].color[0];
					ambientlight[1] += add * cl_dlights[i].color[1];
					ambientlight[2] += add * cl_dlights[i].color[2];
					//ZOID models should be affected by dlights as well
					shadelight[0] += add * cl_dlights[i].color[0];
					shadelight[1] += add * cl_dlights[i].color[1];
					shadelight[2] += add * cl_dlights[i].color[2];
				}
			}
		}
	}

	for (i = 0; i < 3; i++)	//clamp light so it doesn't get vulgar.
	{
		if (ambientlight[i] > 128)
			ambientlight[i] = 128;
		if (shadelight[i] > 192)
			shadelight[i] = 192;
	}

//MORE HUGE HACKS! WHEN WILL THEY CEASE!
	// clamp lighting so it doesn't overbright as much
	// ZOID: never allow players to go totally black
	if (clmodel->engineflags & MDLF_PLAYER)
	{
		float fb = r_fullbrightSkins.value;
		if (fb > cls.allow_fbskins)
			fb = cls.allow_fbskins;
		if (fb < 0)
			fb = 0;
		if (fb)
		{
			extern cvar_t r_fb_models;

			if (fb >= 1 && r_fb_models.value)
			{
				ambientlight[0] = ambientlight[1] = ambientlight[2] = 1;
				shadelight[0] = shadelight[1] = shadelight[2] = 1;

				e->light_known = 2;
				return e->light_known-1;
			}
			else
			{
				for (i = 0; i < 3; i++)
				{
					ambientlight[i] = max(ambientlight[i], 8 + fb * 120);
					shadelight[i] = max(shadelight[i], 8 + fb * 120);
				}
			}
		}
		for (i = 0; i < 3; i++)
		{
			if (ambientlight[i] < 8)
				ambientlight[i] = 8;
		}
	}


	for (i = 0; i < 3; i++)
	{
		if (ambientlight[i] > 128)
			ambientlight[i] = 128;

		shadelight[i] /= 200.0/255;
		ambientlight[i] /= 200.0/255;
	}

	if ((e->model->flags & EF_ROTATE) && cl.hexen2pickups)
	{
		shadelight[0] = shadelight[1] = shadelight[2] =
		ambientlight[0] = ambientlight[1] = ambientlight[2] = 128+sin(cl.servertime*4)*64;
	}
	if ((e->drawflags & MLS_MASKIN) == MLS_ABSLIGHT)
	{
		shadelight[0] = shadelight[1] = shadelight[2] = e->abslight;
		ambientlight[0] = ambientlight[1] = ambientlight[2] = e->abslight;
	}

//#define SHOWLIGHTDIR
	{	//lightdir is absolute, shadevector is relative
		e->light_dir[0] = DotProduct(lightdir, e->axis[0]);
		e->light_dir[1] = DotProduct(lightdir, e->axis[1]);
		e->light_dir[2] = DotProduct(lightdir, e->axis[2]);

		if (e->flags & Q2RF_WEAPONMODEL)
		{
			vec3_t temp;
			temp[0] = DotProduct(e->light_dir, vpn);
			temp[1] = -DotProduct(e->light_dir, vright);
			temp[2] = DotProduct(e->light_dir, vup);

			VectorCopy(temp, e->light_dir);
		}

		VectorNormalize(e->light_dir);

	}

	shadelight[0] *= 1/255.0f;
	shadelight[1] *= 1/255.0f;
	shadelight[2] *= 1/255.0f;
	ambientlight[0] *= 1/255.0f;
	ambientlight[1] *= 1/255.0f;
	ambientlight[2] *= 1/255.0f;

	if (e->flags & Q2RF_GLOW)
	{
		shadelight[0] += sin(cl.time)*0.25;
		shadelight[1] += sin(cl.time)*0.25;
		shadelight[2] += sin(cl.time)*0.25;
	}

	VectorMA(ambientlight, 0.5, shadelight, e->light_avg);
	VectorSubtract(shadelight, ambientlight, e->light_range);

	e->light_known = 1;
	return e->light_known-1;
}

void R_GAlias_DrawBatch(batch_t *batch)
{
	entity_t *e;

	galiasinfo_t *inf;
	model_t *clmodel;
	int surfnum;

	static mesh_t mesh;
	static mesh_t *meshl = &mesh;

	qboolean needrecolour;
	qboolean nolightdir;

	e = batch->ent;
	clmodel = e->model;

	currententity = e;
	nolightdir = R_CalcModelLighting(e, clmodel);

	inf = RMod_Extradata (clmodel);
	if (inf)
	{
		memset(&mesh, 0, sizeof(mesh));
		for(surfnum=0; inf; ((inf->nextsurf)?(inf = (galiasinfo_t*)((char *)inf + inf->nextsurf)):(inf=NULL)), surfnum++)
		{
			if (batch->surf_first == surfnum)
			{
				needrecolour = Alias_GAliasBuildMesh(&mesh, inf, e, e->shaderRGBAf[3], nolightdir);
				batch->mesh = &meshl;
				return;
			}
		}
	}
	batch->meshes = 0;
	Con_Printf("Broken model surfaces mid-frame\n");
	return;
}

void R_GAlias_GenerateBatches(entity_t *e, batch_t **batches)
{
	galiasinfo_t *inf;
	model_t *clmodel;
	shader_t *shader;
	batch_t *b;
	int surfnum;
	shadersort_t sort;

	texnums_t *skin;

	if (r_refdef.externalview && e->flags & Q2RF_WEAPONMODEL)
		return;

	/*switch model if we're the player model, and the player skin says a new model*/
	{
		extern int cl_playerindex;
		if (e->scoreboard && e->model == cl.model_precache[cl_playerindex])
		{
			clmodel = e->scoreboard->model;
			if (clmodel && clmodel->type == mod_alias)
				e->model = clmodel;
		}
	}

	clmodel = e->model;

	if (!(e->flags & Q2RF_WEAPONMODEL))
	{
		if (R_CullEntityBox (e, clmodel->mins, clmodel->maxs))
			return;
#ifdef RTLIGHTS
		if (BE_LightCullModel(e->origin, clmodel))
			return;
	}
	else
	{
		if (BE_LightCullModel(r_origin, clmodel))
			return;
#endif
	}

	if (clmodel->tainted)
	{
		if (!ruleset_allow_modified_eyes.ival && !strcmp(clmodel->name, "progs/eyes.mdl"))
			return;
	}

	inf = RMod_Extradata (clmodel);

	for(surfnum=0; inf; ((inf->nextsurf)?(inf = (galiasinfo_t*)((char *)inf + inf->nextsurf)):(inf=NULL)), surfnum++)
	{
		skin = GL_ChooseSkin(inf, clmodel, surfnum, e);
		if (!skin)
			continue;
		shader = e->forcedshader?e->forcedshader:skin->shader;
		if (shader)
		{
			b = BE_GetTempBatch();
			if (!b)
				break;

			b->buildmeshes = R_GAlias_DrawBatch;
			b->ent = e;
			b->mesh = NULL;
			b->firstmesh = 0;
			b->meshes = 1;
			b->skin = skin;
			b->texture = NULL;
			b->shader = shader;
			b->lightmap = -1;
			b->surf_first = surfnum;
			b->flags = 0;
			sort = shader->sort;
			if (e->flags & Q2RF_ADDITIVE)
			{
				b->flags |= BEF_FORCEADDITIVE;
				if (sort < SHADER_SORT_ADDITIVE)
					sort = SHADER_SORT_ADDITIVE;
			}
			if (e->flags & Q2RF_TRANSLUCENT)
			{
				b->flags |= BEF_FORCETRANSPARENT;
				if (sort < SHADER_SORT_BLEND)
					sort = SHADER_SORT_BLEND;
			}
			if (e->flags & RF_NODEPTHTEST)
			{
				b->flags |= BEF_FORCENODEPTH;
				if (sort < SHADER_SORT_NEAREST)
					sort = SHADER_SORT_NEAREST;
			}
			if (e->flags & RF_NOSHADOW)
				b->flags |= BEF_NOSHADOWS;
			b->vbo = 0;
			b->next = batches[sort];
			batches[sort] = b;
		}
	}
}

#if 0
void R_Sprite_GenerateBatches(entity_t *e, batch_t **batches)
{
	galiasinfo_t *inf;
	model_t *clmodel;
	shader_t *shader;
	batch_t *b;
	int surfnum;

	texnums_t *skin;

	if (r_refdef.externalview && e->flags & Q2RF_WEAPONMODEL)
		return;

	clmodel = e->model;

	if (!(e->flags & Q2RF_WEAPONMODEL))
	{
		if (R_CullEntityBox (e, clmodel->mins, clmodel->maxs))
			return;
#ifdef RTLIGHTS
		if (BE_LightCullModel(e->origin, clmodel))
			return;
	}
	else
	{
		if (BE_LightCullModel(r_origin, clmodel))
			return;
#endif
	}

	if (clmodel->tainted)
	{
		if (!ruleset_allow_modified_eyes.ival && !strcmp(clmodel->name, "progs/eyes.mdl"))
			return;
	}

	inf = RMod_Extradata (clmodel);

	if (!e->model || e->forcedshader)
	{
		//fixme
		return;
	}
	else
	{
		frame = R_GetSpriteFrame (e);
		psprite = e->model->cache.data;
		sprtype = psprite->type;
		shader = frame->shader;
	}

	if (shader)
	{
		b = BE_GetTempBatch();
		if (!b)
			break;

		b->buildmeshes = R_Sprite_DrawBatch;
		b->ent = e;
		b->mesh = NULL;
		b->firstmesh = 0;
		b->meshes = 1;
		b->skin = frame-;
		b->texture = NULL;
		b->shader = frame->shader;
		b->lightmap = -1;
		b->surf_first = surfnum;
		b->flags = 0;
		b->vbo = 0;
		b->next = batches[shader->sort];
		batches[shader->sort] = b;
	}
}
#endif

//returns the rotated offset of the two points in result
void RotateLightVector(const vec3_t *axis, const vec3_t origin, const vec3_t lightpoint, vec3_t result)
{
	vec3_t offs;

	offs[0] = lightpoint[0] - origin[0];
	offs[1] = lightpoint[1] - origin[1];
	offs[2] = lightpoint[2] - origin[2];

	result[0] = DotProduct (offs, axis[0]);
	result[1] = DotProduct (offs, axis[1]);
	result[2] = DotProduct (offs, axis[2]);
}

#if defined(RTLIGHTS) && defined(GLQUAKE)
void GL_LightMesh (mesh_t *mesh, vec3_t lightpos, vec3_t colours, float radius)
{
	vec3_t dir;
	int i;
	float dot, d, f, a;

	vecV_t *xyz = mesh->xyz_array;
	vec3_t *normals = mesh->normals_array;
	vec4_t *out = mesh->colors4f_array;

	if (!out)
		return;	//urm..

	if (normals)
	{
		for (i = 0; i < mesh->numvertexes; i++)
		{
			VectorSubtract(lightpos, xyz[i], dir);
			dot = DotProduct(dir, normals[i]);
			if (dot > 0)
			{
				d = DotProduct(dir, dir)/radius;
				a = 1/d;
				if (a>0)
				{
					a *= dot/sqrt(d);
					f = a*colours[0];
					out[i][0] = f;

					f = a*colours[1];
					out[i][1] = f;

					f = a*colours[2];
					out[i][2] = f;
				}
				else
				{
					out[i][0] = 0;
					out[i][1] = 0;
					out[i][2] = 0;
				}
			}
			else
			{
				out[i][0] = 0;
				out[i][1] = 0;
				out[i][2] = 0;
			}
			out[i][3] = 1;
		}
	}
	else
	{
		for (i = 0; i < mesh->numvertexes; i++)
		{
			VectorSubtract(lightpos, xyz[i], dir);
			out[i][0] = colours[0];
			out[i][1] = colours[1];
			out[i][2] = colours[2];
			out[i][3] = 1;
		}
	}
}

//courtesy of DP
void R_BuildBumpVectors(const float *v0, const float *v1, const float *v2, const float *tc0, const float *tc1, const float *tc2, float *svector3f, float *tvector3f, float *normal3f)
{
	float f, tangentcross[3], v10[3], v20[3], tc10[2], tc20[2];
	// 79 add/sub/negate/multiply (1 cycle), 1 compare (3 cycle?), total cycles not counting load/store/exchange roughly 82 cycles
	// 6 add, 28 subtract, 39 multiply, 1 compare, 50% chance of 6 negates

	// 6 multiply, 9 subtract
	VectorSubtract(v1, v0, v10);
	VectorSubtract(v2, v0, v20);
	normal3f[0] = v10[1] * v20[2] - v10[2] * v20[1];
	normal3f[1] = v10[2] * v20[0] - v10[0] * v20[2];
	normal3f[2] = v10[0] * v20[1] - v10[1] * v20[0];
	// 12 multiply, 10 subtract
	tc10[1] = tc1[1] - tc0[1];
	tc20[1] = tc2[1] - tc0[1];
	svector3f[0] = tc10[1] * v20[0] - tc20[1] * v10[0];
	svector3f[1] = tc10[1] * v20[1] - tc20[1] * v10[1];
	svector3f[2] = tc10[1] * v20[2] - tc20[1] * v10[2];
	tc10[0] = tc1[0] - tc0[0];
	tc20[0] = tc2[0] - tc0[0];
	tvector3f[0] = tc10[0] * v20[0] - tc20[0] * v10[0];
	tvector3f[1] = tc10[0] * v20[1] - tc20[0] * v10[1];
	tvector3f[2] = tc10[0] * v20[2] - tc20[0] * v10[2];
	// 12 multiply, 4 add, 6 subtract
	f = DotProduct(svector3f, normal3f);
	svector3f[0] -= f * normal3f[0];
	svector3f[1] -= f * normal3f[1];
	svector3f[2] -= f * normal3f[2];
	f = DotProduct(tvector3f, normal3f);
	tvector3f[0] -= f * normal3f[0];
	tvector3f[1] -= f * normal3f[1];
	tvector3f[2] -= f * normal3f[2];
	// if texture is mapped the wrong way (counterclockwise), the tangents
	// have to be flipped, this is detected by calculating a normal from the
	// two tangents, and seeing if it is opposite the surface normal
	// 9 multiply, 2 add, 3 subtract, 1 compare, 50% chance of: 6 negates
	CrossProduct(tvector3f, svector3f, tangentcross);
	if (DotProduct(tangentcross, normal3f) < 0)
	{
		VectorNegate(svector3f, svector3f);
		VectorNegate(tvector3f, tvector3f);
	}
}

//courtesy of DP
void R_AliasGenerateTextureVectors(mesh_t *mesh, float *normal3f, float *svector3f, float *tvector3f)
{
	int i;
	float sdir[3], tdir[3], normal[3], *v;
	index_t *e;
	float *vertex3f = (float*)mesh->xyz_array;
	float *texcoord2f = (float*)mesh->st_array;
	// clear the vectors
//	if (svector3f)
		memset(svector3f, 0, mesh->numvertexes * sizeof(float[3]));
//	if (tvector3f)
		memset(tvector3f, 0, mesh->numvertexes * sizeof(float[3]));
//	if (normal3f)
		memset(normal3f, 0, mesh->numvertexes * sizeof(float[3]));
	// process each vertex of each triangle and accumulate the results
	for (e = mesh->indexes; e < mesh->indexes+mesh->numindexes; e += 3)
	{
		R_BuildBumpVectors(vertex3f + e[0] * 3, vertex3f + e[1] * 3, vertex3f + e[2] * 3, texcoord2f + e[0] * 2, texcoord2f + e[1] * 2, texcoord2f + e[2] * 2, sdir, tdir, normal);
//		if (!areaweighting)
//		{
//			VectorNormalize(sdir);
//			VectorNormalize(tdir);
//			VectorNormalize(normal);
//		}
//		if (svector3f)
			for (i = 0;i < 3;i++)
				VectorAdd(svector3f + e[i]*3, sdir, svector3f + e[i]*3);
//		if (tvector3f)
			for (i = 0;i < 3;i++)
				VectorAdd(tvector3f + e[i]*3, tdir, tvector3f + e[i]*3);
//		if (normal3f)
			for (i = 0;i < 3;i++)
				VectorAdd(normal3f + e[i]*3, normal, normal3f + e[i]*3);
	}
	// now we could divide the vectors by the number of averaged values on
	// each vertex...  but instead normalize them
	// 4 assignments, 1 divide, 1 sqrt, 2 adds, 6 multiplies
	if (svector3f)
		for (i = 0, v = svector3f;i < mesh->numvertexes;i++, v += 3)
			VectorNormalize(v);
	// 4 assignments, 1 divide, 1 sqrt, 2 adds, 6 multiplies
	if (tvector3f)
		for (i = 0, v = tvector3f;i < mesh->numvertexes;i++, v += 3)
			VectorNormalize(v);
	// 4 assignments, 1 divide, 1 sqrt, 2 adds, 6 multiplies
	if (normal3f)
		for (i = 0, v = normal3f;i < mesh->numvertexes;i++, v += 3)
			VectorNormalize(v);

}


void R_AliasGenerateVertexLightDirs(mesh_t *mesh, vec3_t lightdir, vec3_t *results, vec3_t *normal3f, vec3_t *svector3f, vec3_t *tvector3f)
{
	int i;
	R_AliasGenerateTextureVectors(mesh, (float*)normal3f, (float*)svector3f, (float*)tvector3f);

	for (i = 0; i < mesh->numvertexes; i++)
	{
		results[i][0] = -DotProduct(lightdir, tvector3f[i]);
		results[i][1] = -DotProduct(lightdir, svector3f[i]);
		results[i][2] = -DotProduct(lightdir, normal3f[i]);
	}
}

//FIXME: Be less agressive.
//This function will have to be called twice (for geforce cards), with the same data, so do the building once and rendering twice.
void R_DrawGAliasShadowVolume(entity_t *e, vec3_t lightpos, float radius)
{
	model_t *clmodel = e->model;
	galiasinfo_t *inf;
	mesh_t mesh;
	vec3_t lightorg;

	if (clmodel->engineflags & (MDLF_FLAME | MDLF_BOLT))
		return;
	if (r_noaliasshadows.ival)
		return;

//	if (e->shaderRGBAf[3] < 0.5)
//		return;

	RotateLightVector((void *)e->axis, e->origin, lightpos, lightorg);

	if (Length(lightorg) > radius + clmodel->radius)
		return;

	BE_SelectEntity(e);

	inf = RMod_Extradata (clmodel);
	while(inf)
	{
		if (inf->ofs_trineighbours)
		{
			Alias_GAliasBuildMesh(&mesh, inf, e, 1, true);
			R_CalcFacing(&mesh, lightorg);
			R_ProjectShadowVolume(&mesh, lightorg);
			R_DrawShadowVolume(&mesh);
		}

		if (inf->nextsurf)
			inf = (galiasinfo_t*)((char *)inf + inf->nextsurf);
		else
			inf = NULL;
	}
}
#endif





#if 0
static int R_FindTriangleWithEdge ( index_t *indexes, int numtris, index_t start, index_t end, int ignore)
{
	int i;
	int match, count;

	count = 0;
	match = -1;

	for (i = 0; i < numtris; i++, indexes += 3)
	{
		if ( (indexes[0] == start && indexes[1] == end)
			|| (indexes[1] == start && indexes[2] == end)
			|| (indexes[2] == start && indexes[0] == end) ) {
			if (i != ignore)
				match = i;
			count++;
		} else if ( (indexes[1] == start && indexes[0] == end)
			|| (indexes[2] == start && indexes[1] == end)
			|| (indexes[0] == start && indexes[2] == end) ) {
			count++;
		}
	}

	// detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}
#endif

#if 0
static void R_BuildTriangleNeighbours ( int *neighbours, index_t *indexes, int numtris )
{
	int i, *n;
	index_t *index;

	for (i = 0, index = indexes, n = neighbours; i < numtris; i++, index += 3, n += 3)
	{
		n[0] = R_FindTriangleWithEdge (indexes, numtris, index[1], index[0], i);
		n[1] = R_FindTriangleWithEdge (indexes, numtris, index[2], index[1], i);
		n[2] = R_FindTriangleWithEdge (indexes, numtris, index[0], index[2], i);
	}
}
#endif




#if 0
void GL_GenerateNormals(float *orgs, float *normals, int *indicies, int numtris, int numverts)
{
	vec3_t d1, d2;
	vec3_t norm;
	int t, i, v1, v2, v3;
	int tricounts[MD2MAX_VERTS];
	vec3_t combined[MD2MAX_VERTS];
	int triremap[MD2MAX_VERTS];
	if (numverts > MD2MAX_VERTS)
		return;	//not an issue, you just loose the normals.

	memset(triremap, 0, numverts*sizeof(triremap[0]));

	v2=0;
	for (i = 0; i < numverts; i++)	//weld points
	{
		for (v1 = 0; v1 < v2; v1++)
		{
			if (orgs[i*3+0] == combined[v1][0] &&
				orgs[i*3+1] == combined[v1][1] &&
				orgs[i*3+2] == combined[v1][2])
			{
				triremap[i] = v1;
				break;
			}
		}
		if (v1 == v2)
		{
			combined[v1][0] = orgs[i*3+0];
			combined[v1][1] = orgs[i*3+1];
			combined[v1][2] = orgs[i*3+2];
			v2++;

			triremap[i] = v1;
		}
	}
	memset(tricounts, 0, v2*sizeof(tricounts[0]));
	memset(combined, 0, v2*sizeof(*combined));

	for (t = 0; t < numtris; t++)
	{
		v1 = triremap[indicies[t*3]];
		v2 = triremap[indicies[t*3+1]];
		v3 = triremap[indicies[t*3+2]];

		VectorSubtract((orgs+v2*3), (orgs+v1*3), d1);
		VectorSubtract((orgs+v3*3), (orgs+v1*3), d2);
		CrossProduct(d1, d2, norm);
		VectorNormalize(norm);

		VectorAdd(norm, combined[v1], combined[v1]);
		VectorAdd(norm, combined[v2], combined[v2]);
		VectorAdd(norm, combined[v3], combined[v3]);

		tricounts[v1]++;
		tricounts[v2]++;
		tricounts[v3]++;
	}

	for (i = 0; i < numverts; i++)
	{
		if (tricounts[triremap[i]])
		{
			VectorScale(combined[triremap[i]], 1.0f/tricounts[triremap[i]], normals+i*3);
		}
	}
}
#endif
#endif







qboolean BE_ShouldDraw(entity_t *e)
{
	if (!r_refdef.externalview && (e->externalmodelview & (1<<r_refdef.currentplayernum)))
		return false;
	if (!Cam_DrawPlayer(r_refdef.currentplayernum, e->keynum-1))
		return false;
	return true;
}

#ifdef Q3CLIENT
//q3 lightning gun
static void R_DB_LightningBeam(batch_t *batch)
{
	entity_t *e = batch->ent;
	vec3_t v;
	vec3_t dir, cr;
	float scale = e->scale;
	float length;

	static vecV_t points[4];
	static vec2_t texcoords[4] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
	static index_t indexarray[6] = {0, 1, 2, 0, 2, 3};
	static vec4_t colors[4];

	static mesh_t mesh;
	static mesh_t *meshptr = &mesh;

	if (batch->ent == &r_worldentity)
	{
		mesh.numindexes = 0;
		mesh.numindexes = 0;
		return;
	}

	scale *= -10;
	if (!scale)
		scale = 10;


	VectorSubtract(e->origin, e->oldorigin, dir);
	length = Length(dir);

	//this seems to be about right.
	texcoords[2][0] = length/128;
	texcoords[3][0] = length/128;

	VectorSubtract(r_refdef.vieworg, e->origin, v);
	CrossProduct(v, dir, cr);
	VectorNormalize(cr);

	VectorMA(e->origin, -scale/2, cr, points[0]);
	VectorMA(e->origin, scale/2, cr, points[1]);

	VectorSubtract(r_refdef.vieworg, e->oldorigin, v);
	CrossProduct(v, dir, cr);
	VectorNormalize(cr);

	VectorMA(e->oldorigin, scale/2, cr, points[2]);
	VectorMA(e->oldorigin, -scale/2, cr, points[3]);

	/*this is actually meant to be 4 separate quads at 45 degrees from each other*/

	Vector4Copy(e->shaderRGBAf, colors[0]);
	Vector4Copy(e->shaderRGBAf, colors[1]);
	Vector4Copy(e->shaderRGBAf, colors[2]);
	Vector4Copy(e->shaderRGBAf, colors[3]);

	batch->ent = &r_worldentity;
	batch->mesh = &meshptr;

	memset(&mesh, 0, sizeof(mesh));
	mesh.vbofirstelement = 0;
	mesh.vbofirstvert = 0;
	mesh.xyz_array = points;
	mesh.indexes = indexarray;
	mesh.numindexes = sizeof(indexarray)/sizeof(indexarray[0]);
	mesh.colors4f_array = (vec4_t*)colors;
	mesh.lmst_array = NULL;
	mesh.normals_array = NULL;
	mesh.numvertexes = 4;
	mesh.st_array = texcoords;
}
//q3 railgun beam
static void R_DB_RailgunBeam(batch_t *batch)
{
	entity_t *e = batch->ent;
	vec3_t v;
	vec3_t dir, cr;
	float scale = e->scale;
	float length;

	static mesh_t mesh;
	static mesh_t *meshptr = &mesh;
	static vecV_t points[4];
	static vec2_t texcoords[4] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
	static index_t indexarray[6] = {0, 1, 2, 0, 2, 3};
	static vec4_t colors[4];

	if (batch->ent == &r_worldentity)
	{
		mesh.numindexes = 0;
		mesh.numindexes = 0;
		return;
	}

	if (!e->forcedshader)
		return;

	if (!scale)
		scale = 10;


	VectorSubtract(e->origin, e->oldorigin, dir);
	length = Length(dir);

	//this seems to be about right.
	texcoords[2][0] = length/128;
	texcoords[3][0] = length/128;

	VectorSubtract(r_refdef.vieworg, e->origin, v);
	CrossProduct(v, dir, cr);
	VectorNormalize(cr);

	VectorMA(e->origin, -scale/2, cr, points[0]);
	VectorMA(e->origin, scale/2, cr, points[1]);

	VectorSubtract(r_refdef.vieworg, e->oldorigin, v);
	CrossProduct(v, dir, cr);
	VectorNormalize(cr);

	VectorMA(e->oldorigin, scale/2, cr, points[2]);
	VectorMA(e->oldorigin, -scale/2, cr, points[3]);

	Vector4Copy(e->shaderRGBAf, colors[0]);
	Vector4Copy(e->shaderRGBAf, colors[1]);
	Vector4Copy(e->shaderRGBAf, colors[2]);
	Vector4Copy(e->shaderRGBAf, colors[3]);

	batch->ent = &r_worldentity;
	batch->mesh = &meshptr;

	memset(&mesh, 0, sizeof(mesh));
	mesh.vbofirstelement = 0;
	mesh.vbofirstvert = 0;
	mesh.xyz_array = points;
	mesh.indexes = indexarray;
	mesh.numindexes = sizeof(indexarray)/sizeof(indexarray[0]);
	mesh.colors4f_array = (vec4_t*)colors;
	mesh.lmst_array = NULL;
	mesh.normals_array = NULL;
	mesh.numvertexes = 4;
	mesh.st_array = texcoords;

}
#endif
static void R_DB_Sprite(batch_t *batch)
{
	entity_t *e = batch->ent;
	vec3_t	point;
	mspriteframe_t	*frame, genframe;
	vec3_t		forward, right, up;
	msprite_t		*psprite;
	vec3_t sprorigin;
	unsigned int sprtype;

	static vec2_t texcoords[4]={{0, 1},{0,0},{1,0},{1,1}};
	static index_t indexes[6] = {0, 1, 2, 0, 2, 3};
	static vecV_t vertcoords[4];
	static avec4_t colours[4];
	static mesh_t mesh;
	static mesh_t *meshptr = &mesh;

	if (batch->ent == &r_worldentity)
	{
		mesh.numindexes = 0;
		mesh.numindexes = 0;
		return;
	}

	if (e->flags & Q2RF_WEAPONMODEL && r_refdef.currentplayernum >= 0)
	{
		sprorigin[0] = cl.viewent[r_refdef.currentplayernum].origin[0];
		sprorigin[1] = cl.viewent[r_refdef.currentplayernum].origin[1];
		sprorigin[2] = cl.viewent[r_refdef.currentplayernum].origin[2];
		VectorMA(sprorigin, e->origin[0], cl.viewent[r_refdef.currentplayernum].axis[0], sprorigin);
		VectorMA(sprorigin, e->origin[1], cl.viewent[r_refdef.currentplayernum].axis[1], sprorigin);
		VectorMA(sprorigin, e->origin[2], cl.viewent[r_refdef.currentplayernum].axis[2], sprorigin);
		VectorMA(sprorigin, 12, vpn, sprorigin);

		batch->flags |= BEF_FORCENODEPTH;
	}
	else
		VectorCopy(e->origin, sprorigin);

	if (!e->model || e->forcedshader)
	{
		genframe.shader = e->forcedshader;
		genframe.up = genframe.left = -1;
		genframe.down = genframe.right = 1;
		sprtype = SPR_VP_PARALLEL;
		frame = &genframe;
	}
	else
	{
		// don't even bother culling, because it's just a single
		// polygon without a surface cache
		frame = R_GetSpriteFrame (e);
		psprite = e->model->cache.data;
		sprtype = psprite->type;
	}
	if (!frame->shader)
		return;

	switch(sprtype)
	{
	case SPR_ORIENTED:
		// bullet marks on walls
		AngleVectors (e->angles, forward, right, up);
		break;

	case SPR_FACING_UPRIGHT:
		up[0] = 0;up[1] = 0;up[2]=1;
		right[0] = sprorigin[1] - r_origin[1];
		right[1] = -(sprorigin[0] - r_origin[0]);
		right[2] = 0;
		VectorNormalize (right);
		break;
	case SPR_VP_PARALLEL_UPRIGHT:
		up[0] = 0;up[1] = 0;up[2]=1;
		VectorCopy (vright, right);
		break;

	default:
	case SPR_VP_PARALLEL:
		//normal sprite
		VectorCopy(vup, up);
		VectorCopy(vright, right);
		break;
	}
	up[0]*=e->scale;
	up[1]*=e->scale;
	up[2]*=e->scale;
	right[0]*=e->scale;
	right[1]*=e->scale;
	right[2]*=e->scale;

	if (e->shaderRGBAf[0] > 1)
		e->shaderRGBAf[0] = 1;
	if (e->shaderRGBAf[1] > 1)
		e->shaderRGBAf[1] = 1;
	if (e->shaderRGBAf[2] > 1)
		e->shaderRGBAf[2] = 1;

	Vector4Copy(e->shaderRGBAf, colours[0]);
	Vector4Copy(e->shaderRGBAf, colours[1]);
	Vector4Copy(e->shaderRGBAf, colours[2]);
	Vector4Copy(e->shaderRGBAf, colours[3]);

	VectorMA (sprorigin, frame->down, up, point);
	VectorMA (point, frame->left, right, vertcoords[0]);

	VectorMA (sprorigin, frame->up, up, point);
	VectorMA (point, frame->left, right, vertcoords[1]);

	VectorMA (sprorigin, frame->up, up, point);
	VectorMA (point, frame->right, right, vertcoords[2]);

	VectorMA (sprorigin, frame->down, up, point);
	VectorMA (point, frame->right, right, vertcoords[3]);

	batch->ent = &r_worldentity;
	batch->mesh = &meshptr;

	memset(&mesh, 0, sizeof(mesh));
	mesh.vbofirstelement = 0;
	mesh.vbofirstvert = 0;
	mesh.xyz_array = vertcoords;
	mesh.indexes = indexes;
	mesh.numindexes = sizeof(indexes)/sizeof(indexes[0]);
	mesh.colors4f_array = colours;
	mesh.lmst_array = NULL;
	mesh.normals_array = NULL;
	mesh.numvertexes = 4;
	mesh.st_array = texcoords;
	mesh.istrifan = true;
}
static void R_Sprite_GenerateBatch(entity_t *e, batch_t **batches, void (*drawfunc)(batch_t *batch))
{
	extern cvar_t gl_blendsprites;
	shader_t *shader = NULL;
	batch_t *b;
	shadersort_t sort;

	if (!e->model || e->model->type != mod_sprite || e->forcedshader)
	{
		shader = e->forcedshader;
		if (!shader)
			shader = R_RegisterShader("q2beam",
				"{\n"
					"{\n"
						"map $whiteimage\n"
						"rgbgen vertex\n"
						"alphagen vertex\n"
						"blendfunc blend\n"
					"}\n"
				"}\n"
				);
	}
	else
	{
		// don't even bother culling, because it's just a single
		// polygon without a surface cache
		shader = R_GetSpriteFrame(e)->shader;
	}

	if (!shader)
		return;

	b = BE_GetTempBatch();
	if (!b)
		return;

	b->flags = 0;
	sort = shader->sort;
	if (e->flags & Q2RF_ADDITIVE)
	{
		b->flags |= BEF_FORCEADDITIVE;
		if (sort < SHADER_SORT_ADDITIVE)
			sort = SHADER_SORT_ADDITIVE;
	}
	if (e->flags & Q2RF_TRANSLUCENT || (gl_blendsprites.ival && drawfunc == R_DB_Sprite))
	{
		b->flags |= BEF_FORCETRANSPARENT;
		if (sort < SHADER_SORT_BLEND)
			sort = SHADER_SORT_BLEND;
	}
	if (e->flags & RF_NODEPTHTEST)
	{
		b->flags |= BEF_FORCENODEPTH;
		if (sort < SHADER_SORT_BANNER)
			sort = SHADER_SORT_BANNER;
	}

	b->buildmeshes = drawfunc;
	b->ent = e;
	b->mesh = NULL;
	b->firstmesh = 0;
	b->meshes = 1;
	b->skin = &shader->defaulttextures;
	b->texture = NULL;
	b->shader = shader;
	b->lightmap = -1;
	b->surf_first = 0;
	b->flags |= BEF_NODLIGHT|BEF_NOSHADOWS;
	b->vbo = 0;
	b->next = batches[sort];
	batches[sort] = b;
}

static void R_DB_Poly(batch_t *batch)
{
	static mesh_t mesh;
	static mesh_t *meshptr = &mesh;
	unsigned int i = batch->surf_first;

	batch->mesh = &meshptr;

	mesh.xyz_array = cl_strisvertv + cl_stris[i].firstvert;
	mesh.st_array = cl_strisvertt + cl_stris[i].firstvert;
	mesh.colors4f_array = cl_strisvertc + cl_stris[i].firstvert;
	mesh.indexes = cl_strisidx + cl_stris[i].firstidx;
	mesh.numindexes = cl_stris[i].numidx;
	mesh.numvertexes = cl_stris[i].numvert;
}
void BE_GenPolyBatches(batch_t **batches)
{
	shader_t *shader = NULL;
	batch_t *b;
	unsigned int i;

	for (i = 0; i < cl_numstris; i++)
	{
		b = BE_GetTempBatch();
		if (!b)
			return;

		shader = cl_stris[i].shader;

		b->buildmeshes = R_DB_Poly;
		b->ent = &r_worldentity;
		b->mesh = NULL;
		b->firstmesh = 0;
		b->meshes = 1;
		b->skin = &shader->defaulttextures;
		b->texture = NULL;
		b->shader = shader;
		b->lightmap = -1;
		b->surf_first = i;
		b->flags = BEF_NODLIGHT|BEF_NOSHADOWS;
		b->vbo = 0;
		b->next = batches[shader->sort];
		batches[shader->sort] = b;
	}
}

void BE_GenModelBatches(batch_t **batches)
{
	int		i;
	entity_t *ent;

	/*clear the batch list*/
	for (i = 0; i < SHADER_SORT_COUNT; i++)
		batches[i] = NULL;

	if (!r_drawentities.ival)
		return;

#if defined(TERRAIN)
	if (cl.worldmodel && cl.worldmodel->type == mod_heightmap)
		GL_DrawHeightmapModel(batches, &r_worldentity);
#endif

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		ent = &cl_visedicts[i];

		if (!BE_ShouldDraw(ent))
			continue;

		switch(ent->rtype)
		{
		case RT_MODEL:
		default:
			if (!ent->model)
				continue;
			if (ent->model->needload)
				continue;

			if (cl.lerpents && (cls.allow_anyparticles || ent->visframe))	//allowed or static
			{
				if (gl_part_flame.value)
				{
					if (ent->model->engineflags & MDLF_ENGULPHS)
						continue;
				}
			}

			if (ent->model->engineflags & MDLF_NOTREPLACEMENTS)
			{
				if (ent->model->fromgame != fg_quake || ent->model->type != mod_alias)
					if (!ruleset_allow_sensative_texture_replacements.value)
						continue;
			}

			switch(ent->model->type)
			{
			case mod_brush:
				if (r_drawentities.ival == 2)
					continue;
				Surf_GenBrushBatches(batches, ent);
				break;
			case mod_alias:
				if (r_drawentities.ival == 3)
					continue;
				R_GAlias_GenerateBatches(ent, batches);
				break;
			case mod_sprite:
				R_Sprite_GenerateBatch(ent, batches, R_DB_Sprite);
				break;
			// warning: enumeration value �mod_*� not handled in switch
			case mod_dummy:
			case mod_halflife:
			case mod_heightmap:
				break;
			}
			break;
		case RT_SPRITE:
			R_Sprite_GenerateBatch(ent, batches, R_DB_Sprite);
			break;

#ifdef Q3CLIENT
		case RT_BEAM:
		case RT_RAIL_RINGS:
		case RT_LIGHTNING:
			R_Sprite_GenerateBatch(ent, batches, R_DB_LightningBeam);
			continue;
		case RT_RAIL_CORE:
			R_Sprite_GenerateBatch(ent, batches, R_DB_RailgunBeam);
			continue;
#endif

		case RT_POLY:
			/*not implemented*/
			break;
		case RT_PORTALSURFACE:
			/*nothing*/
			break;
		}
	}

	if (cl_numstris)
		BE_GenPolyBatches(batches);
}

#endif	// defined(GLQUAKE)