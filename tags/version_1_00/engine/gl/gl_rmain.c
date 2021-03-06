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
// r_main.c

#include "quakedef.h"

#ifdef GLQUAKE
#include "glquake.h"
#include "renderque.h"
#include "shader.h"
#include "gl_draw.h"

void R_RenderBrushPoly (msurface_t *fa);

#define PROJECTION_DISTANCE			200
#define MAX_STENCIL_ENTS			128

extern int		gl_canstencil;

FTEPFNGLCOMPRESSEDTEXIMAGE2DARBPROC qglCompressedTexImage2DARB;
FTEPFNGLGETCOMPRESSEDTEXIMAGEARBPROC qglGetCompressedTexImageARB;

entity_t	r_worldentity;

vec3_t		modelorg, r_entorigin;

int			r_visframecount;	// bumped when going to a new PVS
extern int			r_framecount;		// used for dlight push checking

float		r_wateralphaval;	//allowed or not...

//mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;
mleaf_t		*r_viewleaf2, *r_oldviewleaf2;
int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

texture_t	*r_notexture_mip;

cvar_t	r_norefresh = SCVAR("r_norefresh","0");

extern cvar_t	gl_part_flame;
extern cvar_t	r_bloom;

cvar_t	gl_affinemodels = SCVAR("gl_affinemodels","0");
cvar_t	gl_reporttjunctions = SCVAR("gl_reporttjunctions","0");
cvar_t	gl_finish = SCVAR("gl_finish","0");
cvar_t	gl_dither = SCVAR("gl_dither", "1");

extern cvar_t	gl_screenangle;

extern cvar_t	gl_mindist;

extern cvar_t	ffov;

extern cvar_t	gl_motionblur;
extern cvar_t	gl_motionblurscale;

extern cvar_t gl_ati_truform;
extern cvar_t gl_ati_truform_type;
extern cvar_t gl_ati_truform_tesselation;

extern cvar_t gl_blendsprites;

#ifdef R_XFLIP
cvar_t	r_xflip = SCVAR("leftisright", "0");
#endif

extern	cvar_t	scr_fov;

shader_t *scenepp_waterwarp;

// post processing stuff
texid_t sceneblur_texture;
texid_t scenepp_texture_warp;
texid_t scenepp_texture_edge;

int scenepp_mt_program;
int scenepp_mt_parm_texture0i;
int scenepp_mt_parm_colorf;
int scenepp_mt_parm_inverti;

texid_t scenepp_fisheye_texture;
int scenepp_fisheye_program;
int scenepp_fisheye_parm_fov;
int scenepp_panorama_program;
int scenepp_panorama_parm_fov;

// KrimZon - init post processing - called in GL_CheckExtensions, when they're called
// I put it here so that only this file need be changed when messing with the post
// processing shaders
void GL_InitSceneProcessingShaders_WaterWarp (void)
{
	/*
	inputs:
	texcoords: edge points
	coords: vertex coords (duh)
	time
	ampscale (cvar = r_waterwarp)

	use ifs instead of an edge map?
	*/
	if (gl_config.arb_shader_objects)
	{
		scenepp_waterwarp = R_RegisterShader("waterwarp",
			"{\n"
			"glslprogram\n"
			"{\n"
			"#ifdef VERTEX_SHADER\n"
			"\
			attribute vec2 v_texcoord;\
			varying vec2 v_stc;\
			varying vec2 v_warp;\
			varying vec2 v_edge;\
			uniform float e_time;\
			void main (void)\
			{\
				gl_Position = ftetransform();\
				v_stc = (1.0+(gl_Position.xy / gl_Position.w))/2.0;\
				v_warp.s = e_time * 0.25 + v_texcoord.s;\
				v_warp.t = e_time * 0.25 + v_texcoord.t;\
				v_edge = v_texcoord.xy;\
			}\
			\n"
			"#endif\n"
			"#ifdef FRAGMENT_SHADER\n"
			"\
			varying vec2 v_stc;\
			varying vec2 v_warp;\
			varying vec2 v_edge;\
			uniform sampler2D s_t0;\
			uniform sampler2D s_t1;\
			uniform sampler2D s_t2;\
			uniform float ampscale;\
			uniform vec3 rendertexturescale;\
			void main (void)\
			{\
				float amptemp;\
				vec3 edge;\
				edge = texture2D( s_t2, v_edge ).rgb;\
				amptemp = (0.010 / 0.625) * ampscale * edge.x;\
				vec3 offset;\
				offset = texture2D( s_t1, v_warp ).rgb;\
				offset.x = (offset.x - 0.5) * 2.0;\
				offset.y = (offset.y - 0.5) * 2.0;\
				vec2 temp;\
				temp.x = v_stc.x + offset.x * amptemp;\
				temp.y = v_stc.y + offset.y * amptemp;\
				gl_FragColor = texture2D( s_t0, temp*rendertexturescale.st );\
			}\
			\n"
			"#endif\n"
			"}\n"
			"param cvarf r_waterwarp ampscale\n"
			"param rendertexturescale rendertexturescale\n"
			"{\n"
			"map $currentrender\n"
			"}\n"
			"{\n"
			"map $upperoverlay\n"
			"}\n"
			"{\n"
			"map $loweroverlay\n"
			"}\n"
			"}\n"
			);
		scenepp_waterwarp->defaulttextures.upperoverlay = scenepp_texture_warp;
		scenepp_waterwarp->defaulttextures.loweroverlay = scenepp_texture_edge;
	}
}

