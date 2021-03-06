CC=gcc
WINDRES=windres
STRIP=strip

STRIPFLAGS=--strip-unneeded --remove-section=.comment

CPUOPTIMIZATIONS=

BASE_DIR=.

SVNREVISION:=-DSVNREVISION=$(shell test -d .svn && svnversion || echo -)

#only limited forms of cross-making is supported
#only the following 3 are supported
#linux->win32 (FTE_TARGET=win32) RPM Package: "mingw32-gcc", DEB Package: "mingw32"
#linux->linux32 (FTE_TARGET=linux32)
#linux->linux64 (FTE_TARGET=linux64)
#linux->morphos (FTE_TARGET=morphos) Contact Moodles for crosscompiler
#linux->macosx (FTE_TARGET=macosx) or (FTE_TARGET=macosx_x86) Contact Moodles for crosscompiler, only works with Mac OSX 10.4 Tiger.
#if you are cross compiling, you'll need to use FTE_TARGET=mytaget

#correct the gcc build when cross compiling
ifeq ($(FTE_TARGET),win32)
	ifeq ($(shell $(CC) -v 2>&1 | grep mingw),)
		#CC didn't state that it was mingw... so try fixing that up
		ifneq ($(shell which i586-mingw32msvc-gcc 2> /dev/null),)
			#yup, the alternative exists (this matches the one debian has)
			CC=i586-mingw32msvc-gcc
			WINDRES=i586-mingw32msvc-windres
			STRIP=i586-mingw32msvc-strip
#			BITS?=32
		endif
	endif
endif

#correct the gcc build when cross compiling
ifeq ($(FTE_TARGET),win64)
	ifeq ($(shell $(CC) -v 2>&1 | grep mingw),)
		#CC didn't state that it was mingw... so try fixing that up
		ifneq ($(shell which amd64-mingw32msvc-gcc 2> /dev/null),)
			#yup, the alternative exists (this matches the one debian has)
			CC=amd64-mingw32msvc-gcc -m64
			WINDRES=amd64-mingw32msvc-windres
			STRIP=amd64-mingw32msvc-strip
#			BITS?=64
		endif
	endif
endif

#correct the gcc build when cross compiling
ifeq ($(FTE_TARGET),win32_SDL)
	ifeq ($(shell $(CC) -v 2>&1 | grep mingw),)
		#CC didn't state that it was mingw... so try fixing that up
		ifneq ($(shell which i586-mingw32msvc-gcc 2> /dev/null),)
			#yup, the alternative exists (this matches the one debian has)
			CC=i586-mingw32msvc-gcc
			WINDRES=i586-mingw32msvc-windres
			STRIP=i586-mingw32msvc-strip
#			BITS?=32
		endif
	endif
endif

#correct the gcc build when cross compiling
ifeq ($(FTE_TARGET),win64_SDL)
	ifeq ($(shell $(CC) -v 2>&1 | grep mingw),)
		#CC didn't state that it was mingw... so try fixing that up
		ifneq ($(shell which amd64-mingw32msvc-gcc 2> /dev/null),)
			#yup, the alternative exists (this matches the one debian has)
			CC=amd64-mingw32msvc-gcc -m64
			WINDRES=amd64-mingw32msvc-windres
			STRIP=amd64-mingw32msvc-strip
#			BITS?=64
		endif
	endif
endif

#crosscompile macosx from linux, default target ppc 32bit
ifeq ($(FTE_TARGET),macosx)
	ifeq ($(shell $(CC) -v 2>&1 | grep apple),)
		ifneq ($(shell which powerpc-apple-darwin8-gcc 2> /dev/null),)
			CC=powerpc-apple-darwin8-gcc
			STRIP=powerpc-apple-darwin8-strip
			#seems, macosx has a more limited version of strip
			STRIPFLAGS=
			BITS=32
			EXTENSION=_ppc
		endif
	endif
endif

ifeq ($(FTE_TARGET),macosx_ppc64)
	ifeq ($(shell $(CC) -v 2>&1 | grep apple),)
		ifneq ($(shell which powerpc-apple-darwin8-gcc 2> /dev/null),)
			FTE_TARGET=macosx
			CC=powerpc-apple-darwin8-gcc -arch ppc64
			STRIP=powerpc-apple-darwin8-strip
			#seems, macosx has a more limited version of strip
			STRIPFLAGS=
			BITS=64
			EXTENSION=_ppc
		endif
	endif
endif

ifeq ($(FTE_TARGET),macosx_x86)
	ifeq ($(shell $(CC) -v 2>&1 | grep apple),)
		ifneq ($(shell which i686-apple-darwin8-gcc 2> /dev/null),)
			FTE_TARGET=macosx
			# i686-apple-darwin8-gcc's default target is i386, powerpc-apple-darwin8-gcc -arch i386 just invokes i686-apple-darwin8-gcc anyway
			CC=i686-apple-darwin8-gcc
			STRIP=i686-apple-darwin8-strip
			#seems, macosx has a more limited version of strip
			STRIPFLAGS=
			EXTENSION=_x86
		endif
	endif
endif

#crosscompile morphos from linux
ifeq ($(FTE_TARGET),morphos)
	ifeq ($(shell $(CC) -v 2>&1 | grep morphos),)
		ifneq ($(shell which ppc-morphos-gcc 2> /dev/null),)
			CC=ppc-morphos-gcc
			#morphos strip has a 'feature', it strips permissions
			STRIP=ppc-morphos-strip
		endif
	endif
endif

#if you have an x86, you can get gcc to build 32bit or 64bit specific builds, instead of builds for the native platform
ifeq ($(FTE_TARGET),linux32)
	FTE_TARGET=linux
	CC=gcc -m32
	BITS=32
endif
ifeq ($(FTE_TARGET),linux64)
	FTE_TARGET=linux
	CC=gcc -m64
	BITS=64
endif


ifeq ($(FTE_TARGET),) 	#user didn't specify prefered target
	ifneq ($(shell $(CC) -v 2>&1 | grep mingw),)
		FTE_TARGET=win32
	endif
	ifneq ($(shell $(CC) -v 2>&1 | grep cygwin),)
		FTE_TARGET=cygwin
	endif
	ifeq ($(FTE_TARGET),) 	#still not set

		ifeq ($(shell uname),Linux)
			FTE_TARGET=linux
		endif
		ifeq ($(shell uname),Darwin)
			FTE_TARGET=macosx
		endif
		ifeq ($(shell uname),FreeBSD)
			FTE_TARGET=bsd
		endif
		ifeq ($(shell uname),NetBSD)
			FTE_TARGET=bsd
		endif
		ifeq ($(shell uname),OpenBSD)
			FTE_TARGET=bsd
		endif
		ifeq ($(shell uname),MorphOS)
			FTE_TARGET=morphos
		endif
		#else I've no idea what it is you're running
	endif

	FTE_TARGET ?= sdl	#so go for sdl.
endif

ifneq ($(shell ls|grep config.h),)
	HAVECONFIG=-DHAVE_CONFIG_H
endif

CLIENT_DIR=$(BASE_DIR)/client
GL_DIR=$(BASE_DIR)/gl
D3D_DIR=$(BASE_DIR)/d3d
SERVER_DIR=$(BASE_DIR)/server
COMMON_DIR=$(BASE_DIR)/common
HTTP_DIR=$(BASE_DIR)/http
#LIBS_DIR=$(BASE_DIR)/libs
LIBS_DIR?=.
PROGS_DIR=$(BASE_DIR)/qclib
SNDCODEC_DIR=$(BASE_DIR)/sndcodec

