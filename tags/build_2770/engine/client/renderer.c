#include "quakedef.h"
#include "winquake.h"
#ifdef RGLQUAKE
#include "gl_draw.h"
#endif
#ifdef SWQUAKE
#include "sw_draw.h"
#endif


qboolean vid_isfullscreen;

#define VIDCOMMANDGROUP "Video config"
#define GRAPHICALNICETIES "Graphical Nicaties"	//or eyecandy, which ever you prefer.
#define BULLETENVARS		"BulletenBoard controls"
#define GLRENDEREROPTIONS	"GL Renderer Options"
#define SWRENDEREROPTIONS	"SW Renderer Options"
#define SCREENOPTIONS	"Screen Options"


unsigned short	d_8to16table[256];
unsigned int	d_8to24bgrtable[256];
unsigned int	d_8to24rgbtable[256];
unsigned int	*d_8to32table = d_8to24bgrtable;	//palette lookups while rendering r_pixbytes=4

extern int gl_anisotropy_factor;

// callbacks used for cvars
void SCR_Viewsize_Callback (struct cvar_s *var, char *oldvalue);
void SCR_Fov_Callback (struct cvar_s *var, char *oldvalue);
#if defined(RGLQUAKE)
void GL_Texturemode_Callback (struct cvar_s *var, char *oldvalue);
void GL_Texture_Anisotropic_Filtering_Callback (struct cvar_s *var, char *oldvalue);
#endif

//

cvar_t	r_drawviewmodel = SCVAR("r_drawviewmodel","1");
cvar_t  r_drawviewmodelinvis = SCVAR("r_drawviewmodelinvis", "0");
cvar_t	r_netgraph = SCVAR("r_netgraph","0");
cvar_t	r_speeds = SCVARF("r_speeds","0", CVAR_CHEAT);
cvar_t	r_waterwarp = SCVARF("r_waterwarp","1", CVAR_ARCHIVE);
cvar_t	r_drawentities = SCVAR("r_drawentities","1");
cvar_t	r_fullbright = SCVARF("r_fullbright","0", CVAR_CHEAT);
cvar_t	r_ambient = SCVARF("r_ambient", "0", CVAR_CHEAT);
#if defined(SWQUAKE)
cvar_t	r_draworder = SCVARF("r_draworder","0", CVAR_CHEAT);
cvar_t	r_timegraph = SCVAR("r_timegraph","0");
cvar_t	r_zgraph = SCVAR("r_zgraph","0");
cvar_t	r_graphheight = SCVAR("r_graphheight","15");
cvar_t	r_clearcolor = SCVAR("r_clearcolor","218");
cvar_t	r_aliasstats = SCVAR("r_polymodelstats","0");
cvar_t	r_dspeeds = SCVAR("r_dspeeds","0");
//cvar_t	r_drawflat = SCVARF("r_drawflat", "0", CVAR_CHEAT);
cvar_t	r_reportsurfout = SCVAR("r_reportsurfout", "0");
cvar_t	r_maxsurfs = SCVAR("r_maxsurfs", "0");
cvar_t	r_numsurfs = SCVAR("r_numsurfs", "0");
cvar_t	r_reportedgeout = SCVAR("r_reportedgeout", "0");
cvar_t	r_maxedges = SCVAR("r_maxedges", "0");
cvar_t	r_numedges = SCVAR("r_numedges", "0");
cvar_t	r_aliastransbase = SCVAR("r_aliastransbase", "200");
cvar_t	r_aliastransadj = SCVAR("r_aliastransadj", "100");
cvar_t	d_smooth = SCVAR("d_smooth", "0");
cvar_t  sw_surfcachesize = SCVARF("sw_surfcachesize", "0", CVAR_RENDERERLATCH);
#endif
cvar_t	gl_skyboxdist = SCVAR("gl_skyboxdist", "2300");

cvar_t	r_vertexdlights = SCVAR("r_vertexdlights", "1");

cvar_t cl_cursor = SCVAR("cl_cursor", "");
cvar_t cl_cursorsize = SCVAR("cl_cursorsize", "32");
cvar_t cl_cursorbias = SCVAR("cl_cursorbias", "4");

extern	cvar_t	r_dodgytgafiles;
extern	cvar_t	r_dodgypcxfiles;

cvar_t	r_nolerp = SCVAR("r_nolerp", "0");
cvar_t	r_nolightdir = SCVAR("r_nolightdir", "0");

cvar_t	r_sirds = SCVARF("r_sirds", "0", CVAR_SEMICHEAT);//whack in a value of 2 and you get easily visible players.

cvar_t	r_loadlits = SCVAR("r_loadlit", "1");

cvar_t r_stains = SCVARFC("r_stains", "0.75", CVAR_ARCHIVE, Cvar_Limiter_ZeroToOne_Callback);
cvar_t r_stainfadetime = SCVAR("r_stainfadetime", "1");
cvar_t r_stainfadeammount = SCVAR("r_stainfadeammount", "1");

cvar_t		_windowed_mouse = SCVARF("_windowed_mouse","1", CVAR_ARCHIVE);
cvar_t		_vid_wait_override = FCVAR("vid_wait", "_vid_wait_override", "", CVAR_ARCHIVE);