void GL_InitFisheyeFov(void)
{
	char *vshader = "\
		varying vec2 texcoord;\
		void main(void)\
		{\
			texcoord = gl_MultiTexCoord0.xy;\
			gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\
		}";
	char *fisheyefshader = "\
		uniform samplerCube source;\
		varying vec2 texcoord;\
		uniform float fov;\
		void main(void)\
		{\
			vec3 tc;	\
			vec2 d;	\
			vec2 ang;	\
			d = texcoord;	\
			ang.x = sqrt(d.x*d.x+d.y*d.y)*fov;	\
			ang.y = -atan(d.y, d.x);	\
			tc.x = sin(ang.x) * cos(ang.y);	\
			tc.y = sin(ang.x) * sin(ang.y);	\
			tc.z = cos(ang.x);	\
			gl_FragColor = textureCube(source, tc);\
		}";

	char *panoramafshader = "\
		uniform samplerCube source;\
		varying vec2 texcoord;\
		uniform float fov;\
		void main(void)\
		{\
			vec3 tc;	\
			float ang;	\
			ang = texcoord.x*fov;	\
			tc.x = sin(ang);	\
			tc.y = -texcoord.y;	\
			tc.z = cos(ang);	\
			gl_FragColor = textureCube(source, tc);\
		}";

	if (gl_config.gles)
		return;

	scenepp_fisheye_program = GLSlang_CreateProgram("fisheye", "#version 110\n", NULL, vshader, fisheyefshader);
	if (scenepp_fisheye_program)
	{
		GLSlang_UseProgram(scenepp_fisheye_program);
		GLSlang_SetUniform1i(GLSlang_GetUniformLocation(scenepp_fisheye_program, "source"), 0);
		scenepp_fisheye_parm_fov = GLSlang_GetUniformLocation(scenepp_fisheye_program, "fov");
		GLSlang_UseProgram(0);
	}

	scenepp_panorama_program = GLSlang_CreateProgram("panorama", "#version 110\n", NULL, vshader, panoramafshader);
	if (scenepp_panorama_program)
	{
		GLSlang_UseProgram(scenepp_panorama_program);
		GLSlang_SetUniform1i(GLSlang_GetUniformLocation(scenepp_panorama_program, "source"), 0);
		scenepp_panorama_parm_fov = GLSlang_GetUniformLocation(scenepp_panorama_program, "fov");
		GLSlang_UseProgram(0);
	}
}

void GL_InitSceneProcessingShaders (void)
{
	if (gl_config.arb_shader_objects)
	{
		GL_InitSceneProcessingShaders_WaterWarp();
		GL_InitFisheyeFov();
	}
}

#define PP_WARP_TEX_SIZE 64
#define PP_AMP_TEX_SIZE 64
#define PP_AMP_TEX_BORDER 4
void GL_SetupSceneProcessingTextures (void)
{
	int i, x, y;
	unsigned char pp_warp_tex[PP_WARP_TEX_SIZE*PP_WARP_TEX_SIZE*3];
	unsigned char pp_edge_tex[PP_AMP_TEX_SIZE*PP_AMP_TEX_SIZE*3];

	scenepp_fisheye_texture = r_nulltex;

	sceneblur_texture = GL_AllocNewTexture(0, 0);

	if (!gl_config.arb_shader_objects)
		return;

	scenepp_texture_warp = GL_AllocNewTexture(0, 0);
	scenepp_texture_edge = GL_AllocNewTexture(0, 0);

	// init warp texture - this specifies offset in
	for (y=0; y<PP_WARP_TEX_SIZE; y++)
	{
		for (x=0; x<PP_WARP_TEX_SIZE; x++)
		{
			float fx, fy;

			i = (x + y*PP_WARP_TEX_SIZE) * 3;

			fx = sin(((double)y / PP_WARP_TEX_SIZE) * M_PI * 2);
			fy = cos(((double)x / PP_WARP_TEX_SIZE) * M_PI * 2);

			pp_warp_tex[i  ] = (fx+1.0f)*127.0f;
			pp_warp_tex[i+1] = (fy+1.0f)*127.0f;
			pp_warp_tex[i+2] = 0;
		}
	}

	GL_MTBind(0, GL_TEXTURE_2D, scenepp_texture_warp);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PP_WARP_TEX_SIZE, PP_WARP_TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, pp_warp_tex);

	// TODO: init edge texture - this is ampscale * 2, with ampscale calculated
	// init warp texture - this specifies offset in
	for (y=0; y<PP_AMP_TEX_SIZE; y++)
	{
		for (x=0; x<PP_AMP_TEX_SIZE; x++)
		{
			float fx = 1, fy = 1;

			i = (x + y*PP_AMP_TEX_SIZE) * 3;

			if (x < PP_AMP_TEX_BORDER)
			{
				fx = (float)x / PP_AMP_TEX_BORDER;
			}
			if (x > PP_AMP_TEX_SIZE - PP_AMP_TEX_BORDER)
			{
				fx = (PP_AMP_TEX_SIZE - (float)x) / PP_AMP_TEX_BORDER;
			}

			if (y < PP_AMP_TEX_BORDER)
			{
				fy = (float)y / PP_AMP_TEX_BORDER;
			}
			if (y > PP_AMP_TEX_SIZE - PP_AMP_TEX_BORDER)
			{
				fy = (PP_AMP_TEX_SIZE - (float)y) / PP_AMP_TEX_BORDER;
			}

			if (fx < fy)
			{
				fy = fx;
			}

			pp_edge_tex[i  ] = fy * 255;
			pp_edge_tex[i+1] = 0;
			pp_edge_tex[i+2] = 0;
		}
	}

	GL_MTBind(0, GL_TEXTURE_2D, scenepp_texture_edge);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PP_WARP_TEX_SIZE, PP_WARP_TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, pp_edge_tex);
}