RELEASE_DIR=$(BASE_DIR)/release
DEBUG_DIR=$(BASE_DIR)/debug
PROFILE_DIR=$(BASE_DIR)/profile

ALL_CFLAGS=$(HAVECONFIG) $(CFLAGS) $(BASE_CFLAGS) $(WCFLAGS)

#cheap compile-everything-in-one-unit (compile becomes preprocess only)
ifneq ($(WPO),)
	LTO_CC= -E
	LTO_LD= -combine -fwhole-program -x c
	LTO_END=ltoxnone
	LTO_START=ltoxc
endif
#proper/consistant link-time optimisations (requires gcc 4.5+ or so)
ifneq ($(LTO),)
	LTO_CC=-flto
	LTO_LD=-flto
endif

DO_CC=@echo $< && $(CC) $(LTO_CC) $(ALL_CFLAGS) -o $@ -c $<

ifeq ($(FTE_TARGET),vc)
	BASELDFLAGS=
endif
ifeq ($(FTE_TARGET),win32)
	BASELDFLAGS=-lm
endif
ifeq ($(FTE_TARGET),win64)
	BASELDFLAGS=-lm
endif
ifeq ($(FTE_TARGET),win32_SDL)
	BASELDFLAGS=-lm
endif
ifeq ($(FTE_TARGET),win64_SDL)
	BASELDFLAGS=-lm
endif
ifeq ($(FTE_TARGET),cygwin)
	BASELDFLAGS=-lm
endif
ifeq ($(FTE_TARGET),morphos)
	BASELDFLAGS=-lm
endif
ifeq ($(FTE_TARGET),bsd)
	BASELDFLAGS=-lm
endif
BASELDFLAGS ?= -lm -ldl -lpthread

ifeq ($(FTE_TARGET),win32)
	IMAGELDFLAGS=$(LIBS_DIR)/mingw-libs/libpng.a $(LIBS_DIR)/mingw-libs/libz.a $(LIBS_DIR)/mingw-libs/libjpeg.a
endif
ifeq ($(FTE_TARGET),win64)
	IMAGELDFLAGS=$(LIBS_DIR)/mingw64-libs/libpng.a $(LIBS_DIR)/mingw64-libs/libz.a $(LIBS_DIR)/mingw64-libs/libjpeg.a
endif
ifeq ($(FTE_TARGET),win32_SDL)
	IMAGELDFLAGS=./libs/mingw-libs/libpng.a ./libs/mingw-libs/libz.a ./libs/mingw-libs/libjpeg.a
endif
ifeq ($(FTE_TARGET),win64_SDL)
	IMAGELDFLAGS=./libs/mingw64-libs/libpng.a ./libs/mingw64-libs/libz.a ./libs/mingw64-libs/libjpeg.a
endif
IMAGELDFLAGS ?= -lpng -ljpeg

#BASELDFLAGS=-lm  -lz
GLXLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext $(IMAGELDFLAGS)
GLSLDFLAGS=-L/usr/X11R6/lib -lMesaGL -lglide -lvga
XLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext $(IMAGELDFLAGS)
SLDFLAGS=-lvga

ifeq ($(FTE_TARGET),vc)
	WARNINGFLAGS=-W3 -D_CRT_SECURE_NO_WARNINGS
	GNUC_FUNCS=
else
	WARNINGFLAGS=-Wall -Wno-pointer-sign

	GNUC_FUNCS= -Dstrnicmp=strncasecmp -Dstricmp=strcasecmp -D_vsnprintf=vsnprintf -D_snprintf=snprintf
endif

BASE_CFLAGS=$(WARNINGFLAGS) $(GNUC_FUNCS) -I$(CLIENT_DIR) -I$(SERVER_DIR) -I$(COMMON_DIR) -I$(GL_DIR) -I$(D3D_DIR) -I$(PROGS_DIR) -I$(LIBS_DIR) -I$(LIBS_DIR)/dxsdk9/include -I$(LIBS_DIR)/sdl/include -I/usr/include/SDL -I$(LIBS_DIR)/sdl/include/SDL -I./libs/freetype2/include -I./libs/freetype2/include/freetype -I./libs/speex $(SVNREVISION)
CLIENT_ONLY_CFLAGS=-DCLIENTONLY
SERVER_ONLY_CFLAGS=-DSERVERONLY
JOINT_CFLAGS=
DEBUG_CFLAGS=-ggdb -g
DEBUG_CFLAGS=-rdynamic
PROFILE_CFLAGS=-pg

ifeq ($(FTE_TARGET),win32)
	# Let's make sure we're not chasing our tails with MinGW32 compiler bugs
	#RELEASE_CFLAGS=-fno-strict-aliasing -ffast-math -fexpensive-optimizations $(CPUOPTIMIZATIONS)
	RELEASE_CFLAGS=$(CPUOPTIMIZATIONS)
endif
ifeq ($(FTE_TARGET),win64)
	# Let's make sure we're not chasing our tails with MinGW32 compiler bugs
	#RELEASE_CFLAGS=-fno-strict-aliasing -ffast-math -fexpensive-optimizations $(CPUOPTIMIZATIONS)
	RELEASE_CFLAGS=$(CPUOPTIMIZATIONS)
endif
ifeq ($(FTE_TARGET),win32_SDL)
	# Let's make sure we're not chasing our tails with MinGW32 compiler bugs
	#RELEASE_CFLAGS=-fno-strict-aliasing -ffast-math -fexpensive-optimizations $(CPUOPTIMIZATIONS) -D_SDL
	RELEASE_CFLAGS=$(CPUOPTIMIZATIONS) -D_SDL
endif
ifeq ($(FTE_TARGET),win64_SDL)
	# Let's make sure we're not chasing our tails with MinGW32 compiler bugs
	#RELEASE_CFLAGS=-fno-strict-aliasing -ffast-math -fexpensive-optimizations $(CPUOPTIMIZATIONS) -D_SDL
	RELEASE_CFLAGS=$(CPUOPTIMIZATIONS) -D_SDL
endif

# Make sure the Win32 targets don't get these added on
ifneq ($(FTE_TARGET),win32)
	ifneq ($(FTE_TARGET),win32_SDL)
		RELEASE_CFLAGS ?= -O2 -fno-strict-aliasing $(CPUOPTIMIZATIONS)
		#RELEASE_CFLAGS=-O6 -fno-strict-aliasing -ffast-math -funroll-loops -fexpensive-optimizations $(CPUOPTIMIZATIONS)
	endif
endif
ifneq ($(FTE_TARGET),win64)
	ifneq ($(FTE_TARGET),win64_SDL)
		RELEASE_CFLAGS ?= -O2 -fno-strict-aliasing $(CPUOPTIMIZATIONS)
		#RELEASE_CFLAGS=-O6 -fno-strict-aliasing -ffast-math -funroll-loops -fexpensive-optimizations $(CPUOPTIMIZATIONS)
	endif
endif

GLCFLAGS=-DGLQUAKE
D3DCFLAGS=-DD3DQUAKE
NPQTVCFLAGS=-DNPQTV