static cvar_t		vid_stretch = SCVARF("vid_stretch","1", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
//cvar_t		_windowed_mouse = SCVARF("_windowed_mouse","1", CVAR_ARCHIVE);
static cvar_t	gl_driver = SCVARF("gl_driver","", CVAR_ARCHIVE|CVAR_RENDERERLATCH);	//opengl library
cvar_t	vid_renderer = SCVARF("vid_renderer", "", CVAR_ARCHIVE|CVAR_RENDERERLATCH);	//see R_RestartRenderer_f for the effective default 'if (newr.renderer == -1)'.

static cvar_t	vid_bpp = SCVARF("vid_bpp", "32", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
static cvar_t	vid_allow_modex = SCVARF("vid_allow_modex", "1", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
static cvar_t	vid_fullscreen = SCVARF("vid_fullscreen", "1", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
static cvar_t	vid_width = SCVARF("vid_width", "640", CVAR_ARCHIVE|CVAR_RENDERERLATCH);	//more readable defaults to match conwidth/conheight.
static cvar_t	vid_height = SCVARF("vid_height", "480", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
static cvar_t	vid_refreshrate = SCVARF("vid_displayfrequency", "0", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
static cvar_t	vid_multisample = SCVARF("vid_multisample", "0", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
static cvar_t	vid_desktopsettings = SCVARF("vid_desktopsettings", "0", CVAR_ARCHIVE|CVAR_RENDERERLATCH);

#if defined(RGLQUAKE)
cvar_t	gl_texturemode = SCVARFC("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE|CVAR_RENDERERCALLBACK, GL_Texturemode_Callback);
cvar_t	gl_texture_anisotropic_filtering = SCVARFC("gl_texture_anisotropic_filtering", "0", CVAR_ARCHIVE|CVAR_RENDERERCALLBACK, GL_Texture_Anisotropic_Filtering_Callback);
cvar_t	gl_conback = SCVARF("gl_conback", "", CVAR_RENDERERCALLBACK);
cvar_t	gl_font = SCVARF("gl_font", "", CVAR_RENDERERCALLBACK);
//gl blends. Set this to 1 to stop the outside of your conchars from being visible
cvar_t	gl_fontinwardstep = SCVAR("gl_fontinwardstep", "0");	
cvar_t	gl_smoothfont	= SCVAR("gl_smoothfont", "1");
cvar_t  gl_smoothcrosshair = SCVAR("gl_smoothcrosshair", "1");
#endif
cvar_t	gl_motionblur = SCVARF("gl_motionblur", "0", CVAR_ARCHIVE);
cvar_t	gl_motionblurscale = SCVAR("gl_motionblurscale", "1");
cvar_t	gl_part_flame	= SCVAR("gl_part_flame", "1");
cvar_t	r_part_rain	= SCVARF("r_part_rain", "0", CVAR_ARCHIVE);

cvar_t	r_bouncysparks	= SCVARF("r_bouncysparks", "0", CVAR_ARCHIVE);

cvar_t	r_fullbrightSkins	= SCVARF("r_fullbrightSkins", "0",	CVAR_SEMICHEAT);
cvar_t	r_fb_models		= FCVAR("r_fb_models", "gl_fb_models", "1", CVAR_SEMICHEAT|CVAR_RENDERERLATCH);
cvar_t	r_fb_bmodels	= SCVARF("gl_fb_bmodels", "1", CVAR_SEMICHEAT|CVAR_RENDERERLATCH);

cvar_t	r_shadow_bumpscale_basetexture	= SCVAR("r_shadow_bumpscale_basetexture", "4");
cvar_t	r_shadow_bumpscale_bumpmap	= SCVAR("r_shadow_bumpscale_bumpmap", "10");
cvar_t	gl_nocolors	= SCVAR("gl_nocolors","0");
cvar_t		gl_load24bit	= SCVARF("gl_load24bit", "1", CVAR_ARCHIVE);
cvar_t		vid_conwidth	= SCVARF("vid_conwidth", "640", CVAR_ARCHIVE|CVAR_RENDERERCALLBACK);
cvar_t		vid_conheight	= SCVARF("vid_conheight", "480", CVAR_ARCHIVE);
cvar_t		vid_conautoscale = SCVARF("vid_conautoscale", "0", CVAR_ARCHIVE|CVAR_RENDERERCALLBACK);
cvar_t		gl_nobind	= SCVAR("gl_nobind", "0");
cvar_t		gl_max_size	= SCVAR("gl_max_size", "1024");
cvar_t		gl_picmip	= SCVAR("gl_picmip", "0");
cvar_t		gl_picmip2d	= SCVAR("gl_picmip2d", "0");
cvar_t		r_drawdisk	= SCVAR("r_drawdisk", "1");
cvar_t		gl_compress	= SCVARF("gl_compress", "0", CVAR_ARCHIVE);
cvar_t		gl_savecompressedtex	 = SCVAR("gl_savecompressedtex", "0");
extern cvar_t gl_dither;
extern	cvar_t	gl_maxdist;
extern	cvar_t	gl_mindist;


cvar_t		gl_detail	= SCVARF("gl_detail", "0", CVAR_ARCHIVE);
cvar_t		gl_detailscale	= SCVAR("gl_detailscale", "5");
cvar_t		gl_overbright	= SCVARF("gl_overbright", "0", CVAR_ARCHIVE);
cvar_t		gl_overbright_all = SCVARF("gl_overbright_all", "0", CVAR_ARCHIVE);
cvar_t		r_shadows	= SCVARF("r_shadows", "0", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
cvar_t		r_shadow_realtime_world	= SCVARF("r_shadow_realtime_world", "0", CVAR_CHEAT|CVAR_ARCHIVE);
cvar_t		r_shadow_realtime_world_lightmaps	= SCVARF("r_shadow_realtime_world_lightmaps", "0.8", CVAR_CHEAT);
cvar_t		r_noaliasshadows	= SCVARF("r_noaliasshadows", "0", CVAR_ARCHIVE);
cvar_t		gl_maxshadowlights	= SCVARF("gl_maxshadowlights", "2", CVAR_ARCHIVE);
cvar_t		gl_bump	= SCVARF("gl_bump", "0", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
cvar_t r_shadow_glsl_offsetmapping	= SCVAR("r_shadow_glsl_offsetmapping", "0");
cvar_t r_shadow_glsl_offsetmapping_scale	= SCVAR("r_shadow_glsl_offsetmapping_scale", "-0.04");
cvar_t r_shadow_glsl_offsetmapping_bias	= SCVAR("r_shadow_glsl_offsetmapping_bias", "0.04");
#ifdef SPECULAR
cvar_t		gl_specular	= SCVAR("gl_specular", "0");
#endif
//cvar_t		gl_lightmapmode	= SCVARF("gl_lightmapmode", "", CVAR_ARCHIVE);

cvar_t		gl_ati_truform	= SCVAR("gl_ati_truform", "0");
cvar_t		gl_ati_truform_type	= SCVAR("gl_ati_truform_type", "1");
cvar_t		gl_ati_truform_tesselation	= SCVAR("gl_ati_truform_tesselation", "3");

cvar_t		mod_md3flags	= SCVAR("mod_md3flags", "1");

cvar_t		gl_lateswap	= SCVAR("gl_lateswap", "0");

cvar_t		gl_mylumassuck	= SCVAR("gl_mylumassuck", "0");

cvar_t			scr_sshot_type	= SCVAR("scr_sshot_type", "jpg");
cvar_t			scr_sshot_compression = SCVAR("scr_sshot_compression", "75");

cvar_t			scr_centersbar	= SCVAR("scr_centersbar", "0");
cvar_t			scr_consize	= SCVAR("scr_consize", "0.5");
cvar_t			scr_conalpha = SCVARC("scr_conalpha", "0.7", Cvar_Limiter_ZeroToOne_Callback);

cvar_t          scr_viewsize	= SCVARFC("viewsize","100", CVAR_ARCHIVE, SCR_Viewsize_Callback);
cvar_t          scr_fov	= SCVARFC("fov","90", CVAR_ARCHIVE, SCR_Fov_Callback); // 10 - 170
cvar_t          scr_conspeed	= SCVAR("scr_conspeed","300");
cvar_t          scr_centertime	= SCVAR("scr_centertime","2");
cvar_t          scr_showram	= SCVAR("showram","1");
cvar_t          scr_showturtle	= SCVAR("showturtle","0");
cvar_t          scr_showpause	= SCVAR("showpause","1");
cvar_t          scr_printspeed	= SCVAR("scr_printspeed","8");
cvar_t			scr_allowsnap	= SCVARF("scr_allowsnap", "1", CVAR_NOTFROMSERVER);	//otherwise it would defeat the point.
cvar_t			con_ocranaleds	= SCVAR("con_ocranaleds", "2");

cvar_t			scr_chatmodecvar	= SCVAR("scr_chatmode", "0");

#ifdef Q3SHADERS
cvar_t 			gl_shadeq3	= SCVARF("gl_shadeq3", "1", CVAR_SEMICHEAT);	//use if you want.
cvar_t 			gl_shadeq2	= SCVARF("gl_shadeq2", "0", CVAR_SEMICHEAT);	//use if you want.
extern cvar_t r_vertexlight;
cvar_t			gl_shadeq1	= SCVARF("gl_shadeq1", "0", CVAR_SEMICHEAT);
cvar_t			gl_shadeq1_name	= SCVAR("gl_shadeq1_name", "*");
#endif

cvar_t			gl_blend2d	= SCVAR("gl_blend2d", "1");
cvar_t			gl_blendsprites	= SCVAR("gl_blendsprites", "1");

cvar_t r_bloodstains	= SCVAR("r_bloodstains", "1");

extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_speeds;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_shadows;
extern	cvar_t	r_mirroralpha;
cvar_t	r_wateralpha = SCVAR("r_wateralpha","1");
cvar_t	r_dynamic	= SCVARF("r_dynamic","1", CVAR_ARCHIVE);
cvar_t	r_flashblend	= SCVARF("gl_flashblend","0", CVAR_ARCHIVE);
cvar_t	r_lightstylesmooth	= SCVAR("r_lightstylesmooth", "0");
cvar_t	r_lightstylespeed	= SCVAR("r_lightstylespeed", "10");
extern	cvar_t	r_novis;
extern	cvar_t	r_netgraph;

cvar_t r_drawflat	= SCVARF("r_drawflat","0", CVAR_SEMICHEAT|CVAR_RENDERERCALLBACK);
cvar_t r_drawflat_nonworldmodel = SCVAR("r_drawflat_nonworldmodel", "0");
cvar_t r_wallcolour	= SCVARF("r_wallcolour","255 255 255", CVAR_RENDERERCALLBACK);
cvar_t r_floorcolour	= SCVARF("r_floorcolour","255 255 255", CVAR_RENDERERCALLBACK);
cvar_t r_walltexture	= SCVARF("r_walltexture","", CVAR_RENDERERCALLBACK);
cvar_t r_floortexture	= SCVARF("r_floortexture","", CVAR_RENDERERCALLBACK);

cvar_t d_palconvwrite	= SCVAR("d_palconvwrite", "1");
cvar_t d_palremapsize	= SCVARF("d_palremapsize", "64", CVAR_RENDERERLATCH);

cvar_t r_lightmap_saturation	= SCVAR("r_lightmap_saturation", "1");

extern cvar_t bul_text1;
extern cvar_t bul_text2;
extern cvar_t bul_text3;
extern cvar_t bul_text4;
extern cvar_t bul_text5;
extern cvar_t bul_text6;

extern cvar_t bul_scrollspeedx;
extern cvar_t bul_scrollspeedy;
extern cvar_t bul_backcol;
extern cvar_t bul_textpalette;
extern cvar_t bul_norender;
extern cvar_t bul_sparkle;
extern cvar_t bul_forcemode;
extern cvar_t bul_ripplespeed;
extern cvar_t bul_rippleamount;
extern cvar_t bul_nowater;
void R_BulletenForce_f (void);

rendererstate_t currentrendererstate;

cvar_t	r_skyboxname	= SCVARF("r_skybox", "", CVAR_RENDERERCALLBACK);
cvar_t	r_fastsky	= SCVAR("r_fastsky", "0");
cvar_t	r_fastskycolour	= SCVAR("r_fastskycolour", "0");

cvar_t r_menutint = SCVARF("r_menutint", "0.68 0.4 0.13", CVAR_RENDERERCALLBACK);

#if defined(RGLQUAKE)
cvar_t gl_schematics = SCVAR("gl_schematics","0");
cvar_t	gl_ztrick = SCVAR("gl_ztrick","0");
cvar_t	gl_lerpimages = SCVAR("gl_lerpimages", "1");
cvar_t gl_lightmap_shift = SCVARF("gl_lightmap_shift", "0", CVAR_ARCHIVE | CVAR_LATCH);
cvar_t gl_menutint_shader = SCVAR("gl_menutint_shader", "1");
extern cvar_t r_waterlayers;
cvar_t			gl_triplebuffer = SCVARF("gl_triplebuffer", "1", CVAR_ARCHIVE);
cvar_t			vid_hardwaregamma = SCVARF("vid_hardwaregamma", "1", CVAR_ARCHIVE|CVAR_RENDERERLATCH);
cvar_t			vid_desktopgamma = SCVARF("vid_desktopgamma", "0", CVAR_ARCHIVE|CVAR_RENDERERLATCH);

void GLRenderer_Init(void)
{
	extern cvar_t gl_contrast;
	//screen
	Cvar_Register (&gl_triplebuffer, GLRENDEREROPTIONS);

	Cvar_Register (&vid_hardwaregamma, GLRENDEREROPTIONS);
	Cvar_Register (&vid_desktopgamma, GLRENDEREROPTIONS);

//renderer
	Cvar_Register (&r_novis, GLRENDEREROPTIONS);
	Cvar_Register (&r_mirroralpha, GLRENDEREROPTIONS);
	Cvar_Register (&r_norefresh, GLRENDEREROPTIONS);


	Cvar_Register (&gl_clear, GLRENDEREROPTIONS);

	Cvar_Register (&gl_cull, GLRENDEREROPTIONS);
	Cvar_Register (&gl_smoothmodels, GRAPHICALNICETIES);
	Cvar_Register (&gl_affinemodels, GLRENDEREROPTIONS);
	Cvar_Register (&gl_polyblend, GLRENDEREROPTIONS);
	Cvar_Register (&r_flashblend, GLRENDEREROPTIONS);
	Cvar_Register (&gl_playermip, GLRENDEREROPTIONS);
	Cvar_Register (&gl_nocolors, GLRENDEREROPTIONS);
	Cvar_Register (&gl_finish, GLRENDEREROPTIONS);
	Cvar_Register (&gl_lateswap, GLRENDEREROPTIONS);
	Cvar_Register (&gl_lerpimages, GLRENDEREROPTIONS);

	Cvar_Register (&r_shadows, GLRENDEREROPTIONS);
	Cvar_Register (&r_noaliasshadows, GLRENDEREROPTIONS);
	Cvar_Register (&gl_maxshadowlights, GLRENDEREROPTIONS);
	Cvar_Register (&r_shadow_bumpscale_basetexture, GLRENDEREROPTIONS);
	Cvar_Register (&r_shadow_bumpscale_bumpmap, GLRENDEREROPTIONS);
	Cvar_Register (&r_shadow_realtime_world, GLRENDEREROPTIONS);
	Cvar_Register (&r_shadow_realtime_world_lightmaps, GLRENDEREROPTIONS);

	Cvar_Register (&gl_keeptjunctions, GLRENDEREROPTIONS);
	Cvar_Register (&gl_reporttjunctions, GLRENDEREROPTIONS);

	Cvar_Register (&gl_ztrick, GLRENDEREROPTIONS);

	Cvar_Register (&gl_motionblur, GLRENDEREROPTIONS);
	Cvar_Register (&gl_motionblurscale, GLRENDEREROPTIONS);
	Cvar_Register (&gl_max_size, GLRENDEREROPTIONS);
	Cvar_Register (&gl_maxdist, GLRENDEREROPTIONS);
	Cvar_Register (&gl_mindist, GLRENDEREROPTIONS);
	Cvar_Register (&vid_multisample, GLRENDEREROPTIONS);

	Cvar_Register (&gl_fontinwardstep, GRAPHICALNICETIES);
	Cvar_Register (&gl_font, GRAPHICALNICETIES);
	Cvar_Register (&gl_conback, GRAPHICALNICETIES);
	Cvar_Register (&gl_smoothfont, GRAPHICALNICETIES);
	Cvar_Register (&gl_smoothcrosshair, GRAPHICALNICETIES);

	Cvar_Register (&gl_bump, GRAPHICALNICETIES);
	Cvar_Register (&r_shadow_glsl_offsetmapping, GRAPHICALNICETIES);
	Cvar_Register (&r_shadow_glsl_offsetmapping_scale, GRAPHICALNICETIES);
	Cvar_Register (&r_shadow_glsl_offsetmapping_bias, GRAPHICALNICETIES);

	Cvar_Register (&gl_contrast, GLRENDEREROPTIONS);
#ifdef R_XFLIP
	Cvar_Register (&r_xflip, GLRENDEREROPTIONS);
#endif
	Cvar_Register (&gl_load24bit, GRAPHICALNICETIES);
	Cvar_Register (&gl_specular, GRAPHICALNICETIES);

//	Cvar_Register (&gl_lightmapmode, GLRENDEREROPTIONS);

#ifdef WATERLAYERS
	Cvar_Register (&r_waterlayers, GRAPHICALNICETIES);
#endif

	Cvar_Register (&gl_nobind, GLRENDEREROPTIONS);
	Cvar_Register (&gl_picmip, GLRENDEREROPTIONS);
	Cvar_Register (&gl_picmip2d, GLRENDEREROPTIONS);
	Cvar_Register (&r_drawdisk, GLRENDEREROPTIONS);

	Cvar_Register (&gl_texturemode, GLRENDEREROPTIONS);
	Cvar_Register (&gl_texture_anisotropic_filtering, GLRENDEREROPTIONS);
	Cvar_Register (&gl_savecompressedtex, GLRENDEREROPTIONS);
	Cvar_Register (&gl_compress, GLRENDEREROPTIONS);
	Cvar_Register (&gl_driver, GLRENDEREROPTIONS);
	Cvar_Register (&gl_detail, GRAPHICALNICETIES);
	Cvar_Register (&gl_detailscale, GRAPHICALNICETIES);
	Cvar_Register (&gl_overbright, GRAPHICALNICETIES);
	Cvar_Register (&gl_overbright_all, GRAPHICALNICETIES);
	Cvar_Register (&gl_dither, GRAPHICALNICETIES);
	Cvar_Register (&r_fb_bmodels, GRAPHICALNICETIES);

	Cvar_Register (&gl_ati_truform, GRAPHICALNICETIES);
	Cvar_Register (&gl_ati_truform_type, GRAPHICALNICETIES);
	Cvar_Register (&gl_ati_truform_tesselation, GRAPHICALNICETIES);

	Cvar_Register (&gl_skyboxdist, GLRENDEREROPTIONS);

	Cvar_Register (&r_wallcolour, GLRENDEREROPTIONS);
	Cvar_Register (&r_floorcolour, GLRENDEREROPTIONS);
	Cvar_Register (&r_walltexture, GLRENDEREROPTIONS);
	Cvar_Register (&r_floortexture, GLRENDEREROPTIONS);

	Cvar_Register (&r_vertexdlights, GLRENDEREROPTIONS);

	Cvar_Register (&gl_schematics, GLRENDEREROPTIONS);
#ifdef Q3SHADERS
	Cvar_Register (&r_vertexlight, GLRENDEREROPTIONS);
	Cvar_Register (&gl_shadeq1, GLRENDEREROPTIONS);
	Cvar_Register (&gl_shadeq1_name, GLRENDEREROPTIONS);
	Cvar_Register (&gl_shadeq2, GLRENDEREROPTIONS);
	Cvar_Register (&gl_shadeq3, GLRENDEREROPTIONS);

	Cvar_Register (&gl_blend2d, GLRENDEREROPTIONS);
#endif
	Cvar_Register (&gl_blendsprites, GLRENDEREROPTIONS);

	Cvar_Register (&gl_mylumassuck, GLRENDEREROPTIONS);

	Cvar_Register (&gl_lightmap_shift, GLRENDEREROPTIONS);

	Cvar_Register (&gl_menutint_shader, GLRENDEREROPTIONS);

	R_BloomRegister();
}
#endif
#if defined(SWQUAKE)
extern cvar_t d_subdiv16;
extern cvar_t d_mipcap;
extern cvar_t d_mipscale;
extern cvar_t d_smooth;
void SWRenderer_Init(void)
{
	Cvar_Register (&d_subdiv16, SWRENDEREROPTIONS);
	Cvar_Register (&d_mipcap, SWRENDEREROPTIONS);
	Cvar_Register (&d_mipscale, SWRENDEREROPTIONS);
	Cvar_Register (&d_smooth, SWRENDEREROPTIONS);

	Cvar_Register (&r_maxsurfs, SWRENDEREROPTIONS);
	Cvar_Register (&r_numsurfs, SWRENDEREROPTIONS);
	Cvar_Register (&r_maxedges, SWRENDEREROPTIONS);
	Cvar_Register (&r_numedges, SWRENDEREROPTIONS);
	Cvar_Register (&r_sirds, SWRENDEREROPTIONS);

	Cvar_Register (&r_aliastransbase, SWRENDEREROPTIONS);
	Cvar_Register (&r_aliastransadj, SWRENDEREROPTIONS);
	Cvar_Register (&r_reportedgeout, SWRENDEREROPTIONS);
	Cvar_Register (&r_aliasstats, SWRENDEREROPTIONS);
	Cvar_Register (&r_clearcolor, SWRENDEREROPTIONS);

	Cvar_Register (&r_timegraph, SWRENDEREROPTIONS);
	Cvar_Register (&r_draworder, SWRENDEREROPTIONS);
	Cvar_Register (&r_zgraph, SWRENDEREROPTIONS);
	Cvar_Register (&r_graphheight, SWRENDEREROPTIONS);
	Cvar_Register (&r_aliasstats, SWRENDEREROPTIONS);
	Cvar_Register (&r_dspeeds, SWRENDEREROPTIONS);
	Cvar_Register (&r_ambient, SWRENDEREROPTIONS);
	Cvar_Register (&r_ambient, SWRENDEREROPTIONS);
	Cvar_Register (&r_reportsurfout, SWRENDEREROPTIONS);

	Cvar_Register (&d_palconvwrite, SWRENDEREROPTIONS);
	Cvar_Register (&d_palremapsize, SWRENDEREROPTIONS);

	Cvar_Register (&sw_surfcachesize, SWRENDEREROPTIONS);
}
#endif

void	R_InitTextures (void)
{
	int		x,y, m;
	qbyte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = BZ_Malloc (sizeof(texture_t) + 16*16+8*8+4*4+2*2);

	r_notexture_mip->pixbytes = 1;
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;

	for (m=0 ; m<4 ; m++)
	{
		dest = (qbyte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y=0 ; y< (16>>m) ; y++)
			for (x=0 ; x< (16>>m) ; x++)
			{
				if (  (y< (8>>m) ) ^ (x< (8>>m) ) )
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}
}



void R_SetRenderer_f (void);

void Renderer_Init(void)
{
	currentrendererstate.bpp = -1;	//no previous.

	currentrendererstate.renderer = -1;

	Cmd_AddCommand("setrenderer", R_SetRenderer_f);
	Cmd_AddCommand("vid_restart", R_RestartRenderer_f);

#if defined(RGLQUAKE)
	GLRenderer_Init();
#endif
#if defined(SWQUAKE)
	SWRenderer_Init();
#endif

	//but register ALL vid_ commands.
	Cvar_Register (&_vid_wait_override, VIDCOMMANDGROUP);
	Cvar_Register (&vid_stretch, VIDCOMMANDGROUP);
	Cvar_Register (&_windowed_mouse, VIDCOMMANDGROUP);
	Cvar_Register (&vid_renderer, VIDCOMMANDGROUP);

	Cvar_Register (&vid_fullscreen, VIDCOMMANDGROUP);
//	Cvar_Register (&vid_stretch, VIDCOMMANDGROUP);
	Cvar_Register (&vid_bpp, VIDCOMMANDGROUP);

	Cvar_Register (&vid_conwidth, VIDCOMMANDGROUP);
	Cvar_Register (&vid_conheight, VIDCOMMANDGROUP);
	Cvar_Register (&vid_conautoscale, VIDCOMMANDGROUP);

	Cvar_Register (&vid_allow_modex, VIDCOMMANDGROUP);

	Cvar_Register (&vid_width, VIDCOMMANDGROUP);
	Cvar_Register (&vid_height, VIDCOMMANDGROUP);
	Cvar_Register (&vid_refreshrate, VIDCOMMANDGROUP);

	Cvar_Register (&vid_desktopsettings, VIDCOMMANDGROUP);

	Cvar_Register (&r_skyboxname, GRAPHICALNICETIES);

	Cvar_Register(&r_dodgytgafiles, "Bug fixes");
	Cvar_Register(&r_dodgypcxfiles, "Bug fixes");
	Cvar_Register(&r_loadlits, GRAPHICALNICETIES);
	Cvar_Register(&r_lightstylesmooth, GRAPHICALNICETIES);
	Cvar_Register(&r_lightstylespeed, GRAPHICALNICETIES);

	Cvar_Register(&r_stains, GRAPHICALNICETIES);
	Cvar_Register(&r_stainfadetime, GRAPHICALNICETIES);
	Cvar_Register(&r_stainfadeammount, GRAPHICALNICETIES);

	Cvar_Register(&scr_viewsize, SCREENOPTIONS);
	Cvar_Register(&scr_fov, SCREENOPTIONS);
	Cvar_Register(&scr_chatmodecvar, SCREENOPTIONS);

	Cvar_Register (&scr_sshot_type, SCREENOPTIONS);
	Cvar_Register (&scr_sshot_compression, SCREENOPTIONS);

	Cvar_Register(&cl_cursor,	SCREENOPTIONS);
	Cvar_Register(&cl_cursorsize,	SCREENOPTIONS);
	Cvar_Register(&cl_cursorbias,	SCREENOPTIONS);


//screen
	Cvar_Register (&scr_conspeed, SCREENOPTIONS);
	Cvar_Register (&scr_conalpha, SCREENOPTIONS);
	Cvar_Register (&scr_showram, SCREENOPTIONS);
	Cvar_Register (&scr_showturtle, SCREENOPTIONS);
	Cvar_Register (&scr_showpause, SCREENOPTIONS);
	Cvar_Register (&scr_centertime, SCREENOPTIONS);
	Cvar_Register (&scr_printspeed, SCREENOPTIONS);
	Cvar_Register (&scr_allowsnap, SCREENOPTIONS);
	Cvar_Register (&scr_consize, SCREENOPTIONS);
	Cvar_Register (&scr_centersbar, SCREENOPTIONS);

	Cvar_Register(&r_bloodstains, GRAPHICALNICETIES);

	Cvar_Register(&r_fullbrightSkins, GRAPHICALNICETIES);

	Cvar_Register (&mod_md3flags, GRAPHICALNICETIES);


//renderer
	Cvar_Register (&r_fullbright, SCREENOPTIONS);
	Cvar_Register (&r_drawentities, GRAPHICALNICETIES);
	Cvar_Register (&r_drawviewmodel, GRAPHICALNICETIES);
	Cvar_Register (&r_drawviewmodelinvis, GRAPHICALNICETIES);
	Cvar_Register (&r_waterwarp, GRAPHICALNICETIES);
	Cvar_Register (&r_speeds, SCREENOPTIONS);
	Cvar_Register (&r_netgraph, SCREENOPTIONS);

	Cvar_Register (&r_dynamic, GRAPHICALNICETIES);
	Cvar_Register (&r_lightmap_saturation, GRAPHICALNICETIES);

	Cvar_Register (&r_nolerp, GRAPHICALNICETIES);
	Cvar_Register (&r_nolightdir, GRAPHICALNICETIES);

	Cvar_Register (&r_fastsky, GRAPHICALNICETIES);
	Cvar_Register (&r_fastskycolour, GRAPHICALNICETIES);
	Cvar_Register (&r_wateralpha, GRAPHICALNICETIES);

	Cvar_Register (&r_drawflat, GRAPHICALNICETIES);
	Cvar_Register (&r_drawflat_nonworldmodel, GRAPHICALNICETIES);
	Cvar_Register (&r_menutint, GRAPHICALNICETIES);

	Cvar_Register (&r_fb_models, GRAPHICALNICETIES);

//bulletens
	Cvar_Register(&bul_nowater, BULLETENVARS);
	Cvar_Register(&bul_rippleamount, BULLETENVARS);
	Cvar_Register(&bul_ripplespeed, BULLETENVARS);
	Cvar_Register(&bul_forcemode, BULLETENVARS);
	Cvar_Register(&bul_sparkle, BULLETENVARS);
	Cvar_Register(&bul_textpalette, BULLETENVARS);
	Cvar_Register(&bul_scrollspeedy, BULLETENVARS);
	Cvar_Register(&bul_scrollspeedx, BULLETENVARS);
	Cvar_Register(&bul_backcol, BULLETENVARS);

	Cvar_Register(&bul_text6,	BULLETENVARS);	//reverse order, to get forwards ordered console vars.
	Cvar_Register(&bul_text5,	BULLETENVARS);
	Cvar_Register(&bul_text4,	BULLETENVARS);
	Cvar_Register(&bul_text3,	BULLETENVARS);
	Cvar_Register(&bul_text2,	BULLETENVARS);
	Cvar_Register(&bul_text1,	BULLETENVARS);


// misc
	Cvar_Register(&con_ocranaleds, "Console controls");

	Cvar_Register(&bul_norender,	BULLETENVARS);	//find this one first...

	Cmd_AddCommand("bul_make",	R_BulletenForce_f);

	P_InitParticles();
	R_InitTextures();
	RQ_Init();
}


mpic_t	*(*Draw_SafePicFromWad)		(char *name);
mpic_t	*(*Draw_CachePic)			(char *path);
mpic_t	*(*Draw_SafeCachePic)		(char *path);
void	(*Draw_Init)				(void);
void	(*Draw_ReInit)				(void);
void	(*Draw_Character)			(int x, int y, unsigned int num);
void	(*Draw_ColouredCharacter)	(int x, int y, unsigned int num);
void	(*Draw_String)				(int x, int y, const qbyte *str);
void	(*Draw_Alt_String)			(int x, int y, const qbyte *str);
void	(*Draw_Crosshair)			(void);
void	(*Draw_DebugChar)			(qbyte num);
void	(*Draw_Pic)					(int x, int y, mpic_t *pic);
void	(*Draw_ScalePic)			(int x, int y, int width, int height, mpic_t *pic);
void	(*Draw_SubPic)				(int x, int y, mpic_t *pic, int srcx, int srcy, int width, int height);
void	(*Draw_TransPic)			(int x, int y, mpic_t *pic);
void	(*Draw_TransPicTranslate)	(int x, int y, int w, int h, qbyte *image, qbyte *translation);
void	(*Draw_ConsoleBackground)	(int lines);
void	(*Draw_EditorBackground)	(int lines);
void	(*Draw_TileClear)			(int x, int y, int w, int h);
void	(*Draw_Fill)				(int x, int y, int w, int h, int c);
void    (*Draw_FillRGB)				(int x, int y, int w, int h, float r, float g, float b);
void	(*Draw_FadeScreen)			(void);
void	(*Draw_BeginDisc)			(void);
void	(*Draw_EndDisc)				(void);

void	(*Draw_Image)							(float x, float y, float w, float h, float s1, float t1, float s2, float t2, mpic_t *pic);	//gl-style scaled/coloured/subpic
void	(*Draw_ImageColours)		(float r, float g, float b, float a);

void	(*R_Init)					(void);
void	(*R_DeInit)					(void);
void	(*R_ReInit)					(void);
void	(*R_RenderView)				(void);		// must set r_refdef first

qboolean	(*R_CheckSky)			(void);
void	(*R_SetSky)					(char *name, float rotate, vec3_t axis);

void	(*R_NewMap)					(void);
void	(*R_PreNewMap)				(void);
int		(*R_LightPoint)				(vec3_t point);

void	(*R_PushDlights)			(void);
void	(*R_AddStain)				(vec3_t org, float red, float green, float blue, float radius);
void	(*R_LessenStains)			(void);

void (*Media_ShowFrameBGR_24_Flip)	(qbyte *framedata, int inwidth, int inheight);	//input is bottom up...
void (*Media_ShowFrameRGBA_32)		(qbyte *framedata, int inwidth, int inheight);	//top down
void (*Media_ShowFrame8bit)			(qbyte *framedata, int inwidth, int inheight, qbyte *palette);	//paletted topdown (framedata is 8bit indexes into palette)

void	(*Mod_Init)					(void);
void	(*Mod_ClearAll)				(void);
struct model_s *(*Mod_ForName)		(char *name, qboolean crash);
struct model_s *(*Mod_FindName)		(char *name);
void	*(*Mod_Extradata)			(struct model_s *mod);	// handles caching
void	(*Mod_TouchModel)			(char *name);

void	(*Mod_NowLoadExternal)		(void);
void	(*Mod_Think)				(void);
qboolean	(*Mod_GetTag)			(struct model_s *model, int tagnum, int frame, int frame2, float f2ness, float f1time, float f2time, float *transforms);
int (*Mod_TagNumForName)			(struct model_s *model, char *name);
int (*Mod_SkinForName)				(struct model_s *model, char *name);



qboolean (*VID_Init)				(rendererstate_t *info, unsigned char *palette);
void	 (*VID_DeInit)				(void);
void	(*VID_HandlePause)			(qboolean pause);
void	(*VID_LockBuffer)			(void);
void	(*VID_UnlockBuffer)			(void);
void	(*D_BeginDirectRect)		(int x, int y, qbyte *pbitmap, int width, int height);
void	(*D_EndDirectRect)			(int x, int y, int width, int height);
void	(*VID_ForceLockState)		(int lk);
int		(*VID_ForceUnlockedAndReturnState) (void);
void	(*VID_SetPalette)			(unsigned char *palette);
void	(*VID_ShiftPalette)			(unsigned char *palette);
char	*(*VID_GetRGBInfo)			(int prepad, int *truevidwidth, int *truevidheight);
void	(*VID_SetWindowCaption)		(char *msg);

void	(*SCR_UpdateScreen)			(void);

r_qrenderer_t qrenderer=-1;
char *q_renderername = "Non-Selected renderer";



rendererinfo_t dedicatedrendererinfo = {
	//ALL builds need a 'none' renderer, as 0.
	"Dedicated server",
	{
		"none",
		"dedicated",
		"terminal",
		"sv"
	},
	QR_NONE,

	NULL,	//Draw_PicFromWad;	//Not supported
	NULL,	//Draw_CachePic;
	NULL,	//Draw_SafeCachePic;
	NULL,	//Draw_Init;
	NULL,	//Draw_Init;
	NULL,	//Draw_Character;
	NULL,	//Draw_ColouredCharacter;
	NULL,	//Draw_String;
	NULL,	//Draw_Alt_String;
	NULL,	//Draw_Crosshair;
	NULL,	//Draw_DebugChar;
	NULL,	//Draw_Pic;
	NULL,	//Draw_SubPic;
	NULL,	//Draw_TransPic;
	NULL,	//Draw_TransPicTranslate;
	NULL,	//Draw_ConsoleBackground;
	NULL,	//Draw_EditorBackground;
	NULL,	//Draw_TileClear;
	NULL,	//Draw_Fill;
	NULL,   //Draw_FillRGB;
	NULL,	//Draw_FadeScreen;
	NULL,	//Draw_BeginDisc;
	NULL,	//Draw_EndDisc;
	NULL,	//I'm lazy.

	NULL,	//Draw_Image
	NULL,	//Draw_ImageColours

	NULL,	//R_Init;
	NULL,	//R_DeInit;
	NULL,	//R_ReInit;
	NULL,	//R_RenderView;

	NULL,	//R_CheckSky;
	NULL,	//R_SetSky;

	NULL,	//R_NewMap;
	NULL,	//R_PreNewMap
	NULL,	//R_LightPoint;
	NULL,	//R_PushDlights;


	NULL,	//R_AddStain;
	NULL,	//R_LessenStains;

	NULL,	//Media_ShowFrameBGR_24_Flip;
	NULL,	//Media_ShowFrameRGBA_32;
	NULL,	//Media_ShowFrame8bit;

#ifdef SWQUAKE
	SWMod_Init,
	SWMod_ClearAll,
	SWMod_ForName,
	SWMod_FindName,
	SWMod_Extradata,
	SWMod_TouchModel,

	SWMod_NowLoadExternal,
	SWMod_Think,
#elif defined(RGLQUAKE)
	GLMod_Init,
	GLMod_ClearAll,
	GLMod_ForName,
	GLMod_FindName,
	GLMod_Extradata,
	GLMod_TouchModel,

	GLMod_NowLoadExternal,
	GLMod_Think,
#else
	#error "No renderer in client build"
#endif

	NULL, //Mod_GetTag
	NULL, //fixme: server will need this one at some point.
	NULL,

	NULL, //VID_Init,
	NULL, //VID_DeInit,
	NULL, //VID_HandlePause,
	NULL, //VID_LockBuffer,
	NULL, //VID_UnlockBuffer,
	NULL, //D_BeginDirectRect,
	NULL, //D_EndDirectRect,
	NULL, //VID_ForceLockState,
	NULL, //VID_ForceUnlockedAndReturnState,
	NULL, //VID_SetPalette,
	NULL, //VID_ShiftPalette,
	NULL, //VID_GetRGBInfo,


	NULL,	//set caption

	NULL,	//SCR_UpdateScreen;

	""
};
rendererinfo_t *pdedicatedrendererinfo = &dedicatedrendererinfo;

#ifdef SWQUAKE
rendererinfo_t softwarerendererinfo = {
	"Software rendering",
	{
		"sw",
		"software",
	},
	QR_SOFTWARE,

	SWDraw_PicFromWad,
	SWDraw_CachePic,
	SWDraw_SafeCachePic,
	SWDraw_Init,
	SWDraw_Init,
	SWDraw_Character,
	SWDraw_ColouredCharacter,
	SWDraw_String,
	SWDraw_Alt_String,
	SWDraw_Crosshair,
	SWDraw_DebugChar,
	SWDraw_Pic,
	NULL,//SWDraw_ScaledPic,
	SWDraw_SubPic,
	SWDraw_TransPic,
	SWDraw_TransPicTranslate,
	SWDraw_ConsoleBackground,
	SWDraw_EditorBackground,
	SWDraw_TileClear,
	SWDraw_Fill,
	SWDraw_FillRGB,
	SWDraw_FadeScreen,
	SWDraw_BeginDisc,
	SWDraw_EndDisc,

	SWDraw_Image,
	SWDraw_ImageColours,

	SWR_Init,
	SWR_DeInit,
	NULL,//SWR_ReInit,
	SWR_RenderView,

	SWR_CheckSky,
	SWR_SetSky,

	SWR_NewMap,
	NULL,
	SWR_LightPoint,
	SWR_PushDlights,

	SWR_AddStain,
	SWR_LessenStains,

	MediaSW_ShowFrameBGR_24_Flip,
	MediaSW_ShowFrameRGBA_32,
	MediaSW_ShowFrame8bit,

	SWMod_Init,
	SWMod_ClearAll,
	SWMod_ForName,
	SWMod_FindName,
	SWMod_Extradata,
	SWMod_TouchModel,

	SWMod_NowLoadExternal,
	SWMod_Think,

	NULL,	//Mod_GetTag
	NULL,	//Mod_TagForName
	NULL,

	SWVID_Init,
	SWVID_Shutdown,
	SWVID_HandlePause,
	SWVID_LockBuffer,
	SWVID_UnlockBuffer,
	SWD_BeginDirectRect,
	SWD_EndDirectRect,
	SWVID_ForceLockState,
	SWVID_ForceUnlockedAndReturnState,
	SWVID_SetPalette,
	SWVID_ShiftPalette,
	SWVID_GetRGBInfo,

	NULL,

	SWSCR_UpdateScreen,

	""
};
rendererinfo_t *psoftwarerendererinfo = &softwarerendererinfo;
#endif
#ifdef RGLQUAKE
rendererinfo_t openglrendererinfo = {
	"OpenGL",
	{
		"gl",
		"opengl",
		"hardware",
	},
	QR_OPENGL,


	GLDraw_SafePicFromWad,
	GLDraw_CachePic,
	GLDraw_SafeCachePic,
	GLDraw_Init,
	GLDraw_ReInit,
	GLDraw_Character,
	GLDraw_ColouredCharacter,
	GLDraw_String,
	GLDraw_Alt_String,
	GLDraw_Crosshair,
	GLDraw_DebugChar,
	GLDraw_Pic,
	GLDraw_ScalePic,
	GLDraw_SubPic,
	GLDraw_TransPic,
	GLDraw_TransPicTranslate,
	GLDraw_ConsoleBackground,
	GLDraw_EditorBackground,
	GLDraw_TileClear,
	GLDraw_Fill,
	GLDraw_FillRGB,
	GLDraw_FadeScreen,
	GLDraw_BeginDisc,
	GLDraw_EndDisc,

	GLDraw_Image,
	GLDraw_ImageColours,

	GLR_Init,
	GLR_DeInit,
	GLR_ReInit,
	GLR_RenderView,


	GLR_CheckSky,
	GLR_SetSky,

	GLR_NewMap,
	GLR_PreNewMap,
	GLR_LightPoint,
	GLR_PushDlights,


	GLR_AddStain,
	GLR_LessenStains,

	MediaGL_ShowFrameBGR_24_Flip,
	MediaGL_ShowFrameRGBA_32,
	MediaGL_ShowFrame8bit,


	GLMod_Init,
	GLMod_ClearAll,
	GLMod_ForName,
	GLMod_FindName,
	GLMod_Extradata,
	GLMod_TouchModel,

	GLMod_NowLoadExternal,
	GLMod_Think,

	GLMod_GetTag,
	GLMod_TagNumForName,
	GLMod_SkinNumForName,

	GLVID_Init,
	GLVID_DeInit,
	GLVID_HandlePause,
	GLVID_LockBuffer,
	GLVID_UnlockBuffer,
	GLD_BeginDirectRect,
	GLD_EndDirectRect,
	GLVID_ForceLockState,
	GLVID_ForceUnlockedAndReturnState,
	GLVID_SetPalette,
	GLVID_ShiftPalette,
	GLVID_GetRGBInfo,

	NULL,	//setcaption


	GLSCR_UpdateScreen,

	""
};
rendererinfo_t *popenglrendererinfo = &openglrendererinfo;
#endif

rendererinfo_t *pd3drendererinfo;

rendererinfo_t **rendererinfo[] =
{
	&pdedicatedrendererinfo,
#ifdef SWQUAKE
	&psoftwarerendererinfo,
#endif
#ifdef RGLQUAKE
	&popenglrendererinfo,
	&pd3drendererinfo,
#endif
};




typedef struct vidmode_s
{
	const char *description;
	int         width, height;
} vidmode_t;

vidmode_t vid_modes[] =
{
	{ "320x200",   320, 200},
	{ "320x240",   320, 240},
	{ "400x300",   400, 300},
	{ "512x384",   512, 384},
	{ "640x480",   640, 480},
	{ "800x600",   800, 600},
	{ "960x720",   960, 720},
	{ "1024x768 (XGA: 14-15inch LCD Native)",  1024, 768},
	{ "1152x864",  1152, 864},
	{ "1280x960",  1280, 960},
	{ "1280x1024 (SXGA: 17-19inch LCD Native)", 1280, 1024},
	{ "1440x900 (WXGA: 19inch Widescreen LCD Native Resolution)", 1440, 900},
	{ "1600x1200 (UXGA: 20+inch LCD Native Resolution)", 1600, 1200},	//sw height is bound to 200 to 1024
	{ "1680x1050 (WSXGA: 20inch Widescreen LCD Native)", 1680, 1050},
	{ "1920x1200 (WUXGA: 24inch Widescreen LCD Native)", 1920, 1200},
	{ "2048x1536", 2048, 1536},	//too much width will disable water warping (>1280) (but at that resolution, it's almost unnoticable)
	{ "2560x1600 (30inch Widescreen LCD Native)", 2560, 1600}
};
#define NUMVIDMODES sizeof(vid_modes)/sizeof(vid_modes[0])

qboolean M_Vid_GetMove(int num, int *w, int *h)
{
	if ((unsigned)num >= NUMVIDMODES)
		return false;

	*w = vid_modes[num].width;
	*h = vid_modes[num].height;
	return true;
}

typedef struct {
	menucombo_t *renderer;
	menucombo_t *modecombo;
	menucombo_t *conscalecombo;
	menucombo_t *bppcombo;
	menucombo_t *texturefiltercombo;
	menuedit_t *customwidth;
	menuedit_t *customheight;
} videomenuinfo_t;

menuedit_t *MC_AddEdit(menu_t *menu, int x, int y, char *text, char *def);
void CheckCustomMode(struct menu_s *menu)
{
	videomenuinfo_t *info = menu->data;
	if (info->modecombo->selectedoption && info->conscalecombo->selectedoption)
	{	//hide the custom options
		info->customwidth->common.ishidden = true;
		info->customheight->common.ishidden = true;
	}
	else
	{
		info->customwidth->common.ishidden = false;
		info->customheight->common.ishidden = false;
	}

#ifdef SWQUAKE
	if (info->renderer->selectedoption < 1)
	{
		info->conscalecombo->common.ishidden = true;
	}
	else
#endif
	{
		if (!info->bppcombo->selectedoption)
			info->bppcombo->selectedoption = 1;

		info->conscalecombo->common.ishidden = false;
	}
}
qboolean M_VideoApply (union menuoption_s *op,struct menu_s *menu,int key)
{
	videomenuinfo_t *info = menu->data;
	int selectedbpp;

	if (key != K_ENTER)
		return false;

	if (info->modecombo->selectedoption)
	{	//set a prefab
		Cbuf_AddText(va("vid_width %i\n", vid_modes[info->modecombo->selectedoption-1].width), RESTRICT_LOCAL);
		Cbuf_AddText(va("vid_height %i\n", vid_modes[info->modecombo->selectedoption-1].height), RESTRICT_LOCAL);
	}
	else
	{	//use the custom one
		Cbuf_AddText(va("vid_width %s\n", info->customwidth->text), RESTRICT_LOCAL);
		Cbuf_AddText(va("vid_height %s\n", info->customheight->text), RESTRICT_LOCAL);
	}

	if (info->conscalecombo->selectedoption)	//I am aware that this handicaps the menu a bit, but it should be easier for n00bs.
	{	//set a prefab
		Cbuf_AddText(va("vid_conwidth %i\n", vid_modes[info->conscalecombo->selectedoption-1].width), RESTRICT_LOCAL);
		Cbuf_AddText(va("vid_conheight %i\n", vid_modes[info->conscalecombo->selectedoption-1].height), RESTRICT_LOCAL);
	}
	else
	{	//use the custom one
		Cbuf_AddText(va("vid_conwidth %s\n", info->customwidth->text), RESTRICT_LOCAL);
		Cbuf_AddText(va("vid_conheight %s\n", info->customheight->text), RESTRICT_LOCAL);
	}

	selectedbpp = 16;
	switch(info->bppcombo->selectedoption)
	{
	case 0:
		if (info->renderer->selectedoption)
			selectedbpp = 16;
		else
			selectedbpp = 8;
		break;
	case 1:
		selectedbpp = 16;
		break;
	case 2:
		selectedbpp = 32;
		break;
	}

	switch(info->texturefiltercombo->selectedoption)
	{
	case 0:
		Cbuf_AddText("gl_texturemode gl_nearest_mipmap_nearest\n", RESTRICT_LOCAL);
		break;
	case 1:
		Cbuf_AddText("gl_texturemode gl_linear_mipmap_nearest\n", RESTRICT_LOCAL);
		break;
	case 2:
		Cbuf_AddText("gl_texturemode gl_linear_mipmap_linear\n", RESTRICT_LOCAL);
		break;
	}

	Cbuf_AddText(va("vid_bpp %i\n", selectedbpp), RESTRICT_LOCAL);

	switch(info->renderer->selectedoption)
	{
#ifdef SWQUAKE
	case 0:
		Cbuf_AddText("setrenderer sw\n", RESTRICT_LOCAL);
		break;
	case 1:
#else
	case 0:
#endif
		Cbuf_AddText("setrenderer gl\n", RESTRICT_LOCAL);
		break;
#ifdef SWQUAKE
	case 2:
#else
	case 1:
#endif
		Cbuf_AddText("setrenderer d3d\n", RESTRICT_LOCAL);
		break;
	}
	M_RemoveMenu(menu);
	Cbuf_AddText("menu_video\n", RESTRICT_LOCAL);
	return true;
}
void M_Menu_Video_f (void)
{
	extern cvar_t r_stains, v_contrast;
#if defined(SWQUAKE)
	extern cvar_t d_smooth;
#endif
#if defined(RGLQUAKE)
	extern cvar_t r_bloom;
#endif
	extern cvar_t r_bouncysparks;
	static const char *modenames[128] = {"Custom"};
	static const char *rendererops[] = {
#ifdef SWQUAKE
		"Software",
#endif
#ifdef RGLQUAKE
		"OpenGL",
#ifdef USE_D3D
		"Direct3D",
#endif
#endif
		NULL
	};
	static const char *bppnames[] =
	{
		"8",
		"16",
		"32",
		NULL
	};
	static const char *texturefilternames[] =
	{
		"Nearest",
		"Bilinear",
		"Trilinear",
		NULL
	};

	videomenuinfo_t *info;
	menu_t *menu;
	int prefabmode;
	int prefab2dmode;
	int currentbpp;
#ifdef RGLQUAKE
	int currenttexturefilter;
#endif

	int i, y;
	prefabmode = -1;
	prefab2dmode = -1;
	for (i = 0; i < sizeof(vid_modes)/sizeof(vidmode_t); i++)
	{
		if (vid_modes[i].width == vid_width.value && vid_modes[i].height == vid_height.value)
			prefabmode = i;
		if (vid_modes[i].width == vid_conwidth.value && vid_modes[i].height == vid_conheight.value)
			prefab2dmode = i;
		modenames[i+1] = vid_modes[i].description;
	}
	modenames[i+1] = NULL;

	key_dest = key_menu;
	m_state = m_complex;

	menu = M_CreateMenu(sizeof(videomenuinfo_t));
	info = menu->data;

#if defined(SWQUAKE) && defined(RGLQUAKE)
	if (qrenderer == QR_OPENGL)
	{
#ifdef USE_D3D
		if (!strcmp(vid_renderer.string, "d3d"))
			i = 2;
		else
#endif
			i = 1;
	}
	else
#endif
#if defined(RGLQUAKE) && defined(USE_D3D)
		 if (!strcmp(vid_renderer.string, "d3d"))
		i = 1;
	else
#endif
		i = 0;

	if (vid_bpp.value >= 32)
		currentbpp = 2;
	else if (vid_bpp.value >= 16)
		currentbpp = 1;
	else
		currentbpp = 0;

#ifdef RGLQUAKE
	if (!Q_strcasecmp(gl_texturemode.string, "gl_nearest_mipmap_nearest"))
		currenttexturefilter = 0;
	else if (!Q_strcasecmp(gl_texturemode.string, "gl_linear_mipmap_linear"))
		currenttexturefilter = 2;
	else if (!Q_strcasecmp(gl_texturemode.string, "gl_linear_mipmap_nearest"))
		currenttexturefilter = 1;
	else
		currenttexturefilter = 1;
#endif


	MC_AddCenterPicture(menu, 4, "vidmodes");

	y = 32;
	info->renderer = MC_AddCombo(menu,	16, y,				"   Renderer     ", rendererops, i);	y+=8;
	info->bppcombo = MC_AddCombo(menu,	16, y,				"   Color Depth  ", bppnames, currentbpp); y+=8;
	info->modecombo = MC_AddCombo(menu,	16, y,				"   Video Size   ", modenames, prefabmode+1);	y+=8;
	info->conscalecombo = MC_AddCombo(menu,	16, y,			"      2d Size   ", modenames, prefab2dmode+1);	y+=8;
	MC_AddCheckBox(menu,	16, y,							"   Fullscreen   ", &vid_fullscreen,0);	y+=8;
	y+=4;info->customwidth = MC_AddEdit(menu, 16, y,		"   Custom width ", vid_width.string);	y+=8;
	y+=4;info->customheight = MC_AddEdit(menu, 16, y,		"   Custom height", vid_height.string);	y+=12;
	y+=8;
	MC_AddCommand(menu,	16, y,								"           Apply", M_VideoApply);	y+=8;
	y+=8;
	MC_AddCheckBox(menu,	16, y,							"      Stain maps", &r_stains,0);	y+=8;
	MC_AddCheckBox(menu,	16, y,							"   Bouncy sparks", &r_bouncysparks,0);	y+=8;
	MC_AddCheckBox(menu,	16, y,							"            Rain", &r_part_rain,0);	y+=8;
#if defined(SWQUAKE)
	MC_AddCheckBox(menu,	16, y,							"    SW Smoothing", &d_smooth,0);	y+=8;
#endif
#ifdef RGLQUAKE
	MC_AddCheckBox(menu,	16, y,							"  GL Bumpmapping", &gl_bump,0);	y+=8;
	MC_AddCheckBox(menu,	16, y,							"           Bloom", &r_bloom,0);	y+=8;
#endif
	MC_AddCheckBox(menu,	16, y,							"  Dynamic lights", &r_dynamic,0);	y+=8;
	MC_AddSlider(menu,	16, y,								"     Screen size", &scr_viewsize,	30,		120);y+=8;
	MC_AddSlider(menu,	16, y,								"           Gamma", &v_gamma, 0.3, 1);	y+=8;
	MC_AddSlider(menu,	16, y,								"        Contrast", &v_contrast, 1, 3);	y+=8;
#ifdef RGLQUAKE
	info->texturefiltercombo = MC_AddCombo(menu, 16, y,		" Texture Filter ", texturefilternames, currenttexturefilter); y+=8;
	MC_AddSlider(menu, 16, y,								"Anisotropy Level", &gl_texture_anisotropic_filtering, 1, 16); y+=8;
#endif

	menu->cursoritem = (menuoption_t*)MC_AddWhiteText(menu, 152, 32, NULL, false);
	menu->selecteditem = (union menuoption_s *)info->renderer;
	menu->event = CheckCustomMode;
}


void R_SetRenderer(int wanted)
{
	rendererinfo_t *ri;

	if (wanted<0)
	{	//-1 is used so we know when we've applied something instead of never setting anything.
		wanted=0;
		qrenderer = -1;
	}
	else
		qrenderer = (*rendererinfo[wanted])->rtype;

	ri = (*rendererinfo[wanted]);

	q_renderername = ri->name[0];


	Draw_SafePicFromWad		= ri->Draw_SafePicFromWad;	//Not supported
	Draw_CachePic			= ri->Draw_CachePic;
	Draw_SafeCachePic		= ri->Draw_SafeCachePic;
	Draw_Init				= ri->Draw_Init;
	Draw_ReInit				= ri->Draw_Init;
	Draw_Character			= ri->Draw_Character;
	Draw_ColouredCharacter	= ri->Draw_ColouredCharacter;
	Draw_String				= ri->Draw_String;
	Draw_Alt_String			= ri->Draw_Alt_String;
	Draw_Crosshair			= ri->Draw_Crosshair;
	Draw_DebugChar			= ri->Draw_DebugChar;
	Draw_Pic				= ri->Draw_Pic;
	Draw_SubPic				= ri->Draw_SubPic;
	Draw_TransPic			= ri->Draw_TransPic;
	Draw_TransPicTranslate	= ri->Draw_TransPicTranslate;
	Draw_ConsoleBackground	= ri->Draw_ConsoleBackground;
	Draw_EditorBackground	= ri->Draw_EditorBackground;
	Draw_TileClear			= ri->Draw_TileClear;
	Draw_Fill				= ri->Draw_Fill;
	Draw_FillRGB			= ri->Draw_FillRGB;
	Draw_FadeScreen			= ri->Draw_FadeScreen;
	Draw_BeginDisc			= ri->Draw_BeginDisc;
	Draw_EndDisc			= ri->Draw_EndDisc;
	Draw_ScalePic			= ri->Draw_ScalePic;

	Draw_Image				= ri->Draw_Image;
	Draw_ImageColours 		= ri->Draw_ImageColours;

	R_Init					= ri->R_Init;
	R_DeInit				= ri->R_DeInit;
	R_RenderView			= ri->R_RenderView;
	R_NewMap				= ri->R_NewMap;
	R_PreNewMap				= ri->R_PreNewMap;
	R_LightPoint			= ri->R_LightPoint;
	R_PushDlights			= ri->R_PushDlights;
	R_CheckSky				= ri->R_CheckSky;
	R_SetSky				= ri->R_SetSky;

	R_AddStain				= ri->R_AddStain;
	R_LessenStains			= ri->R_LessenStains;

	VID_Init				= ri->VID_Init;
	VID_DeInit				= ri->VID_DeInit;
	VID_HandlePause			= ri->VID_HandlePause;
	VID_LockBuffer			= ri->VID_LockBuffer;
	VID_UnlockBuffer		= ri->VID_UnlockBuffer;
	D_BeginDirectRect		= ri->D_BeginDirectRect;
	D_EndDirectRect			= ri->D_EndDirectRect;
	VID_ForceLockState		= ri->VID_ForceLockState;
	VID_ForceUnlockedAndReturnState	= ri->VID_ForceUnlockedAndReturnState;
	VID_SetPalette			= ri->VID_SetPalette;
	VID_ShiftPalette		= ri->VID_ShiftPalette;
	VID_GetRGBInfo			= ri->VID_GetRGBInfo;
	VID_SetWindowCaption	= ri->VID_SetWindowCaption;

	Media_ShowFrame8bit			= ri->Media_ShowFrame8bit;
	Media_ShowFrameRGBA_32		= ri->Media_ShowFrameRGBA_32;
	Media_ShowFrameBGR_24_Flip	= ri->Media_ShowFrameBGR_24_Flip;

	Mod_Init				= ri->Mod_Init;
	Mod_Think				= ri->Mod_Think;
	Mod_ClearAll			= ri->Mod_ClearAll;
	Mod_ForName				= ri->Mod_ForName;
	Mod_FindName			= ri->Mod_FindName;
	Mod_Extradata			= ri->Mod_Extradata;
	Mod_TouchModel			= ri->Mod_TouchModel;

	Mod_NowLoadExternal		= ri->Mod_NowLoadExternal;

	Mod_GetTag				= ri->Mod_GetTag;
	Mod_TagNumForName 		= ri->Mod_TagNumForName;
	Mod_SkinForName 		= ri->Mod_SkinForName;

	SCR_UpdateScreen		= ri->SCR_UpdateScreen;
}

static qbyte default_quakepal[768] =
{
0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,155,171,171,171,187,187,187,203,203,203,219,219,219,235,235,235,15,11,7,23,15,11,31,23,11,39,27,15,47,35,19,55,43,23,63,47,23,75,55,27,83,59,27,91,67,31,99,75,31,107,83,31,115,87,31,123,95,35,131,103,35,143,111,35,11,11,15,19,19,27,27,27,39,39,39,51,47,47,63,55,55,75,63,63,87,71,71,103,79,79,115,91,91,127,99,99,
139,107,107,151,115,115,163,123,123,175,131,131,187,139,139,203,0,0,0,7,7,0,11,11,0,19,19,0,27,27,0,35,35,0,43,43,7,47,47,7,55,55,7,63,63,7,71,71,7,75,75,11,83,83,11,91,91,11,99,99,11,107,107,15,7,0,0,15,0,0,23,0,0,31,0,0,39,0,0,47,0,0,55,0,0,63,0,0,71,0,0,79,0,0,87,0,0,95,0,0,103,0,0,111,0,0,119,0,0,127,0,0,19,19,0,27,27,0,35,35,0,47,43,0,55,47,0,67,
55,0,75,59,7,87,67,7,95,71,7,107,75,11,119,83,15,131,87,19,139,91,19,151,95,27,163,99,31,175,103,35,35,19,7,47,23,11,59,31,15,75,35,19,87,43,23,99,47,31,115,55,35,127,59,43,143,67,51,159,79,51,175,99,47,191,119,47,207,143,43,223,171,39,239,203,31,255,243,27,11,7,0,27,19,0,43,35,15,55,43,19,71,51,27,83,55,35,99,63,43,111,71,51,127,83,63,139,95,71,155,107,83,167,123,95,183,135,107,195,147,123,211,163,139,227,179,151,
171,139,163,159,127,151,147,115,135,139,103,123,127,91,111,119,83,99,107,75,87,95,63,75,87,55,67,75,47,55,67,39,47,55,31,35,43,23,27,35,19,19,23,11,11,15,7,7,187,115,159,175,107,143,163,95,131,151,87,119,139,79,107,127,75,95,115,67,83,107,59,75,95,51,63,83,43,55,71,35,43,59,31,35,47,23,27,35,19,19,23,11,11,15,7,7,219,195,187,203,179,167,191,163,155,175,151,139,163,135,123,151,123,111,135,111,95,123,99,83,107,87,71,95,75,59,83,63,
51,67,51,39,55,43,31,39,31,23,27,19,15,15,11,7,111,131,123,103,123,111,95,115,103,87,107,95,79,99,87,71,91,79,63,83,71,55,75,63,47,67,55,43,59,47,35,51,39,31,43,31,23,35,23,15,27,19,11,19,11,7,11,7,255,243,27,239,223,23,219,203,19,203,183,15,187,167,15,171,151,11,155,131,7,139,115,7,123,99,7,107,83,0,91,71,0,75,55,0,59,43,0,43,31,0,27,15,0,11,7,0,0,0,255,11,11,239,19,19,223,27,27,207,35,35,191,43,
43,175,47,47,159,47,47,143,47,47,127,47,47,111,47,47,95,43,43,79,35,35,63,27,27,47,19,19,31,11,11,15,43,0,0,59,0,0,75,7,0,95,7,0,111,15,0,127,23,7,147,31,7,163,39,11,183,51,15,195,75,27,207,99,43,219,127,59,227,151,79,231,171,95,239,191,119,247,211,139,167,123,59,183,155,55,199,195,55,231,227,87,127,191,255,171,231,255,215,255,255,103,0,0,139,0,0,179,0,0,215,0,0,255,0,0,255,243,147,255,247,199,255,255,255,159,91,83
};
qbyte default_conchar[11356] =
{
#include "lhfont.h"
};

qboolean R_ApplyRenderer (rendererstate_t *newr)
{
	int i, j;
	extern model_t *loadmodel;
	extern int host_hunklevel;

	if (newr->bpp == -1)
		return false;

	CL_AllowIndependantSendCmd(false);	//FIXME: figure out exactly which parts are going to affect the model loading.

	IN_Shutdown();

	if (R_DeInit)
	{
		TRACE(("dbg: R_ApplyRenderer: R_DeInit\n"));
		R_DeInit();
	}

	if (VID_DeInit)
	{
		TRACE(("dbg: R_ApplyRenderer: VID_DeInit\n"));
		VID_DeInit();
	}

	TRACE(("dbg: R_ApplyRenderer: SCR_DeInit\n"));
	SCR_DeInit();

	COM_FlushTempoaryPacks();

	S_Shutdown();

	if (qrenderer == QR_NONE || qrenderer==-1)
	{
		if (newr->renderer == QR_NONE && qrenderer != -1)
			return true;	//no point

		Sys_CloseTerminal ();
	}

	R_SetRenderer(newr->renderer);

	Cache_Flush();

	Hunk_FreeToLowMark(host_hunklevel);	//is this a good idea?

	TRACE(("dbg: R_ApplyRenderer: old renderer closed\n"));

	pmove.numphysent = 0;

	if (qrenderer)	//graphics stuff only when not dedicated
	{
		qbyte *data;
#ifndef CLIENTONLY
		isDedicated = false;
#endif
		Con_Printf("Setting mode %i*%i*%i*%i\n", newr->width, newr->height, newr->bpp, newr->rate);

		if (host_basepal)
			BZ_Free(host_basepal);
		if (host_colormap)
			BZ_Free(host_colormap);
		host_basepal = (qbyte *)COM_LoadMallocFile ("gfx/palette.lmp");
		if (!host_basepal)
		{
			qbyte *pcx=NULL;
			host_basepal = BZ_Malloc(768);
			pcx = COM_LoadTempFile("pics/colormap.pcx");
			if (!pcx || !ReadPCXPalette(pcx, com_filesize, host_basepal))
			{
				memcpy(host_basepal, default_quakepal, 768);
			}
			else
			{
				host_colormap = BZ_Malloc(256*VID_GRADES);
				if (ReadPCXData(pcx, com_filesize, 256, VID_GRADES, host_colormap))
					goto q2colormap;	//skip the colormap.lmp file as we already read it
			}
		}
		host_colormap = (qbyte *)COM_LoadMallocFile ("gfx/colormap.lmp");
		if (!host_colormap)
		{
#ifdef SWQUAKE
			float f;
			if (qrenderer == QR_SOFTWARE) //glquake doesn't care
			{
				data = host_colormap = BZ_Malloc(256*VID_GRADES+sizeof(int));
				//let's try making one. this is probably caused by running out of baseq2.
				for (j = 0; j < VID_GRADES; j++)
				{
					f = 1 - ((float)j/VID_GRADES);
					for (i = 0; i < 256-vid.fullbright; i++)
					{
						data[i] = GetPalette(host_basepal[i*3+0]*f, host_basepal[i*3+1]*f, host_basepal[i*3+2]*f);
					}
					for (; i < 256; i++)
						data[i] = i;
					data+=256;
				}
			}
#endif

			vid.fullbright=0;
		}
		else
		{
			j = VID_GRADES-1;
			data = host_colormap + j*256;
			vid.fullbright=0;
			for (i = 255; i >= 0; i--)
			{
				if (host_colormap[i] == data[i])
					vid.fullbright++;
				else
					break;
			}
		}

		if (vid.fullbright < 2)
			vid.fullbright = 0;	//transparent colour doesn't count.

q2colormap:

TRACE(("dbg: R_ApplyRenderer: Palette loaded\n"));

		if (!VID_Init(newr, host_basepal))
		{
			return false;
		}
TRACE(("dbg: R_ApplyRenderer: vid applied\n"));

		W_LoadWadFile("gfx.wad");
TRACE(("dbg: R_ApplyRenderer: wad loaded\n"));
		Draw_Init();
TRACE(("dbg: R_ApplyRenderer: draw inited\n"));
		R_Init();
TRACE(("dbg: R_ApplyRenderer: renderer inited\n"));
		SCR_Init();
TRACE(("dbg: R_ApplyRenderer: screen inited\n"));
		Sbar_Flush();

		IN_ReInit();
	}
	else
	{
#ifdef CLIENTONLY
		Sys_Error("Tried setting dedicated mode\n");
		//we could support this, but there's no real reason to actually do so.

		//fixme: despite the checks in the setrenderer command, we can still get here via a config using vid_renderer.
#else
TRACE(("dbg: R_ApplyRenderer: isDedicated = true\n"));
		isDedicated = true;
		if (cls.state)
		{
			int os = sv.state;
			sv.state = ss_dead;	//prevents server from being killed off too.
			CL_Disconnect();
			sv.state = os;
		}
		Sys_InitTerminal();
		Con_PrintToSys();
#endif
	}
TRACE(("dbg: R_ApplyRenderer: initing mods\n"));
	Mod_Init();
TRACE(("dbg: R_ApplyRenderer: initing bulletein boards\n"));
	WipeBulletenTextures();

//	host_hunklevel = Hunk_LowMark();

	if (R_PreNewMap)
	if (cl.worldmodel)
	{
		TRACE(("dbg: R_ApplyRenderer: R_PreNewMap (how handy)\n"));
		R_PreNewMap();
	}

#ifndef CLIENTONLY
	if (sv.worldmodel)
	{
		edict_t *ent;
#ifdef Q2SERVER
		q2edict_t *q2ent;
#endif

TRACE(("dbg: R_ApplyRenderer: reloading server map\n"));
		sv.worldmodel = Mod_ForName (sv.modelname, false);
TRACE(("dbg: R_ApplyRenderer: loaded\n"));
		if (sv.worldmodel->needload)
		{
			SV_Error("Bsp went missing on render restart\n");
		}
TRACE(("dbg: R_ApplyRenderer: doing that funky phs thang\n"));
		SV_CalcPHS ();

TRACE(("dbg: R_ApplyRenderer: clearing world\n"));
		SV_ClearWorld ();

		if (svs.gametype == GT_PROGS)
		{
			for (i = 0; i < MAX_MODELS; i++)
			{
				if (sv.strings.model_precache[i] && *sv.strings.model_precache[i] && (!strcmp(sv.strings.model_precache[i] + strlen(sv.strings.model_precache[i]) - 4, ".bsp") || i-1 < sv.worldmodel->numsubmodels))
					sv.models[i] = Mod_FindName(sv.strings.model_precache[i]);
				else
					sv.models[i] = NULL;
			}

			ent = sv.edicts;
			ent->v->model = PR_SetString(svprogfuncs, sv.worldmodel->name);	//FIXME: is this a problem for normal ents?
			for (i=0 ; i<sv.num_edicts ; i++)
			{
				ent = EDICT_NUM(svprogfuncs, i);
				if (!ent)
					continue;
				if (ent->isfree)
					continue;

				if (ent->area.prev)
				{
					ent->area.prev = ent->area.next = NULL;
					SV_LinkEdict (ent, false);	// relink ents so touch functions continue to work.
				}
			}
		}
#ifdef Q2SERVER
		else if (svs.gametype == GT_QUAKE2)
		{
			for (i = 0; i < MAX_MODELS; i++)
			{
				if (sv.strings.configstring[Q2CS_MODELS+i] && *sv.strings.configstring[Q2CS_MODELS+i] && (!strcmp(sv.strings.configstring[Q2CS_MODELS+i] + strlen(sv.strings.configstring[Q2CS_MODELS+i]) - 4, ".bsp") || i-1 < sv.worldmodel->numsubmodels))
					sv.models[i] = Mod_FindName(sv.strings.configstring[Q2CS_MODELS+i]);
				else
					sv.models[i] = NULL;
			}

			q2ent = ge->edicts;
			for (i=0 ; i<ge->num_edicts ; i++, q2ent = (q2edict_t *)((char *)q2ent + ge->edict_size))
			{
				if (!q2ent)
					continue;
				if (!q2ent->inuse)
					continue;

				if (q2ent->area.prev)
				{
					q2ent->area.prev = q2ent->area.next = NULL;
					SVQ2_LinkEdict (q2ent);	// relink ents so touch functions continue to work.
				}
			}
		}
		else
			SV_UnspawnServer();
#endif
	}
#endif
#ifdef PLUGINS
	Plug_ResChanged();
#endif

TRACE(("dbg: R_ApplyRenderer: starting on client state\n"));
	if (cl.worldmodel)
	{
		int staticmodelindex[MAX_STATIC_ENTITIES];

		for (i = 0; i < cl.num_statics; i++)	//static entities contain pointers to the model index.
		{
			staticmodelindex[i] = 0;
			for (j = 1; j < MAX_MODELS; j++)
				if (cl_static_entities[i].model == cl.model_precache[j])
				{
					staticmodelindex[i] = j;
					break;
				}
		}

		cl.worldmodel = NULL;
		cl_numvisedicts=0;
TRACE(("dbg: R_ApplyRenderer: reloading ALL models\n"));
		for (i=1 ; i<MAX_MODELS ; i++)
		{
			if (!cl.model_name[i][0])
				break;

			cl.model_precache[i] = NULL;
			TRACE(("dbg: R_ApplyRenderer: reloading model %s\n", cl.model_name[i]));
			cl.model_precache[i] = Mod_ForName (cl.model_name[i], false);

			if (!cl.model_precache[i])
			{
				Con_Printf ("\nThe required model file '%s' could not be found.\n\n"
					, cl.model_name[i]);
				Con_Printf ("You may need to download or purchase a client "
					"pack in order to play on this server.\n\n");
				CL_Disconnect ();
#ifdef VM_UI
				UI_Reset();
#endif
				return false;
			}
		}
#ifdef CSQC_DAT
		for (i=1 ; i<MAX_CSQCMODELS ; i++)
		{
			if (!cl.model_csqcname[i][0])
				break;

			cl.model_csqcprecache[i] = NULL;
			TRACE(("dbg: R_ApplyRenderer: reloading csqc model %s\n", cl.model_csqcname[i]));
			cl.model_csqcprecache[i] = Mod_ForName (cl.model_csqcname[i], false);

			if (!cl.model_csqcprecache[i])
			{
				Con_Printf ("\nThe required model file '%s' could not be found.\n\n"
					, cl.model_csqcname[i]);
				Con_Printf ("You may need to download or purchase a client "
					"pack in order to play on this server.\n\n");
				CL_Disconnect ();
#ifdef VM_UI
				UI_Reset();
#endif
				return false;
			}
		}
#endif

		loadmodel = cl.worldmodel = cl.model_precache[1];
TRACE(("dbg: R_ApplyRenderer: done the models\n"));
		if (loadmodel->needload)
		{
				CL_Disconnect ();
#ifdef VM_UI
				UI_Reset();
#endif
				memcpy(&currentrendererstate, newr, sizeof(currentrendererstate));
				return true;
		}

TRACE(("dbg: R_ApplyRenderer: checking any wad textures\n"));
		Mod_NowLoadExternal();
TRACE(("dbg: R_ApplyRenderer: R_NewMap\n"));
		R_NewMap();
TRACE(("dbg: R_ApplyRenderer: efrags\n"));
		for (i = 0; i < cl.num_statics; i++)	//make the static entities reappear.
		{
			cl_static_entities[i].model = cl.model_precache[staticmodelindex[i]];
#ifdef SWQUAKE
			cl_static_entities[i].palremap = D_IdentityRemap();
#endif
			if (staticmodelindex[i])	//make sure it's worthwhile.
			{
				R_AddEfrags(&cl_static_entities[i]);
			}
		}
	}
#ifdef VM_UI
	else
		UI_Reset();
#endif

	switch (qrenderer)
	{
	case QR_NONE:
		Con_Printf(	"\n"
					"-----------------------------\n"
					"Dedicated console created\n");
		break;

	case QR_SOFTWARE:
		Con_Printf(	"\n"
					"-----------------------------\n"
					"Software renderer initialized\n");
		break;

	case QR_OPENGL:
		Con_Printf(	"\n"
					"-----------------------------\n"
					"OpenGL renderer initialized\n");
		break;
	}

	TRACE(("dbg: R_ApplyRenderer: S_Restart_f\n"));
	if (!isDedicated)
		S_DoRestart();

	TRACE(("dbg: R_ApplyRenderer: done\n"));

	memcpy(&currentrendererstate, newr, sizeof(currentrendererstate));
	return true;
}

void R_RestartRenderer_f (void)
{
	int i, j;
	rendererstate_t oldr;
	rendererstate_t newr;
#ifdef MENU_DAT
	MP_Shutdown();
#endif
	memset(&newr, 0, sizeof(newr));

TRACE(("dbg: R_RestartRenderer_f\n"));

	Media_CaptureDemoEnd();

	Cvar_ApplyLatches(CVAR_RENDERERLATCH);

	newr.width = vid_width.value;
	newr.height = vid_height.value;

	newr.allow_modex = vid_allow_modex.value;

	newr.multisample = vid_multisample.value;
	newr.bpp = vid_bpp.value;
	newr.fullscreen = vid_fullscreen.value;
	newr.rate = vid_refreshrate.value;

	Q_strncpyz(newr.glrenderer, gl_driver.string, sizeof(newr.glrenderer));

	newr.renderer = -1;
	for (i = 0; i < sizeof(rendererinfo)/sizeof(rendererinfo[0]); i++)
	{
		if (!*rendererinfo[i])
			continue;	//not valid in this build. :(
		for (j = 4-1; j >= 0; j--)
		{
			if (!(*rendererinfo[i])->name[j])
				continue;
			if (!stricmp((*rendererinfo[i])->name[j], vid_renderer.string))
			{
				newr.renderer = i;
				break;
			}
		}
	}
	if (newr.renderer == -1)
	{
		Con_Printf("vid_renderer unset or invalid. Using default.\n");
		//gotta do this after main hunk is saved off.
#if defined(RGLQUAKE) && defined(SWQUAKE)
		Cmd_ExecuteString("setrenderer sw 8\n", RESTRICT_LOCAL);
		Cbuf_AddText("menu_video\n", RESTRICT_LOCAL);

#elif defined(RGLQUAKE)
		Cmd_ExecuteString("setrenderer gl\n", RESTRICT_LOCAL);
#else
		Cmd_ExecuteString("setrenderer sw\n", RESTRICT_LOCAL);
#endif
		return;
	}

	// use desktop settings if set to 0 and not dedicated
	if (newr.renderer != QR_NONE)
	{
		int dbpp, dheight, dwidth, drate;

		if (!Sys_GetDesktopParameters(&dwidth, &dheight, &dbpp, &drate))
		{
			// force default values for systems not supporting desktop parameters
			dwidth = 640;
			dheight = 480;
			if (newr.renderer == QR_SOFTWARE) // hack for software default
				dbpp = 8;
			else
				dbpp = 32;
		}

		if (vid_desktopsettings.value)
		{
			newr.width = dwidth;
			newr.height = dheight;
			newr.bpp = dbpp;
			newr.rate = drate;
		}
		else
		{
			if (newr.width <= 0 || newr.height <= 0)
			{
				newr.width = dwidth;
				newr.height = dheight;
			}

			if (newr.bpp <= 0)
				newr.bpp = dbpp;
		}
	}

	TRACE(("dbg: R_RestartRenderer_f renderer %i\n", newr.renderer));

	memcpy(&oldr, &currentrendererstate, sizeof(rendererstate_t));
	if (!R_ApplyRenderer(&newr))
	{
		TRACE(("dbg: R_RestartRenderer_f failed\n"));
		if (R_ApplyRenderer(&oldr))
		{
			TRACE(("dbg: R_RestartRenderer_f old restored\n"));
			Con_Printf(S_ERROR "Video mode switch failed. Old mode restored.\n");	//go back to the old mode, the new one failed.
		}
		else
		{
			qboolean failed = true;

			if (newr.rate != 0)
			{
				Con_Printf(S_NOTICE "Trying default refresh rate\n");
				newr.rate = 0;
				failed = !R_ApplyRenderer(&newr);
			}

			if (failed)
			{
				newr.renderer = QR_NONE;
				if (R_ApplyRenderer(&newr))
				{
					TRACE(("dbg: R_RestartRenderer_f going to dedicated\n"));
					Con_Printf(S_ERROR "Video mode switch failed. Old mode wasn't supported either. Console forced.\nChange vid_width, vid_height, vid_bpp, vid_displayfrequency to a compatable mode, and then use the setrenderer command.\n");
				}
				else
					Sys_Error("Couldn't fall back to previous renderer\n");
			}
		}
	}

	Cvar_ApplyCallbacks(CVAR_RENDERERCALLBACK);
	SCR_EndLoadingPlaque();

	TRACE(("dbg: R_RestartRenderer_f success\n"));
#ifdef MENU_DAT
	MP_Init();
#endif
}

void R_SetRenderer_f (void)
{
	int i, j;
	int best;
	char *param = Cmd_Argv(1);
	if (Cmd_Argc() == 1 || !stricmp(param, "help"))
	{
		Con_Printf ("\nValid setrenderer parameters are:\n");
		for (i = 0; i < sizeof(rendererinfo)/sizeof(rendererinfo[0]); i++)
		{
			if ((*rendererinfo[i]))
				Con_Printf("%s: %s\n", (*rendererinfo[i])->name[0], (*rendererinfo[i])->description);
		}
		return;
	}

	best = -1;
	for (i = 0; i < sizeof(rendererinfo)/sizeof(rendererinfo[0]); i++)
	{
		if (!*rendererinfo[i])
			continue;	//not valid in this build. :(
		for (j = 4-1; j >= 0; j--)
		{
			if (!(*rendererinfo[i])->name[j])
				continue;
			if (!stricmp((*rendererinfo[i])->name[j], param))
			{
				best = i;
				break;
			}
		}
	}

#ifdef CLIENTONLY
	if (best == 0)
	{
		Con_Printf("Client-only builds cannot use dedicated modes.\n");
		return;
	}
#endif

	if (best == -1)
	{
		Con_Printf("setrenderer: parameter not supported (%s)\n", param);
		return;
	}
	else
	{
		if (Cmd_Argc() == 3)
			Cvar_Set(&vid_bpp, Cmd_Argv(2));
	}

	Cvar_Set(&vid_renderer, param);
	R_RestartRenderer_f();
}