void R_RotateForEntity (float *modelview, const entity_t *e, const model_t *mod)
{
	float m[16];

	m[0] = e->axis[0][0];
	m[1] = e->axis[0][1];
	m[2] = e->axis[0][2];
	m[3] = 0;

	m[4] = e->axis[1][0];
	m[5] = e->axis[1][1];
	m[6] = e->axis[1][2];
	m[7] = 0;

	m[8] = e->axis[2][0];
	m[9] = e->axis[2][1];
	m[10] = e->axis[2][2];
	m[11] = 0;

	m[12] = e->origin[0];
	m[13] = e->origin[1];
	m[14] = e->origin[2];
	m[15] = 1;

	if (e->scale != 1 && e->scale != 0)	//hexen 2 stuff
	{
		float z;
		float escale;
		escale = e->scale;
		switch(e->drawflags&SCALE_TYPE_MASKIN)
		{
		default:
		case SCALE_TYPE_UNIFORM:
			VectorScale((m+0), escale, (m+0));
			VectorScale((m+4), escale, (m+4));
			VectorScale((m+8), escale, (m+8));
			break;
		case SCALE_TYPE_XYONLY:
			VectorScale((m+0), escale, (m+0));
			VectorScale((m+4), escale, (m+4));
			break;
		case SCALE_TYPE_ZONLY:
			VectorScale((m+8), escale, (m+8));
			break;
		}
		if (mod && (e->drawflags&SCALE_TYPE_MASKIN) != SCALE_TYPE_XYONLY)
		{
			switch(e->drawflags&SCALE_ORIGIN_MASKIN)
			{
			case SCALE_ORIGIN_CENTER:
				z = ((mod->maxs[2] + mod->mins[2]) * (1-escale))/2;
				VectorMA((m+12), z, e->axis[2], (m+12));
				break;
			case SCALE_ORIGIN_BOTTOM:
				VectorMA((m+12), mod->mins[2]*(1-escale), e->axis[2], (m+12));
				break;
			case SCALE_ORIGIN_TOP:
				VectorMA((m+12), -mod->maxs[2], e->axis[2], (m+12));
				break;
			}
		}
	}
	else if (mod && !strcmp(mod->name, "progs/eyes.mdl"))
	{
		/*resize eyes, to make them easier to see*/
		m[14] -= (22 + 8);
		VectorScale((m+0), 2, (m+0));
		VectorScale((m+4), 2, (m+4));
		VectorScale((m+8), 2, (m+8));
	}
	if (mod && !ruleset_allow_larger_models.ival && mod->clampscale != 1)
	{	//possibly this should be on a per-frame basis, but that's a real pain to do
		Con_DPrintf("Rescaling %s by %f\n", mod->name, mod->clampscale);
		VectorScale((m+0), mod->clampscale, (m+0));
		VectorScale((m+4), mod->clampscale, (m+4));
		VectorScale((m+8), mod->clampscale, (m+8));
	}

	if (e->flags & Q2RF_WEAPONMODEL && r_refdef.currentplayernum>=0)
	{
		/*FIXME: no bob*/
		float simpleview[16];
		vec3_t ang;
		ang[0] = 0;
		ang[1] = 0;
		ang[2] = gl_screenangle.value;
		Matrix4_ModelViewMatrix(simpleview, ang, vec3_origin);
		Matrix4_Multiply(simpleview, m, modelview);
	}
	else
	{
		Matrix4_Multiply(r_refdef.m_view, m, modelview);
	}
}

//==================================================================================