CLIENT_OBJS = \
	textedit.o	\
	fragstats.o	\
	teamplay.o	\
	zqtp.o	\
	cl_demo.o	\
	cl_ents.o	\
	clq2_ents.o	\
	cl_input.o	\
	cl_main.o	\
	cl_parse.o	\
	cl_pred.o	\
	cl_tent.o	\
	cl_cam.o	\
	cl_screen.o	\
	cl_ui.o	\
	cl_ignore.o \
	cl_cg.o \
	clq3_parse.o	\
	pr_csqc.o	\
	console.o	\
	image.o	\
	keys.o	\
	menu.o	\
	m_download.o	\
	m_master.o	\
	m_multi.o	\
	m_items.o	\
	m_options.o	\
	m_single.o	\
	m_script.o	\
	m_mp3.o	\
	roq_read.o	\
	clq2_cin.o	\
	net_master.o	\
	r_part.o	\
	p_script.o	\
	p_null.o	\
	p_classic.o	\
	r_partset.o	\
	renderer.o	\
	renderque.o	\
	sbar.o	\
	skin.o	\
	snd_al.o	\
	snd_dma.o	\
	snd_mem.o	\
	snd_mix.o	\
	snd_mp3.o	\
	snd_ov.o	\
	valid.o	\
	view.o	\
	wad.o			\
				\
	ftpclient.o		\
	httpclient.o 	\
				\
				\
	pr_menu.o

GLQUAKE_OBJS =		\
	gl_alias.o		\
	gl_draw.o		\
	gl_model.o		\
	gl_ngraph.o		\
	gl_backend.o		\
	gl_rlight.o		\
	gl_rmain.o		\
	gl_rmisc.o		\
	gl_font.o		\
	gl_heightmap.o		\
	gl_rsurf.o		\
	r_surf.o		\
	ltface.o		\
	r_2d.o			\
	gl_screen.o		\
	gl_bloom.o		\
	gl_shadow.o	\
	gl_shader.o		\
	gl_warp.o		\
	gl_vidcommon.o	\
	gl_hlmdl.o

D3DQUAKE_OBJS =		\
	d3d_backend.o \
	d3d_image.o	\
	d3d_shader.o	\
	vid_d3d.o
	
D3DONLY_OBJS =		\
	gl_font.o \
	gl_model.o	\
	gl_shader.o	\
	gl_rlight.o	\
	gl_alias.o	\
	gl_warp.o	\
	ltface.o	\
	r_surf.o	\
	r_2d.o

MP3_OBJS =			\
	fixed.o		\
	bit.o			\
	timer.o		\
	stream.o		\
	frame.o		\
	synth.o		\
	decoder.o		\
	layer12.o		\
	layer3.o		\
	huffman.o		\
	mymad.o

PROGS_OBJS =		\
	comprout.o		\
	hash.o		\
	initlib.o		\
	pr_bgcmd.o		\
	pr_edict.o		\
	pr_exec.o		\
	pr_multi.o		\
	pr_x86.o		\
	qcc_cmdlib.o	\
	qccmain.o		\
	qcc_pr_comp.o	\
	qcc_pr_lex.o	\
	qcdecomp.o		\
	qcd_main.o

SERVER_OBJS = 		\
	pr_cmds.o 		\
	pr_q1qvm.o	\
	sv_master.o 	\
	sv_init.o 		\
	sv_main.o 		\
	sv_nchan.o 		\
	sv_ents.o 		\
	sv_send.o 		\
	sv_phys.o 		\
	sv_user.o		\
	sv_mvd.o		\
	sv_ccmds.o 		\
	sv_rankin.o 	\
	sv_chat.o 		\
	sv_demo.o		\
	net_preparse.o 	\
	savegame.o		\
	svq2_ents.o 	\
	svq2_game.o 	\
	svq3_game.o	\
	webgen.o		\
	ftpserver.o		\
	httpserver.o

SERVERONLY_OBJS =		\
	gl_alias.o		\
	svmodel.o		\
	sv_sys_unix.o

WINDOWSSERVERONLY_OBJS = \
	gl_alias.o		\
	svmodel.o	\
	sv_sys_win.o

COMMON_OBJS = \
	com_mesh.o	\
	com_phys_ode.o	\
	common.o 		\
	cvar.o 		\
	cmd.o 		\
	crc.o 		\
	fs.o			\
	fs_stdio.o		\
	fs_pak.o		\
	fs_zip.o		\
	mathlib.o 		\
	huff.o		\
	md4.o 		\
	log.o 		\
	net_chan.o 		\
	net_wins.o 		\
	zone.o 		\
	qvm.o	\
	r_d3.o	\
	gl_q2bsp.o		\
	glmod_doom.o 	\
	q3common.o	\
	world.o 		\
	sv_move.o 		\
	pmove.o		\
	pmovetst.o		\
	iwebiface.o		\
	translate.o		\
	plugin.o		\
	q1bsp.o		\
	q2pmove.o

#the defaults for sdl come first
GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o
GL_EXE_NAME=../fteqw_sdl.gl$(BITS)
GLCL_EXE_NAME=../fteqwcl_sdl.gl$(BITS)
ifdef windir
	GL_LDFLAGS=$(GLLDFLAGS) -lmingw32 -lws2_32 `sdl-config --libs`
else
	GL_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS) `sdl-config --libs`
endif
GL_CFLAGS=$(GLCFLAGS) `sdl-config --cflags`
GLB_DIR=gl_sdl$(BITS)
GLCL_DIR=glcl_sdl$(BITS)

SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(SERVERONLY_OBJS)
SV_EXE_NAME=../fteqw_sdl.sv$(BITS)
SV_CFLAGS=$(SERVER_ONLY_CFLAGS)
SV_LDFLAGS=-lz

MINGL_DIR=mingl_sdl$(BITS)
MINGL_EXE_NAME=../fteqw_sdl.mingl$(BITS)

MB_DIR=m_sdl$(BITS)
M_EXE_NAME=../fteqw_sdl$(BITS)
MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o 
M_CFLAGS=$(GLCFLAGS) `sdl-config --cflags` -D_MERGED_SDL
M_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS)

ifdef windir
	M_LDFLAGS=$(MLDFLAGS) -lmingw32 -lws2_32 -lSDLmain -lSDL
else
	#pthread is needed because of SDL.
	M_LDFLAGS=$(MLDFLAGS) `sdl-config --libs` $(IMAGELDFLAGS)
endif


#specific targets override those defaults as needed.