/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	int		x, x2, y2, y, w, h;
	vec3_t newa;

	float fov_x, fov_y;

	if (!r_refdef.recurse)
	{
		AngleVectors (r_refdef.viewangles, vpn, vright, vup);
		VectorCopy (r_refdef.vieworg, r_origin);

		//
		// set up viewpoint
		//
		x = r_refdef.vrect.x * vid.pixelwidth/(int)vid.width;
		x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * vid.pixelwidth/(int)vid.width;
		y = (vid.height-r_refdef.vrect.y) * vid.pixelheight/(int)vid.height;
		y2 = ((int)vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * (int)vid.pixelheight/(int)vid.height;

		// fudge around because of frac screen scale
		if (x > 0)
			x--;
		if (x2 < vid.pixelwidth)
			x2++;
		if (y2 < 0)
			y2--;
		if (y < vid.pixelheight)
			y++;

		w = x2 - x;
		h = y - y2;

		r_refdef.pxrect.x = x;
		r_refdef.pxrect.y = y;
		r_refdef.pxrect.width = w;
		r_refdef.pxrect.height = h;

		qglViewport (x, y2, w, h);

		fov_x = r_refdef.fov_x;//+sin(cl.time)*5;
		fov_y = r_refdef.fov_y;//-sin(cl.time+1)*5;

		if (r_waterwarp.value<0 && r_viewleaf && r_viewleaf->contents <= Q1CONTENTS_WATER)
		{
			fov_x *= 1 + (((sin(cl.time * 4.7) + 1) * 0.015) * r_waterwarp.value);
			fov_y *= 1 + (((sin(cl.time * 3.0) + 1) * 0.015) * r_waterwarp.value);
		}

		screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;
		if (r_refdef.useperspective)
		{
			int stencilshadows = 0;
	#ifdef RTLIGHTS
			stencilshadows |= r_shadow_realtime_dlight.ival && r_shadow_realtime_dlight_shadows.ival;
			stencilshadows |= r_shadow_realtime_world.ival && r_shadow_realtime_world_shadows.ival;
	#endif

			if ((!stencilshadows || !gl_canstencil) && gl_maxdist.value>=100)//gl_nv_range_clamp)
			{
		//		yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*180/M_PI;
		//		yfov = (2.0 * tan (scr_fov.value/360*M_PI)) / screenaspect;
		//		yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*(scr_fov.value*2)/M_PI;
		//		MYgluPerspective (yfov,  screenaspect,  4,  4096);

				Matrix4_Projection_Far(r_refdef.m_projection, fov_x, fov_y, gl_mindist.value, gl_maxdist.value);
			}
			else
			{
				Matrix4_Projection_Inf(r_refdef.m_projection, fov_x, fov_y, gl_mindist.value);
			}
		}
		else
		{
			if (gl_maxdist.value>=1)
				Matrix4_Orthographic(r_refdef.m_projection, -fov_x/2, fov_x/2, fov_y/2, -fov_y/2, -gl_maxdist.value, gl_maxdist.value);
			else
				Matrix4_Orthographic(r_refdef.m_projection, 0, r_refdef.vrect.width, 0, r_refdef.vrect.height, -9999, 9999);
		}

		VectorCopy(r_refdef.viewangles, newa);
		newa[0] = r_refdef.viewangles[0];
		newa[1] = r_refdef.viewangles[1];
		newa[2] = r_refdef.viewangles[2] + gl_screenangle.value;
		Matrix4_ModelViewMatrix(r_refdef.m_view, newa, r_refdef.vieworg);
	}

	if (qglLoadMatrixf)
	{
		qglMatrixMode(GL_PROJECTION);
		qglLoadMatrixf(r_refdef.m_projection);

		qglMatrixMode(GL_MODELVIEW);
		qglLoadMatrixf(r_refdef.m_view);
	}

	if (!gl_config.gles)
	{
		if (gl_dither.ival)
		{
			qglEnable(GL_DITHER);
		}
		else
		{
			qglDisable(GL_DITHER);
		}
	}
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene (void)
{
	int tmpvisents = cl_numvisedicts;	/*world rendering is allowed to add additional ents, but we don't want to keep them for recursive views*/
	if (!cl.worldmodel || (!cl.worldmodel->nodes && cl.worldmodel->type != mod_heightmap))
		r_refdef.flags |= Q2RDF_NOWORLDMODEL;

	TRACE(("dbg: calling R_SetupGL\n"));
	R_SetupGL ();

	TRACE(("dbg: calling R_SetFrustrum\n"));
	R_SetFrustum (r_refdef.m_projection, r_refdef.m_view);

	RQ_BeginFrame();

	if (!(r_refdef.flags & Q2RDF_NOWORLDMODEL))
	{
		TRACE(("dbg: calling R_DrawWorld\n"));
		Surf_DrawWorld ();		// adds static entities to the list
	}
	else
		BE_DrawNonWorld();

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

//	R_DrawDecals();

	TRACE(("dbg: calling R_RenderDlights\n"));
	GLR_RenderDlights ();

	if (!(r_refdef.flags & Q2RDF_NOWORLDMODEL))
	{
		TRACE(("dbg: calling R_DrawParticles\n"));
		P_DrawParticles ();
	}
	RQ_RenderBatchClear();

	cl_numvisedicts = tmpvisents;
}
/*generates a new modelview matrix, as well as vpn vectors*/
static void R_MirrorMatrix(plane_t *plane)
{
	float mirror[16];
	float view[16];
	float result[16];

	vec3_t pnorm;
	VectorNegate(plane->normal, pnorm);

	mirror[0] = 1-2*pnorm[0]*pnorm[0];
	mirror[1] = -2*pnorm[0]*pnorm[1];
	mirror[2] = -2*pnorm[0]*pnorm[2];
	mirror[3] = 0;

	mirror[4] = -2*pnorm[1]*pnorm[0];
	mirror[5] = 1-2*pnorm[1]*pnorm[1];
	mirror[6] = -2*pnorm[1]*pnorm[2] ;
	mirror[7] = 0;

	mirror[8]  = -2*pnorm[2]*pnorm[0];
	mirror[9]  = -2*pnorm[2]*pnorm[1];
	mirror[10] = 1-2*pnorm[2]*pnorm[2];
	mirror[11] = 0;

	mirror[12] = -2*pnorm[0]*plane->dist;
	mirror[13] = -2*pnorm[1]*plane->dist;
	mirror[14] = -2*pnorm[2]*plane->dist;
	mirror[15] = 1;

	view[0] = vpn[0];
	view[1] = vpn[1];
	view[2] = vpn[2];
	view[3] = 0;

	view[4] = -vright[0];
	view[5] = -vright[1];
	view[6] = -vright[2];
	view[7] = 0;

	view[8]  = vup[0];
	view[9]  = vup[1];
	view[10] = vup[2];
	view[11] = 0;

	view[12] = r_refdef.vieworg[0];
	view[13] = r_refdef.vieworg[1];
	view[14] = r_refdef.vieworg[2];
	view[15] = 1;

	VectorMA(r_refdef.vieworg, 0.25, plane->normal, r_refdef.pvsorigin);

	Matrix4_Multiply(mirror, view, result);

	vpn[0] = result[0];
	vpn[1] = result[1];
	vpn[2] = result[2];

	vright[0] = -result[4];
	vright[1] = -result[5];
	vright[2] = -result[6];

	vup[0] = result[8];
	vup[1] = result[9];
	vup[2] = result[10];

	r_refdef.vieworg[0] = result[12];
	r_refdef.vieworg[1] = result[13];
	r_refdef.vieworg[2] = result[14];
}
static entity_t *R_NearestPortal(plane_t *plane)
{
	int i;
	entity_t *best = NULL;
	float dist, bestd = 0;
	for (i = 0; i < cl_numvisedicts; i++)
	{
		if (cl_visedicts[i].rtype == RT_PORTALSURFACE)
		{
			dist = DotProduct(cl_visedicts[i].origin, plane->normal)-plane->dist;
			dist = fabs(dist);
			if (dist < 64 && (!best || dist < bestd))
				best = &cl_visedicts[i];
		}
	}
	return best;
}

static void TransformCoord(vec3_t in, vec3_t planea[3], vec3_t planeo, vec3_t viewa[3], vec3_t viewo, vec3_t result)
{
	int		i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	local[0] = in[0] - planeo[0];
	local[1] = in[1] - planeo[1];
	local[2] = in[2] - planeo[2];

	VectorClear(transformed);
	for ( i = 0 ; i < 3 ; i++ )
	{
		d = DotProduct(local, planea[i]);
		VectorMA(transformed, d, viewa[i], transformed);
	}

	result[0] = transformed[0] + viewo[0];
	result[1] = transformed[1] + viewo[1];
	result[2] = transformed[2] + viewo[2];
}
static void TransformDir(vec3_t in, vec3_t planea[3], vec3_t viewa[3], vec3_t result)
{
	int		i;
	float	d;
	vec3_t tmp;

	VectorCopy(in, tmp);

	VectorClear(result);
	for ( i = 0 ; i < 3 ; i++ )
	{
		d = DotProduct(tmp, planea[i]);
		VectorMA(result, d, viewa[i], result);
	}
}
void GLR_DrawPortal(batch_t *batch, batch_t **blist)
{
	entity_t *view;
	GLdouble glplane[4];
	plane_t plane;
	refdef_t oldrefdef;
	mesh_t *mesh = batch->mesh[batch->firstmesh];
	int sort;

	if (r_refdef.recurse)
		return;

	VectorCopy(mesh->normals_array[0], plane.normal);
	plane.dist = DotProduct(mesh->xyz_array[0], plane.normal);

	//if we're too far away from the surface, don't draw anything
	if (batch->shader->flags & SHADER_AGEN_PORTAL)
	{
		/*there's a portal alpha blend on that surface, that fades out after this distance*/
		if (DotProduct(r_refdef.vieworg, plane.normal)-plane.dist > batch->shader->portaldist)
			return;
	}
	//if we're behind it, then also don't draw anything.
	if (DotProduct(r_refdef.vieworg, plane.normal)-plane.dist < 0)
		return;

	view = R_NearestPortal(&plane);
	//if (!view)
	//	return;

	oldrefdef = r_refdef;
	r_refdef.recurse = true;

	r_refdef.externalview = true;

	if (!view || VectorCompare(view->origin, view->oldorigin))
	{
		r_refdef.flipcull ^= true;
		R_MirrorMatrix(&plane);
	}
	else
	{
		float d;
		vec3_t paxis[3], porigin, vaxis[3], vorg;
		void PerpendicularVector( vec3_t dst, const vec3_t src );

		/*calculate where the surface is meant to be*/
		VectorCopy(mesh->normals_array[0], paxis[0]);
		PerpendicularVector(paxis[1], paxis[0]);
		CrossProduct(paxis[0], paxis[1], paxis[2]);
		d = DotProduct(view->origin, plane.normal) - plane.dist;
		VectorMA(view->origin, -d, paxis[0], porigin);

		/*grab the camera origin*/
		VectorNegate(view->axis[0], vaxis[0]);
		VectorNegate(view->axis[1], vaxis[1]);
		VectorCopy(view->axis[2], vaxis[2]);
		VectorCopy(view->oldorigin, vorg);

		VectorCopy(vorg, r_refdef.pvsorigin);

		/*rotate it a bit*/
		RotatePointAroundVector(vaxis[1], vaxis[0], view->axis[1], sin(realtime)*4);
		CrossProduct(vaxis[0], vaxis[1], vaxis[2]);

		TransformCoord(oldrefdef.vieworg, paxis, porigin, vaxis, vorg, r_refdef.vieworg);
		TransformDir(vpn, paxis, vaxis, vpn);
		TransformDir(vright, paxis, vaxis, vright);
		TransformDir(vup, paxis, vaxis, vup);
	}
	Matrix4_ModelViewMatrixFromAxis(r_refdef.m_view, vpn, vright, vup, r_refdef.vieworg);
	VectorAngles(vpn, vup, r_refdef.viewangles);
	VectorCopy(r_refdef.vieworg, r_origin);

/*FIXME: the batch stuff should be done in renderscene*/

	/*fixup the first mesh index*/
	for (sort = 0; sort < SHADER_SORT_COUNT; sort++)
	for (batch = blist[sort]; batch; batch = batch->next)
	{
		batch->firstmesh = batch->meshes;
	}

	GL_CullFace(0);

	/*FIXME: can we get away with stenciling the screen?*/
	/*Add to frustum culling instead of clip planes?*/
	glplane[0] = -plane.normal[0];
	glplane[1] = -plane.normal[1];
	glplane[2] = -plane.normal[2];
	glplane[3] = plane.dist;
	qglClipPlane(GL_CLIP_PLANE0, glplane);
	qglEnable(GL_CLIP_PLANE0);
	R_RenderScene();
	qglDisable(GL_CLIP_PLANE0);

	for (sort = 0; sort < SHADER_SORT_COUNT; sort++)
	for (batch = blist[sort]; batch; batch = batch->next)
	{
		batch->firstmesh = 0;
	}
	r_refdef = oldrefdef;

	/*broken stuff*/
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);
	R_SetFrustum (r_refdef.m_projection, r_refdef.m_view);

	/*put GL back the way it was*/
	qglMatrixMode(GL_PROJECTION);
	qglLoadMatrixf(r_refdef.m_projection);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadMatrixf(r_refdef.m_view);

	GL_CullFace(0);

#ifdef _MSC_VER
#pragma message("warning: there's a bug with rtlights in portals, culling is broken or something. May also be loading the wrong matrix")
#endif
}