ifeq ($(FTE_TARGET),win32_SDL)
	LIBS_DIR=./libs/
	
	#the defaults for sdl come first
	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(LTO_END) resources.o $(LTO_START)
	GL_EXE_NAME=../fteqw_sdl_gl.exe
	GLCL_EXE_NAME=../fteqwcl_sdl.exe
	ifdef windir
		GL_LDFLAGS=$(GLLDFLAGS) -lmingw32 -lws2_32 `sdl-config --libs`
	else
		GL_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 ./libs/mingw-libs/libSDL.a ./libs/mingw-libs/libSDLmain.a -mwindows -ldxguid -lwinmm -lole32 $(GLLDFLAGS) `sdl-config --libs` 
	endif
	GL_CFLAGS=-D_MINGW_VFPRINTF -D_SDL -I./libs/ -I./libs/mingw-libs/ -I./libs/mingw-libs $(GLCFLAGS) `sdl-config --cflags` -I./libs/dxsdk7/include/
	GLB_DIR=gl_mgw_sdl$(BITS)
	GLCL_DIR=glcl_mgw_sdl$(BITS)
	
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(WINDOWSSERVERONLY_OBJS) $(LTO_END) resources.o $(LTO_START)
	SV_EXE_NAME=../fteqw_sdl_sv.exe
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) -D_SDL
	SV_LDFLAGS=./libs/mingw-libs/libz.a -lm -lmingw32 -lws2_32 -lwinmm

	MINGL_DIR=mingl_sdl$(BITS)
	MINGL_EXE_NAME=../fteqw_sdl_mingl.exe

	MB_DIR=m_mgw_sdl$(BITS)
	M_EXE_NAME=../fteqw_sdl.exe
	MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) $(D3DQUAKE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(LTO_END) resources.o $(LTO_START)
	M_CFLAGS=$(D3DCFLAGS) -D_MINGW_VFPRINTF -D_SDL -I./libs/ -I./libs/mingw-libs/ -I./libs/mingw-libs $(GLCFLAGS) `sdl-config --cflags` -D_MERGED_SDL -I./libs/dxsdk7/include/
	M_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS)
	
	ifdef windir
		M_LDFLAGS=$(MLDFLAGS) -lmingw32 -lws2_32 -lSDLmain -lSDL
	else
		#pthread is needed because of SDL.
		M_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 ./libs/mingw-libs/libSDL.a ./libs/mingw-libs/libSDLmain.a -mwindows -ldxguid -lwinmm -lole32 $(MLDFLAGS) `sdl-config --libs` $(IMAGELDFLAGS)
	endif

	D3DCL_OBJS=$(D3DQUAKE_OBJS) snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(D3DONLY_OBJS) $(LTO_END) resources.o $(LTO_START)
	D3D_EXE_NAME=../fted3dqw.exe
	D3DCL_EXE_NAME=../fted3dclqw.exe
	D3D_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 ./libs/mingw-libs/libSDL.a ./libs/mingw-libs/libSDLmain.a -mwindows -ldxguid -lwinmm -lole32
	D3D_CFLAGS=$(D3DCFLAGS) -D_MINGW_VFPRINTF -D_SDL -DNO_XFLIP -I./libs/ -I./libs/mingw-libs/  -I./libs/mingw-libs `sdl-config --cflags` -I./libs/dxsdk7/include/
	D3DB_DIR=d3d_mgw
	D3DCL_DIR=d3dcl_mgw
	
endif
ifeq ($(FTE_TARGET),win64_SDL)
	LIBS_DIR=./libs/
	
	#the defaults for sdl come first
	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(LTO_END) resources.o $(LTO_START)
	GL_EXE_NAME=../fteqw_sdl_gl64.exe
	GLCL_EXE_NAME=../fteqwcl_sdl64.exe
	ifdef windir
		GL_LDFLAGS=$(GLLDFLAGS) -lmingw32 -lws2_32 `sdl-config --libs`
	else
		GL_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 ./libs/mingw64-libs/libSDL.a ./libs/mingw64-libs/libSDLmain.a -mwindows -ldxguid -lwinmm -lole32 $(GLLDFLAGS) `sdl-config --libs` 
	endif
	GL_CFLAGS=-D_SDL -I./libs/ -I./libs/mingw64-libs/ -I./libs/mingw64-libs $(GLCFLAGS) `sdl-config --cflags` -I./libs/dxsdk7/include/
	GLB_DIR=gl_mgw_sdl$(BITS)
	GLCL_DIR=glcl_mgw_sdl$(BITS)
	
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(WINDOWSSERVERONLY_OBJS) $(LTO_END) resources.o $(LTO_START)
	SV_EXE_NAME=../fteqw_sdl_sv64.exe
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) -D_SDL
	SV_LDFLAGS=./libs/mingw64-libs/libz.a -lm -lmingw32 -lws2_32 -lwinmm

	MINGL_DIR=mingl_sdl$(BITS)
	MINGL_EXE_NAME=../fteqw_sdl_mingl64.exe

	MB_DIR=m_mgw_sdl$(BITS)
	M_EXE_NAME=../fteqw_sdl64.exe
	MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) $(D3DQUAKE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(LTO_END) resources.o $(LTO_START)
	M_CFLAGS= $(D3DCFLAGS) -D_MINGW_VFPRINTF -D_SDL -I./libs/ -I./libs/mingw64-libs/ -I./libs/mingw64-libs $(GLCFLAGS) `sdl-config --cflags` -D_MERGED_SDL -I./libs/dxsdk7/include/
	M_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS)
	
	ifdef windir
		M_LDFLAGS=$(MLDFLAGS) -lmingw32 -lws2_32 -lSDLmain -lSDL
	else
		#pthread is needed because of SDL.
		M_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 ./libs/mingw64-libs/libSDL.a ./libs/mingw64-libs/libSDLmain.a -mwindows -ldxguid -lwinmm -lole32 $(MLDFLAGS) `sdl-config --libs` $(IMAGELDFLAGS)
	endif
endif

ifeq ($(FTE_TARGET),vc)
	CC=cl
	DEBUG_CFLAGS ?= -Od $(CPUOPTIMIZATIONS) /fp:fast
	PROFILE_CFLAGS = -O2 -Ot -Ox -GL $(CPUOPTIMISATIONS) /fp:fast 
	PROFILE_LDFLAGS = /LTCG:PGINSTRUMENT
	RELEASE_CFLAGS = -O2 -Ot -Ox -GL $(CPUOPTIMIZATIONS) /fp:fast
	RELEASE_LDFLAGS = /LTCG:PGOPTIMIZE
	

	DO_CC=@$(CC) /nologo $(ALL_CFLAGS) -Fo$@ -c $<
	DO_LD=link /nologo /out:$@ wsock32.lib user32.lib advapi32.lib winmm.lib libs/zlib.lib shell32.lib /nodefaultlib:libc.lib /nodefaultlib:MSVCRT /manifest:no /OPT:REF 
	PRECOMPHEADERS = 
	NODEPS = 1

	LIBS_DIR=./libs/

	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) $(W32_CFLAGS)
	SV_EXE_NAME=../fteqwsv.exe
	SV_DIR=sv_vc
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(WINDOWSSERVERONLY_OBJS) fs_win32.o resources.o
	SV_LDFLAGS=/subsystem:console
	
	M_EXE_NAME=../fteqw.exe
	MCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) fs_win32.o gl_vidnt.o snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o resources.o
	M_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) $(D3DCFLAGS)
	MB_DIR=m_vc
	M_LDFLAGS=$(GLLDFLAGS) libs/jpeg.lib libs/libpng.lib libs/zlib.lib uuid.lib gdi32.lib ole32.lib /subsystem:windows
endif