/*
=============
R_Clear
=============
*/
int gldepthfunc = GL_LEQUAL;
void R_Clear (void)
{
	/*tbh, this entire function should be in the backend*/
	GL_ForceDepthWritable();
	{
		if (r_clear.ival && !r_secondaryview && !(r_refdef.flags & Q2RDF_NOWORLDMODEL))
			qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			qglClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
		gldepthfunc=GL_LEQUAL;
	}
	if (qglDepthRange)
		qglDepthRange (gldepthmin, gldepthmax);
}

#if 0
void GLR_SetupFog (void)
{
	if (r_viewleaf)// && r_viewleaf->contents != CONTENTS_EMPTY)
	{
		//	static fogcolour;
		float fogcol[4]={0};
		float fogperc;
		float fogdist;

		fogperc=0;
		fogdist=512;
		switch(r_viewleaf->contents)
		{
		case FTECONTENTS_WATER:
			fogcol[0] = 64/255.0;
			fogcol[1] = 128/255.0;
			fogcol[2] = 192/255.0;
			fogperc=0.2;
			fogdist=512;
			break;
		case FTECONTENTS_SLIME:
			fogcol[0] = 32/255.0;
			fogcol[1] = 192/255.0;
			fogcol[2] = 92/255.0;
			fogperc=1;
			fogdist=256;
			break;
		case FTECONTENTS_LAVA:
			fogcol[0] = 192/255.0;
			fogcol[1] = 32/255.0;
			fogcol[2] = 64/255.0;
			fogperc=1;
			fogdist=128;
			break;
		default:
			fogcol[0] = 192/255.0;
			fogcol[1] = 192/255.0;
			fogcol[2] = 192/255.0;
			fogperc=1;
			fogdist=1024;
			break;
		}
		if (fogperc)
		{
			qglFogi(GL_FOG_MODE, GL_LINEAR);
			qglFogfv(GL_FOG_COLOR, fogcol);
			qglFogf(GL_FOG_DENSITY, fogperc);
			qglFogf(GL_FOG_START, 1);
			qglFogf(GL_FOG_END, fogdist);
			qglEnable(GL_FOG);
		}
	}
}
#endif

static void R_RenderMotionBlur(void)
{
	int vwidth = 1, vheight = 1;
	float vs, vt, cs, ct;
#ifdef _MSC_VER
#pragma message("backend fixme")
#endif
	Con_Printf("motionblur is not updated for the backend\n");

	if (gl_config.arb_texture_non_power_of_two)
	{	//we can use any size, supposedly
		vwidth = vid.pixelwidth;
		vheight = vid.pixelheight;
	}
	else
	{	//limit the texture size to square and use padding.
		while (vwidth < vid.pixelwidth)
			vwidth *= 2;
		while (vheight < vid.pixelheight)
			vheight *= 2;
	}

	qglViewport (0, 0, vid.pixelwidth, vid.pixelheight);

	PPL_RevertToKnownState();

	GL_LazyBind(0, GL_TEXTURE_2D, sceneblur_texture, false);

	// go 2d
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity ();
	qglOrtho  (0, vid.pixelwidth, 0, vid.pixelheight, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity ();

	//blend the last frame onto the scene
	//the maths is because our texture is over-sized (must be power of two)
	cs = vs = (float)vid.pixelwidth / vwidth * 0.5;
	ct = vt = (float)vid.pixelheight / vheight * 0.5;
	vs *= gl_motionblurscale.value;
	vt *= gl_motionblurscale.value;

	qglDisable (GL_DEPTH_TEST);
	GL_CullFace(0);
	qglDisable (GL_ALPHA_TEST);
	qglEnable(GL_BLEND);
	qglColor4f(1, 1, 1, gl_motionblur.value);
	qglBegin(GL_QUADS);
	qglTexCoord2f(cs-vs, ct-vt);
	qglVertex2f(0, 0);
	qglTexCoord2f(cs+vs, ct-vt);
	qglVertex2f(vid.pixelwidth, 0);
	qglTexCoord2f(cs+vs, ct+vt);
	qglVertex2f(vid.pixelwidth, vid.pixelheight);
	qglTexCoord2f(cs-vs, ct+vt);
	qglVertex2f(0, vid.pixelheight);
	qglEnd();

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();


	//copy the image into the texture so that we can play with it next frame too!
	qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, vwidth, vheight, 0);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	PPL_RevertToKnownState();
}