ifeq ($(FTE_TARGET),win32)
	# The extra object file called resources.o is specific for MinGW to link the icon in

	#cygwin's gcc requires an extra command to use mingw instead of cygwin (default paths, etc).
	ifneq ($(shell $(CC) -v 2>&1 | grep cygwin),)
		W32_CFLAGS=-mno-cygwin
	endif

	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) $(W32_CFLAGS)


	LIBS_DIR = $(BASE_DIR)/libs
	SV_EXE_NAME=../fteqwsv.exe
	SV_LDFLAGS=$(LIBS_DIR)/mingw-libs/zlib.lib -lws2_32 -lwinmm
	V_DIR=sv_mingw
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(WINDOWSSERVERONLY_OBJS) fs_win32.o $(LTO_END) resources.o $(LTO_START)


	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) fs_win32.o gl_vidnt.o snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o $(LTO_END) resources.o $(LTO_START)
	GL_EXE_NAME=../fteglqw.exe
	GLCL_EXE_NAME=../fteglqwcl.exe
	GL_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32
	GL_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) -I./libs/dxsdk7/include/
	GLB_DIR=gl_mgw
	GLCL_DIR=glcl_mgw

	NPQTVCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) fs_win32.o gl_vidnt.o snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o sys_npqtv.o sys_plugfte.o $(LTO_END) npplug.o ../../ftequake/npapi.def $(LTO_START)
	NPQTV_DLL_NAME=../npqtv.dll
	NPQTVCL_DLL_NAME=../npqtvcl.dll
	NPQTV_LDFLAGS=--enable-stdcall-fixup $(GLLDFLAGS) $(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32 -shared
	NPQTV_CFLAGS=$(NPQTVCFLAGS) $(GLCFLAGS) $(W32_CFLAGS) -I./libs/dxsdk7/include/
	NPQTVB_DIR=npqtv_mgw
	NPQTVCL_DIR=npqtvcl_mgw

	MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) $(D3DQUAKE_OBJS) fs_win32.o gl_vidnt.o snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o $(LTO_END) resources.o $(LTO_START)
	M_EXE_NAME=../fteqw.exe
	MCL_EXE_NAME=../fteqwcl.exe
	M_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32
	M_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) $(D3DCFLAGS) -I./libs/dxsdk7/include/
	MB_DIR=m_mgw
	MCL_DIR=mcl_mgw

	D3DCL_OBJS=$(D3DQUAKE_OBJS) snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o $(D3DONLY_OBJS) fs_win32.o $(LTO_END) resources.o $(LTO_START)
	D3D_EXE_NAME=../fted3dqw.exe
	D3DCL_EXE_NAME=../fted3dclqw.exe
	D3D_LDFLAGS=$(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32
	D3D_CFLAGS=$(D3DCFLAGS) $(W32_CFLAGS) -I./libs/dxsdk7/include/
	D3DB_DIR=d3d_mgw
	D3DCL_DIR=d3dcl_mgw

	MINGL_EXE_NAME=../fteminglqw.exe
	MINGL_DIR=mingl_mgw
endif

ifeq ($(FTE_TARGET),win64)
	# The extra object file called resources.o is specific for MinGW to link the icon in

	#cygwin's gcc requires an extra command to use mingw instead of cygwin (default paths, etc).
	ifneq ($(shell $(CC) -v 2>&1 | grep cygwin),)
		W32_CFLAGS=-mno-cygwin
	endif

	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) $(W32_CFLAGS)


	LIBS_DIR = $(BASE_DIR)/libs
	SV_EXE_NAME=../fteqwsv64.exe
	SV_LDFLAGS=$(LIBS_DIR)/mingw64-libs/libz.a -lws2_32 -lwinmm 
	SV_DIR=sv_mingw64
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(WINDOWSSERVERONLY_OBJS) fs_win32.o $(LTO_END) resources.o $(LTO_START)


	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) fs_win32.o gl_vidnt.o snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o $(LTO_END) resources.o $(LTO_START)
	GL_EXE_NAME=../fteglqw64.exe
	GLCL_EXE_NAME=../fteglqwcl64.exe
	GL_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32
	GL_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) -I./libs/dxsdk7/include/
	GLB_DIR=gl_mgw64
	GLCL_DIR=glcl_mgw64

	NPQTVCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) fs_win32.o gl_vidnt.o snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o sys_npqtv.o sys_plugfte.o $(LTO_END) npplug.o ../../ftequake/npapi.def $(LTO_START)
	NPQTV_DLL_NAME=../npqtv64.dll
	NPQTVCL_DLL_NAME=../npqtvcl64.dll
	NPQTV_LDFLAGS=--enable-stdcall-fixup $(GLLDFLAGS) $(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32 -shared
	NPQTV_CFLAGS=$(NPQTVCFLAGS) $(GLCFLAGS) $(W32_CFLAGS) -I./libs/dxsdk7/include/
	NPQTVB_DIR=npqtv_mgw64
	NPQTVCL_DIR=npqtvcl_mgw64

	MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) $(D3DQUAKE_OBJS) fs_win32.o gl_vidnt.o snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o $(LTO_END) resources.o $(LTO_START)
	M_EXE_NAME=../fteqw64.exe
	MCL_EXE_NAME=../fteqwcl64.exe
	M_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32
	M_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) $(D3DCFLAGS) -I./libs/dxsdk7/include/
	MB_DIR=m_mgw64
	MCL_DIR=mcl_mgw64

	D3DCL_OBJS=$(D3DQUAKE_OBJS) snd_win.o snd_directx.o cd_win.o in_win.o sys_win.o $(D3DONLY_OBJS) fs_win32.o $(LTO_END) resources.o $(LTO_START)
	D3D_EXE_NAME=../fted3dqw64.exe
	D3DCL_EXE_NAME=../fted3dclqw64.exe
	D3D_LDFLAGS=$(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32
	D3D_CFLAGS=$(D3DCFLAGS) $(W32_CFLAGS) -I./libs/dxsdk7/include/
	D3DB_DIR=d3d_mgw64
	D3DCL_DIR=d3dcl_mgw64

	MINGL_EXE_NAME=../fteminglqw64.exe
	MINGL_DIR=mingl_mgw64
endif

ifeq ($(FTE_TARGET),bsd)
	#mostly uses the linux stuff.
	#oss, X, etc.

	SV_DIR=sv_linux
	SV_LDFLAGS=-lz -ldl

	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) gl_vidlinuxglx.o snd_linux.o cd_null.o sys_linux.o
	GL_EXE_NAME=../fteqw.gl
	GLCL_EXE_NAME=../fteqwcl.gl
	GL_LDFLAGS= -L/usr/local/lib $(GLLDFLAGS) $(GLXLDFLAGS) -lXxf86vm -lpthread
	GL_CFLAGS=$(GLCFLAGS) -I/usr/local/include -I/usr/X11R6/include 
	GLB_DIR=gl_bsd
	GLCL_DIR=glcl_bsd

	MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidlinuxglx.o snd_linux.o cd_null.o sys_linux.o
	M_EXE_NAME=../fteqw
	MCL_EXE_NAME=../fteqwcl
	M_LDFLAGS=$(GLLDFLAGS) $(GLXLDFLAGS) -lXxf86vm -lpthread
	M_CFLAGS=$(GLCFLAGS) -I/usr/X11R6/include
	MB_DIR=m_bsd
	MCL_DIR=mcl_bsd

	MINGL_EXE_NAME=../fteqw.mingl
	MINGL_DIR=mingl_bsd