#ifdef FISH
/*FIXME: we could use geometry shaders to draw to all 6 faces at once*/
qboolean R_RenderScene_Fish(void)
{
	int cmapsize = 512;
	int i;
	static vec3_t ang[6] =
				{	{0, -90, 0}, {0, 90, 0},
					{90, 0, 0}, {-90, 0, 0},
					{0, 0, 0}, {0, -180, 0}	};
	int order[6] = {4, 0, 1, 5, 3, 2};
	int numsides = 4;
	vec3_t saveang;

	vrect_t vrect;
	vrect_t prect;

	SCR_VRectForPlayer(&vrect, r_refdef.currentplayernum);
	prect.x = (vrect.x * vid.pixelwidth)/vid.width;
	prect.width = (vrect.width * vid.pixelwidth)/vid.width;
	prect.y = (vrect.y * vid.pixelheight)/vid.height;
	prect.height = (vrect.height * vid.pixelheight)/vid.height;

	if (!scenepp_panorama_program)
		return false;

	if (gl_config.arb_texture_non_power_of_two)
	{
		if (prect.width < prect.height)
			cmapsize = prect.width;
		else
			cmapsize = prect.height;
	}
	else
	{
		while (cmapsize > prect.width || cmapsize > prect.height)
		{
			cmapsize /= 2;
		}
	}

	VectorCopy(r_refdef.viewangles, saveang);
	saveang[2] = 0;

	if (ffov.value < 0)
	{
		//panoramic view needs at most the four sides
		if (ffov.value >= -90)
			numsides = 1;
//			else if (ffov.value >= -180)
//			{
//				numsides = 2;
//				rot45 = 1;
//			}
		else if (ffov.value >= -270)
			numsides = 3;
		else
			numsides = 4;

		order[0] = 4;
		order[1] = 0;
		order[2] = 1;
		order[3] = 5;
	}
	else
	{
		//fisheye view sees a full sphere
		//
		if (ffov.value <= 77)
			numsides = 1;
//			else if (ffov.value <= 180)
//			{
//				numsides = 3;
//				rot45 = 3;
//			}
		else if (ffov.value <= 270)
			numsides = 5;
		else
			numsides = 6;

		order[0] = 4;
		order[1] = 0;
		order[2] = 3;
		order[3] = 1;
		order[4] = 2;
		order[5] = 5;
	}

	if (!TEXVALID(scenepp_fisheye_texture))
	{
		scenepp_fisheye_texture = GL_AllocNewTexture(cmapsize, cmapsize);

		qglDisable(GL_TEXTURE_2D);
		qglEnable(GL_TEXTURE_CUBE_MAP_ARB);

		GL_MTBind(0, GL_TEXTURE_CUBE_MAP_ARB, scenepp_fisheye_texture);
		for (i = 0; i < 6; i++)
			qglCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, GL_RGB, 0, 0, cmapsize, cmapsize, 0);
		qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		qglEnable(GL_TEXTURE_2D);
		qglDisable(GL_TEXTURE_CUBE_MAP_ARB);
	}

	r_refdef.vrect.width = cmapsize;
	r_refdef.vrect.height = cmapsize;
	r_refdef.vrect.x = 0;
	r_refdef.vrect.y = prect.y;

	ang[0][0] = -saveang[0];
	ang[0][1] = -90;
	ang[0][2] = -saveang[0];

	ang[1][0] = -saveang[0];
	ang[1][1] = 90;
	ang[1][2] = saveang[0];
	ang[5][0] = -saveang[0]*2;
	for (i = 0; i < numsides; i++)
	{
		r_refdef.fov_x = 90;
		r_refdef.fov_y = 90;
		r_refdef.viewangles[0] = saveang[0]+ang[order[i]][0];
		r_refdef.viewangles[1] = saveang[1]+ang[order[i]][1];
		r_refdef.viewangles[2] = saveang[2]+ang[order[i]][2];

		R_Clear ();

	//	GLR_SetupFog ();

		GL_SetShaderState2D(false);

		// render normal view
		R_RenderScene ();

		GL_MTBind(0, GL_TEXTURE_CUBE_MAP_ARB, scenepp_fisheye_texture);
		qglCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + order[i], 0, 0, 0, 0, vid.pixelheight - (prect.y + cmapsize), cmapsize, cmapsize);
	}