endif
ifneq ($(shell echo $(FTE_TARGET)|grep linux),)

	SV_DIR=sv_linux$(BITS)
	SV_LDFLAGS=-lz
	SV_EXE_NAME=../fteqw.sv$(BITS)

	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) gl_vidlinuxglx.o snd_alsa.o snd_linux.o cd_linux.o sys_linux.o
	GL_EXE_NAME=../fteqw.gl$(BITS)
	GLCL_EXE_NAME=../fteqwcl.gl$(BITS)
	GL_LDFLAGS=$(GLLDFLAGS) $(GLXLDFLAGS)  -lXxf86vm -lXxf86dga 
	GL_CFLAGS=$(GLCFLAGS) -I/usr/X11R6/include -DUSE_DGA
	GLB_DIR=gl_linux$(BITS)
	GLCL_DIR=glcl_linux$(BITS)

	MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidlinuxglx.o snd_linux.o snd_alsa.o cd_linux.o sys_linux.o
	M_EXE_NAME=../fteqw$(BITS)
	MCL_EXE_NAME=../fteqwcl$(BITS)
	M_LDFLAGS=$(GLLDFLAGS) $(GLXLDFLAGS) -lXxf86vm -lXxf86dga
	M_CFLAGS=$(GLCFLAGS) -I/usr/X11R6/include
	MB_DIR=m_linux$(BITS)
	MCL_DIR=mcl_linux$(BITS)



	MINGL_EXE_NAME=../fteqw.mingl$(BITS)
	MINGL_DIR=mingl_linux$(BITS)
endif
ifneq ($(shell echo $(FTE_TARGET)|grep macosx),)
	SV_DIR=sv_macosx$(EXTENSION)$(BITS)
	GLB_DIR=gl_macosx$(EXTENSION)$(BITS)
	GLCL_DIR=glcl_macosx$(EXTENSION)$(BITS)
	MINGL_DIR=mingl_macosx$(EXTENSION)$(BITS)

	GL_CFLAGS=$(GLCFLAGS) -D__MACOSX__ -L/sw/lib -I/sw/include -L/opt/local/lib -I/opt/local/include -I/home/moodles/mac/include/ -L/home/moodles/mac/lib -I./libs/
	ifeq ($(FTE_TARGET),macosx_x86)
		GL_CFLAGS=$(GLCFLAGS) -D__MACOSX__ -L/sw/lib -I/sw/include -L/opt/local/lib -I/opt/local/include -I/home/moodles/mac/x86/include/ -L/home/moodles/mac/x86/lib -I./libs/
	endif

	GL_LDFLAGS=-framework AGL -framework OpenGL -framework Cocoa -framework AudioUnit -lz -lpng -ljpeg
	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) gl_vidcocoa.mo gl_vidmacos.o sys_linux.o in_macos.o cd_null.o snd_macos.o

	GL_EXE_NAME=../macosx_fteqw.gl$(EXTENSION)$(BITS)
	GLCL_EXE_NAME=../macosx_fteqwcl.gl$(EXTENSION)$(BITS)
	M_EXE_NAME=../macosx_fteqw$(EXTENSION)$(BITS)
	MCL_EXE_NAME=../macosx_fteqwcl$(EXTENSION)$(BITS)
	MINGL_EXE_NAME=../macosx_fteqw.mingl$(EXTENSION)$(BITS)
	MINGL_DIR=mingl_macosx$(EXTENSION)$(BITS)
	
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(SERVERONLY_OBJS)
	SV_EXE_NAME=../macosx_fteqw.sv$(EXTENSION)$(BITS)
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS)
	SV_LDFLAGS=-lz	

	#seems, macosx has a more limited version of strip
	STRIPFLAGS=
endif
ifeq ($(FTE_TARGET),morphos)
	#-Wno-pointer-sign unrecognised 
	WARNINGFLAGS=-Wall

	SV_DIR=sv_morphos
	SV_LDFLAGS=-ldl -lz

	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) gl_vidmorphos.o in_morphos.o snd_morphos.o cd_null.o sys_morphos.o
	GL_EXE_NAME=../morphos_fteqw.gl
	GLCL_EXE_NAME=../morphos_fteqwcl.gl
	GL_LDFLAGS=$(GLLDFLAGS) -ldl $(IMAGELDFLAGS) -lz
	GL_CFLAGS=$(GLCFLAGS) -noixemul -I./libs/speex -I./
	GLB_DIR=gl_morphos
	GLCL_DIR=glcl_morphos

	MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidmorphos.o vid_morphos.o in_morphos.o snd_morphos.o cd_null.o sys_morphos.o
	M_EXE_NAME=../morphos_fteqw
	MCL_EXE_NAME=../morphos_fteqwcl
	M_LDFLAGS=$(GLLDFLAGS)
	M_CFLAGS=$(GLCFLAGS)
	MB_DIR=m_morphos
	MCL_DIR=mcl_morphos

	MINGL_EXE_NAME=../morphos_fteqw.mingl
	MINGL_DIR=mingl_morphos
	
	#morphos dedicated server doesnt compile: alloca.h: No such file or directory
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(SERVERONLY_OBJS)
	SV_EXE_NAME=../morphos_fteqw.sv$(BITS)
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS)	-I/home/moodles/zlib/zlib-1.2.5 -L/home/moodles/zlib/zlib-1.2.5 -I/home/moodles/wip/wip/engine/libs
endif


ifeq ($(FTE_TARGET),cygwin)

	SV_DIR=sv_cygwin
	SV_LDFLAGS=-lz

	GLCL_OBJS=$(GL_OBJS) $(GLQUAKE_OBJS) gl_vidlinuxglx.o snd_linux.o cd_null.o sys_linux.o
	GL_EXE_NAME=../fteqwglcyg.exe
	GLCL_EXE_NAME=../fteqwclglcyg.exe
	GL_LDFLAGS=$(GLLDFLAGS) $(GLXLDFLAGS)
	GL_CFLAGS=$(GLCFLAGS) -I/usr/X11R6/include
	GLB_DIR=gl_cygwin
	GLCL_DIR=glcl_cygwin

	MCL_OBJS=$(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidlinuxglx.o snd_linux.o cd_null.o sys_linux.o
	M_EXE_NAME=../fteqwcyg.exe
	MCL_EXE_NAME=../fteqwclcyg.exe
	M_LDFLAGS=$(GLLDFLAGS) $(GLXLDFLAGS)
	M_CFLAGS=$(GLCFLAGS)
	MB_DIR=m_cygwin
	MCL_DIR=mcl_cygwin



	MINGL_EXE_NAME=../fteqwminglcyg.exe
	MINGL_DIR=mingl_cygwin
endif

SV_DIR?=sv_sdl

.default: help
all: rel
rel: sv-rel gl-rel mingl-rel
dbg: sv-dbg gl-dbg mingl-dbg
relcl: glcl-rel mcl-rel
profile: sv-profile gl-profile mingl-profile

releases:
	#this is for releasing things from a linux box
	#just go through compiling absolutly everything
	-$(MAKE) FTE_TARGET=linux32 rel
	-$(MAKE) FTE_TARGET=linux64 rel
	-$(MAKE) FTE_TARGET=win32 rel
	-$(MAKE) FTE_TARGET=win64 rel
	-$(MAKE) FTE_TARGET=win32_SDL rel
	-$(MAKE) FTE_TARGET=win64_SDL rel
	-$(MAKE) FTE_TARGET=morphos rel
	-$(MAKE) FTE_TARGET=macosx rel
#	-$(MAKE) FTE_TARGET=linux32 relcl
#	-$(MAKE) FTE_TARGET=linux64 relcl
#	-$(MAKE) FTE_TARGET=win32 relcl

autoconfig: clean
	/bin/bash makeconfig.sh y

config: clean
	/bin/bash makeconfig.sh

ifneq ($(OUT_DIR),)
-include $(OUT_DIR)/*.o.d
endif


VPATH = $(BASE_DIR) : $(CLIENT_DIR) : $(GL_DIR) : $(COMMON_DIR) : $(SERVER_DIR) : $(HTTP_DIR) : $(BASE_DIR)/irc : $(BASE_DIR)/email : $(QUX_DIR) : $(PROGS_DIR) : $(SNDCODEC_DIR) : $(D3D_DIR)

# This is for linking the FTE icon to the MinGW target
$(OUT_DIR)/resources.o : winquake.rc
	$(WINDRES) -I$(CLIENT_DIR) -O coff $< $@

#npAPI stuff requires some extra resources
$(OUT_DIR)/npplug.o : ftequake/npplug.rc
	$(WINDRES) -I$(CLIENT_DIR) -O coff $< $@


#$(OUT_DIR)/%.d: %.c
#	@set -e; rm -f $@; \
#	$(CC) -MM $(ALL_CFLAGS) $< > $@.$$$$; \
#	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
#	rm -f $@.$$$$

$(OUT_DIR)/%.o $(OUT_DIR)/%.d : %.c
ifeq ($(NODEPS),)
	@set -e; rm -f $@.d; \
	 $(CC) -MM $(ALL_CFLAGS) $< > $@.d.$$$$; \
	 sed 's,\($*\)\.o[ :]*,$@ $@.d : ,g' < $@.d.$$$$ > $@.d; \
	 sed -e 's/.*://' -e 's/\\$$//' < $@.d.$$$$ | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.d; \
	 rm -f $@.d.$$$$
endif
	$(DO_CC) -I$(OUT_DIR)

$(OUT_DIR)/%.oo $(OUT_DIR)/%.d : %.c
ifeq ($(NODEPS),)
	@set -e; rm -f $@.d; \
	 $(CC) -MM $(ALL_CFLAGS) $< > $@.d.$$$$; \
	 sed 's,\($*\)\.oo[ :]*,$@ $@.d : ,g' < $@.d.$$$$ > $@.d; \
	 sed -e 's/.*://' -e 's/\\$$//' < $@.d.$$$$ | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.d; \
	 rm -f $@.d.$$$$
endif
	$(DO_CC) -I$(OUT_DIR)

$(OUT_DIR)/%.mo $(OUT_DIR)/%.d : %.m
	@set -e; rm -f $@.d; \
	 $(CC) -MM $(ALL_CFLAGS) $< > $@.d.$$$$; \
	 sed 's,\($*\)\.mo[ :]*,$@ $@.d : ,g' < $@.d.$$$$ > $@.d; \
	 sed -e 's/.*://' -e 's/\\$$//' < $@.d.$$$$ | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.d; \
	 rm -f $@.d.$$$$
	$(DO_CC) -I$(OUT_DIR)

#enables use of precompiled headers in gcc 3.4 onwards.
$(OUT_DIR)/quakedef.h.gch : quakedef.h
	$(CC) -x c $(BASE_CFLAGS) $(WCFLAGS) -o $@ -c $< $(CFLAGS)
PRECOMPHEADERS ?= $(OUT_DIR)/quakedef.h.gch

#addprefix is to add the ./release/server/ part of the object name
#foreach is needed as the OBJS is a list of variable names containing object lists.
#which is needed as windows sucks too much for the chaining to carry a full list.
#god knows how gcc loads the list properly.
#or at least I hope he does. It makes no sence to mortals.

DO_LD ?= $(CC) -o $@ $(LTO_LD) $(WCFLAGS) $(CFLAGS)
$(OUT_DIR)/$(EXE_NAME):   $(PRECOMPHEADERS) $(foreach fn, $(CUSTOMOBJS) $(foreach ol, $(OBJS), $($(ol))),$(if $(findstring ltox,$(fn)),,$(OUT_DIR)/$(fn)))
	$(DO_LD) $(foreach fn, $(CUSTOMOBJS) $(foreach ol, $(OBJS) $(LTO_END), $($(ol))),$(if $(findstring ltox,$(fn)),$(subst ltox,-x ,$(fn)),$(OUT_DIR)/$(fn)) ) -x none $(LDFLAGS)

_out-rel:
	$(MAKE) $(OUT_DIR)/$(EXE_NAME) EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(WCFLAGS) $(RELEASE_CFLAGS)" LDFLAGS="$(BASELDFLAGS) $(LDFLAGS) $(RELEASE_LDFLAGS)" OBJS="$(OBJS)"
	$(STRIP) $(STRIPFLAGS) $(OUT_DIR)/$(EXE_NAME)

_out-dbg:
	$(MAKE) $(OUT_DIR)/$(EXE_NAME) EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(WCFLAGS) $(DEBUG_CFLAGS)" LDFLAGS="$(BASELDFLAGS) $(LDFLAGS) $(DEBUG_LDFLAGS)" OBJS="$(OBJS)"

_out-profile:
	$(MAKE) $(OUT_DIR)/$(EXE_NAME) EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(WCFLAGS) $(PROFILE_CFLAGS)" LDFLAGS="$(BASELDFLAGS) $(LDFLAGS) $(PROFILE_LDFLAGS)" OBJS="$(OBJS)"

_cl-rel: reldir
	$(MAKE) _out-rel EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(CLIENT_ONLY_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS"

_cl-dbg: debugdir
	$(MAKE) _out-dbg EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(CLIENT_ONLY_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS"

_cl-profile: reldir
	$(MAKE) _out-profile EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(CLIENT_ONLY_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS"

_clsv-rel: reldir
	$(MAKE) _out-rel EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(JOINT_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS SERVER_OBJS"

_clsv-dbg: debugdir
	$(MAKE) _out-dbg EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(JOINT_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS SERVER_OBJS"

_clsv-profile: reldir
	$(MAKE) _out-profile EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(JOINT_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS SERVER_OBJS"

sv-tmp: reldir debugdir
	$(MAKE) $(TYPE) OUT_DIR="$(OUT_DIR)" EXE_NAME="$(SV_EXE_NAME)" WCFLAGS="$(SV_CFLAGS)" LDFLAGS="$(SV_LDFLAGS) $(LDFLAGS)" OBJS="SV_OBJS"
sv-rel:
	$(MAKE) sv-tmp TYPE=_out-rel OUT_DIR="$(RELEASE_DIR)/$(SV_DIR)"
sv-dbg:
	$(MAKE) sv-tmp TYPE=_out-dbg OUT_DIR="$(DEBUG_DIR)/$(SV_DIR)"
sv-profile:
	$(MAKE) sv-tmp TYPE=_out-profile OUT_DIR="$(PROFILE_DIR)/$(SV_DIR)"

d3dcl-tmp:
	$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(D3DCL_EXE_NAME)" WCFLAGS="$(D3D_CFLAGS)" LDFLAGS="$(D3D_LDFLAGS) $(LDFLAGS)" SOBJS="$(D3DCL_OBJS)"
d3d-tmp:
	$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(D3D_EXE_NAME)" WCFLAGS="$(D3D_CFLAGS)" LDFLAGS="$(D3D_LDFLAGS) $(LDFLAGS)" SOBJS="$(D3DCL_OBJS)"

d3dcl-rel:
	$(MAKE) d3dcl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(D3DCL_DIR)"
d3dcl-dbg:
	$(MAKE) d3dcl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(D3DCL_DIR)"
d3dcl-profile:
	$(MAKE) d3dcl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(D3DCL_DIR)"

d3d-rel:
	$(MAKE) d3d-tmp TYPE=_clsv-rel OUT_DIR="$(RELEASE_DIR)/$(D3DB_DIR)"
d3d-dbg:
	$(MAKE) d3d-tmp TYPE=_clsv-dbg OUT_DIR="$(DEBUG_DIR)/$(D3DB_DIR)"
d3d-profile:
	$(MAKE) d3d-tmp TYPE=_clsv-profile OUT_DIR="$(PROFILE_DIR)/$(D3DB_DIR)"


npqtvcl-tmp:
	$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(NPQTVCL_DLL_NAME)" WCFLAGS="$(NPQTV_CFLAGS)" LDFLAGS="$(NPQTV_LDFLAGS) $(LDFLAGS)" SOBJS="$(NPQTVCL_OBJS)"
npqtv-tmp:
	$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(NPQTV_DLL_NAME)" WCFLAGS="$(NPQTV_CFLAGS)" LDFLAGS="$(NPQTV_LDFLAGS) $(LDFLAGS)" SOBJS="$(NPQTVCL_OBJS)"

npqtvcl-rel:
	$(MAKE) npqtvcl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(NPQTVCL_DIR)"
npqtvcl-dbg:
	$(MAKE) npqtvcl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(NPQTVCL_DIR)"
npqtvcl-profile:
	$(MAKE) npqtvcl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(NPQTVCL_DIR)"
npqtv-rel:
	$(MAKE) npqtv-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(NPQTVB_DIR)"
npqtv-dbg:
	$(MAKE) npqtv-tmp TYPE=_clsv-dbg OUT_DIR="$(DEBUG_DIR)/$(NPQTVB_DIR)"
npqtv-profile:
	$(MAKE) npqtv-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(NPQTVB_DIR)"

glcl-tmp:
	$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(GLCL_EXE_NAME)" WCFLAGS="$(GL_CFLAGS)" LDFLAGS="$(GL_LDFLAGS) $(LDFLAGS)" SOBJS="$(GLCL_OBJS)"
gl-tmp:
	$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(GL_EXE_NAME)" WCFLAGS="$(GL_CFLAGS)" LDFLAGS="$(GL_LDFLAGS) $(LDFLAGS)" SOBJS="$(GLCL_OBJS)"

glcl-rel:
	$(MAKE) glcl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(GLCL_DIR)"
glcl-dbg:
	$(MAKE) glcl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(GLCL_DIR)"
glcl-profile:
	$(MAKE) glcl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(GLCL_DIR)"
gl-rel:
	$(MAKE) gl-tmp TYPE=_clsv-rel OUT_DIR="$(RELEASE_DIR)/$(GLB_DIR)"
gl-dbg:
	$(MAKE) gl-tmp TYPE=_clsv-dbg OUT_DIR="$(DEBUG_DIR)/$(GLB_DIR)"
gl-profile:
	$(MAKE) gl-tmp TYPE=_clsv-profile OUT_DIR="$(PROFILE_DIR)/$(GLB_DIR)"

mingl-tmp: reldir
	$(MAKE) $(TYPE) OUT_DIR="$(OUT_DIR)" EXE_NAME="$(MINGL_EXE_NAME)" WCFLAGS="$(GL_CFLAGS) -DMINIMAL" LDFLAGS="$(GL_LDFLAGS) $(LDFLAGS)" SOBJS="$(GLCL_OBJS)"
mingl-rel:
	$(MAKE) mingl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(MINGL_DIR)"
mingl-dbg:
	$(MAKE) mingl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(MINGL_DIR)"
mingl-profile:
	$(MAKE) mingl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(MINGL_DIR)"

mcl-tmp:
	$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(MCL_EXE_NAME)" WCFLAGS="$(M_CFLAGS)" LDFLAGS="$(M_LDFLAGS) $(LDFLAGS)" SOBJS="$(MCL_OBJS)"
m-tmp:
	$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(M_EXE_NAME)" WCFLAGS="$(M_CFLAGS)" LDFLAGS="$(M_LDFLAGS) $(LDFLAGS)" SOBJS="$(MCL_OBJS)"

mcl-rel:
	$(MAKE) mcl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(MCL_DIR)"
mcl-dbg:
	$(MAKE) mcl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(MCL_DIR)"
mcl-profile:
	$(MAKE) mcl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(MCL_DIR)"
m-rel:
	$(MAKE) m-tmp TYPE=_clsv-rel OUT_DIR="$(RELEASE_DIR)/$(MB_DIR)"
m-dbg:
	$(MAKE) m-tmp TYPE=_clsv-dbg OUT_DIR="$(DEBUG_DIR)/$(MB_DIR)"
m-profile:
	$(MAKE) m-tmp TYPE=_clsv-profile OUT_DIR="$(PROFILE_DIR)/$(MB_DIR)"

.PHONY: m-tmp mcl-tmp mingl-tmp glcl-tmp gl-tmp sv-tmp _clsv-dbg _clsv-rel _cl-dbg _cl-rel _out-rel _out-dbg

ifdef windir
debugdir:
	@-mkdir -p $(subst /,\, $(OUT_DIR))
reldir:
	@-mkdir -p $(subst /,\, $(OUT_DIR))
else
reldir:
	@-mkdir -p $(RELEASE_DIR)
	@-mkdir -p $(OUT_DIR)
debugdir:
	@-mkdir -p $(DEBUG_DIR)
	@-mkdir -p $(OUT_DIR)
endif

plugins:
	ifneq ($(shell grep all ../plugins/Makefile),)
	$(MAKE) -C ../plugins
	else
	@-echo no plugins directory installed
	endif
plugins:

help:
	@-echo "Specfic targets:"
	@-echo "clean - removes all output (use make dirs afterwards)"
	@-echo "all - make all the targets possible"
	@-echo "rel - make the releases for the default system"
	@-echo "dbg - make the debug builds for the default system"
	@-echo "profile - make all the releases with profiling support for the default system"
	@-echo ""
	@-echo "Normal targets:"
	@-echo "(each of these targets must have the postfix -rel or -dbg)"
	@-echo "'sv-???' (Dedicated Server)"
	@-echo "'gl-???' (OpenGL rendering + Built-in Server)"
	@-echo "'m-???' (Merged client, OpenGL & D3D rendering + Dedicated server)"
	@-echo "'mingl-???' (Minimal featured OpenGL render)"
	@-echo "'npqtv-???' (FTE_TARGET=win32 only, for now) (QuakeTV Firefox/Netscape browser plugin)"
	@-echo "'d3d-???' (for windows builds)"
	@-echo "'mcl-???' (currently broken)"
	@-echo "'glcl-???' (currently broken)"
	@-echo ""
	@-echo "Cross targets can be specified with FTE_TARGET=blah"
	@-echo "linux32, linux64 specify specific x86 archs"
	@-echo "SDL - Attempt to use sdl for the current target"
	@-echo "win32 - Mingw compile for win32"
	@-echo "vc - Attempts to use msvc8+ to compile. Note: uses profile guided optimisations. You must build+run the relevent profile target before a release target will compile properly. Debug doesn't care."

install:
	-cp debug/*.* /opt/quake/
	-cp release/*.* /opt/quake/
	-cp profile/*.* /opt/quake

clean:
	-rm -f -r $(RELEASE_DIR)
	-rm -f -r $(DEBUG_DIR)
	-rm -f -r $(PROFILE_DIR)