//qglClear (GL_COLOR_BUFFER_BIT);
	qglViewport (prect.x, vid.pixelheight - (prect.y+prect.height), prect.width, prect.height);

	GL_LazyBind(0, GL_TEXTURE_CUBE_MAP_ARB, scenepp_fisheye_texture, false);

	if (scenepp_panorama_program && ffov.value < 0)
	{
		GLSlang_UseProgram(scenepp_panorama_program);
		GLSlang_SetUniform1f(scenepp_panorama_parm_fov, -ffov.value*3.1415926535897932384626433832795/180);
	}
	else
	{
		GLSlang_UseProgram(scenepp_fisheye_program);
		GLSlang_SetUniform1f(scenepp_fisheye_parm_fov, ffov.value*3.1415926535897932384626433832795/180);
	}


	// go 2d
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity ();
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity ();

	qglDisable (GL_DEPTH_TEST);
	GL_CullFace(0);
	qglDisable (GL_ALPHA_TEST);
	qglDisable(GL_BLEND);
	qglBegin(GL_QUADS);
	qglTexCoord2f(-0.5, 0.5);
	qglVertex2f(0, 0);
	qglTexCoord2f(0.5, 0.5);
	qglVertex2f(vid.width, 0);
	qglTexCoord2f(0.5, -0.5);
	qglVertex2f(vid.width, vid.height);
	qglTexCoord2f(-0.5, -0.5);
	qglVertex2f(0, vid.height);
	qglEnd();

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();

	qglDisable(GL_TEXTURE_CUBE_MAP_ARB);
	qglEnable(GL_TEXTURE_2D);

	GLSlang_UseProgram(0);

	qglEnable (GL_DEPTH_TEST);
	PPL_RevertToKnownState();

	return true;
}
#endif

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void GLR_RenderView (void)
{
	double	time1 = 0, time2;

	if (qglGetError())
		Con_Printf("GL Error before drawing scene\n");

	if (r_norefresh.value || !vid.pixelwidth || !vid.pixelheight)
	{
		GL_DoSwap();
		return;
	}

	if (!(r_refdef.flags & Q2RDF_NOWORLDMODEL))
		if (!r_worldentity.model || !cl.worldmodel)
		{
			GL_DoSwap();
			return;
		}
//		Sys_Error ("R_RenderView: NULL worldmodel");



	if (qglPNTrianglesiATI)
	{
		if (gl_ati_truform_type.ival)
		{	//linear
			qglPNTrianglesiATI(GL_PN_TRIANGLES_NORMAL_MODE_ATI, GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI);
			qglPNTrianglesiATI(GL_PN_TRIANGLES_POINT_MODE_ATI, GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI);
		}
		else
		{	//quadric
			qglPNTrianglesiATI(GL_PN_TRIANGLES_NORMAL_MODE_ATI, GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI);
			qglPNTrianglesiATI(GL_PN_TRIANGLES_POINT_MODE_ATI, GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI);
		}
	    qglPNTrianglesfATI(GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI, gl_ati_truform_tesselation.value);
	}

	if (gl_finish.ival)
	{
		RSpeedMark();
		qglFinish ();
		RSpeedEnd(RSPEED_FINISH);
	}

	if (r_speeds.ival)
	{
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

#ifdef FISH
	if (ffov.value && cls.allow_fish && !(r_refdef.flags & Q2RDF_NOWORLDMODEL) && R_RenderScene_Fish())
	{
		//fisheye does its own rendering.
	}
	else
#endif
	{
		GL_SetShaderState2D(false);

		R_Clear ();

	//	GLR_SetupFog ();

		// render normal view
		R_RenderScene ();
	}

//	qglDisable(GL_FOG);

	if (r_speeds.ival)
	{
//		glFinish ();
		time2 = Sys_DoubleTime ();

		RQuantAdd(RQUANT_MSECS, (int)((time2-time1)*1000000));

		RQuantAdd(RQUANT_WPOLYS, c_brush_polys);
		RQuantAdd(RQUANT_EPOLYS, c_alias_polys);
	//	Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys);
	}

	if (qglGetError())
		Con_Printf("GL Error drawing scene\n");

	if (r_refdef.flags & Q2RDF_NOWORLDMODEL)
		return;

	if (r_bloom.ival)
		R_BloomBlend();

	// SCENE POST PROCESSING
	// we check if we need to use any shaders - currently it's just waterwarp
	if ((r_waterwarp.value>0 && r_viewleaf && r_viewleaf->contents <= Q1CONTENTS_WATER))
	{
		GL_Set2D();
		if (scenepp_waterwarp)
			R2D_ScalePic(0, 0, vid.width, vid.height, scenepp_waterwarp);
	}



	if (gl_motionblur.value>0 && gl_motionblur.value < 1 && qglCopyTexImage2D)
		R_RenderMotionBlur();

	if (qglGetError())
		Con_Printf("GL Error drawing post processing\n");
}

#endif
