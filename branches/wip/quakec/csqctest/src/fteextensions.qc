/*
This file was automatically generated by FTE QuakeWorld v1.01
This file can be regenerated by issuing the following command:
pr_dumpplatform 
Available options:
-Ffte     - target only FTE (optimations and additional extensions)
-Tnq      - dump specifically NQ fields
-Tqw      - dump specifically QW fields
-Tcs      - dump specifically CSQC fields
-Fdefines - generate #defines instead of constants
-O        - write to a different qc file
*/
#pragma noref 1
#if !defined(CSQC) && !defined(SSQC)
#define SSQC
#endif
entity self;
entity other;
entity world;
float time;
#ifdef CSQC
float cltime;
#endif
float frametime;
#ifdef CSQC
float player_localentnum;
float player_localnum;
float maxclients;
float clientcommandframe;
float servercommandframe;
#endif
#if defined(SSQC) && !defined(NETQUAKE)
entity newmis;
#endif
#ifdef SSQC
float force_retouch;
#endif
string mapname;
#if defined(SSQC) && defined(NETQUAKE)
float deathmatch;
float coop;
float teamplay;
#endif
#ifdef SSQC
float serverflags;
float total_secrets;
float total_monsters;
float found_secrets;
float killed_monsters;
float parm1, parm2, parm3, parm4, parm5, parm6, parm7, parm8, parm9, parm10, parm11, parm12, parm13, parm14, parm15, parm16;
#endif
#ifdef CSQC
float intermission;
#endif
vector v_forward, v_up, v_right;
#ifdef CSQC
vector view_angles;
#endif
float trace_allsolid, trace_startsolid, trace_fraction;
vector trace_endpos, trace_plane_normal;
float trace_plane_dist;
entity trace_ent;
float trace_inopen;
float trace_inwater;
#ifdef CSQC
float input_timelength;
vector input_angles;
vector input_movevalues;
float input_buttons;
float input_impulse;
#endif
#ifdef SSQC
entity msg_entity;
void() main;
void() StartFrame;
void() PlayerPreThink;
void() PlayerPostThink;
void() ClientKill;
void(float usingcsqc) ClientConnect;
void() PutClientInServer;
void() ClientDisconnect;
void() SetNewParms;
void() SetChangeParms;
#endif
void end_sys_globals;
.float modelindex;
.vector absmin, absmax;
#ifdef SSQC
.float ltime;
#endif
#ifdef CSQC
.float entnum;
.float drawmask;
.void() predraw;
#endif
#if defined(SSQC) && !defined(NETQUAKE)
.float lastruntime;
#endif
.float movetype;
.float solid;
.vector origin;
.vector oldorigin;
.vector velocity;
.vector angles;
.vector avelocity;
#ifdef CSQC
.float pmove_flags;
#endif
#if defined(SSQC) && defined(NETQUAKE)
.vector punchangle;
#endif
.string classname;
#ifdef CSQC
.float renderflags;
#endif
.string model;
.float frame;
#ifdef CSQC
.float frame1time;
.float frame2;
.float frame2time;
.float lerpfrac;
#endif
.float skin;
.float effects;
.vector mins, maxs;
.vector size;
.void() touch;
#ifdef SSQC
.void() use;
#endif
.void() think;
.void() blocked;
.float nextthink;
#ifdef SSQC
.entity groundentity;
.float health;
.float frags;
.float weapon;
.string weaponmodel;
.float weaponframe;
.float currentammo;
.float ammo_shells;
.float ammo_nails;
.float ammo_rockets;
.float ammo_cells;
.float items;
.float takedamage;
#endif
.entity chain;
#ifdef SSQC
.float deadflag;
.vector view_ofs;
.float button0;
.float button1;
.float button2;
.float impulse;
.float fixangle;
.vector v_angle;
#endif
#if defined(SSQC) && defined(NETQUAKE)
.float idealpitch;
#endif
#ifdef SSQC
.string netname;
#endif
.entity enemy;
.float flags;
.float colormap;
#ifdef SSQC
.float team;
.float max_health;
.float teleport_time;
.float armortype;
.float armorvalue;
.float waterlevel;
.float watertype;
.float ideal_yaw;
.float yaw_speed;
.entity aiment;
.entity goalentity;
.float spawnflags;
.string target;
.string targetname;
.float dmg_take;
.float dmg_save;
.entity dmg_inflictor;
#endif
.entity owner;
#ifdef SSQC
.vector movedir;
.string message;
.float sounds;
.string noise;
.string noise1;
.string noise2;
.string noise3;
#endif
void end_sys_fields;
#ifdef SSQC
.vector absmin;
.vector absmax;
#endif
#if defined(SSQC) && defined(NETQUAKE)
.float lastruntime;
#endif
#ifdef SSQC
.vector mins;
.vector maxs;
#endif
#if defined(CSQC) || (defined(SSQC) && !defined(NETQUAKE))
.vector punchangle;
#endif
.float gravity;
.float hull;
.entity movechain;
.void() chainmoved;
.float dimension_solid;
.float dimension_hit;
.float scale;
.float fatness;
.float alpha;
.entity tag_entity;
.float skeletonindex;
.vector colormod;
.vector glowmod;
.vector gravitydir;
#ifdef SSQC
.float pmove_flags;
#endif
.float friction;
.float erp;
.float jointtype;
.float mass;
.float bouncefactor;
.float bouncestop;
#ifdef SSQC
.float maxspeed;
.float items2;
.entity view2;
.vector movement;
.float vw_index;
.entity nodrawtoclient;
.entity drawonlytoclient;
.entity viewmodelforclient;
.entity exteriormodeltoclient;
.float button3;
.float button4;
.float button5;
.float button6;
.float button7;
.float button8;
.float viewzoom;
.float tag_index;
.float glow_size;
.float glow_color;
.float glow_trail;
.vector color;
.float light_lev;
.float style;
.float pflags;
.float clientcolors;
.float dimension_see;
.float dimension_seen;
.float dimension_ghost;
.float dimension_ghost_alpha;
.float playerclass;
.float drawflags;
.float hasted;
.float light_level;
.float abslight;
.float(entity playerent, float changedflags) SendEntity;
.float SendFlags;
.float Version;
.float pvsflags;
.float modelflags;
.float uniquespawnid;
.float() customizeentityforclient;
#endif
#ifdef CSQC
.float forceshader;
.float baseframe;
.float baseframe2;
.float baseframe1time;
.float baseframe2time;
.float baselerpfrac;
.float basebone;
.float bonecontrol1;
.float bonecontrol2;
.float bonecontrol3;
.float bonecontrol4;
.float bonecontrol5;
.float subblendfrac;
.float basesubblendfrac;
.float ideal_pitch;
.float pitch_speed;
#endif
var float physics_mode = 2;
#ifdef CSQC
float gamespeed;
#endif
const float TRUE = 1;
const float FALSE = 0;
const float MOVETYPE_NONE = 0;
const float MOVETYPE_WALK = 3;
const float MOVETYPE_STEP = 4;
const float MOVETYPE_FLY = 5;
const float MOVETYPE_TOSS = 6;
const float MOVETYPE_PUSH = 7;
const float MOVETYPE_NOCLIP = 8;
const float MOVETYPE_FLYMISSILE = 9;
const float MOVETYPE_BOUNCE = 10;
const float MOVETYPE_BOUNCEMISSILE = 11;
const float MOVETYPE_FOLLOW = 12;
const float MOVETYPE_PHYSICS = 32;
const float SOLID_NOT = 0;
const float SOLID_TRIGGER = 1;
const float SOLID_BBOX = 2;
const float SOLID_SLIDEBOX = 3;
const float SOLID_BSP = 4;
const float SOLID_CORPSE = 5;
const float SOLID_LADDER = 20;
const float SOLID_PHYSICS_BOX = 32;
const float SOLID_PHYSICS_SPHERE = 33;
const float SOLID_PHYSICS_CAPSULE = 34;
const float JOINTTYPE_FIXED = -1;
const float JOINTTYPE_POINT = 1;
const float JOINTTYPE_HINGE = 2;
const float JOINTTYPE_SLIDER = 3;
const float JOINTTYPE_UNIVERSAL = 4;
const float JOINTTYPE_HINGE2 = 5;
#ifdef SSQC
const float DAMAGE_NO = 0;
const float DAMAGE_YES = 1;
const float DAMAGE_AIM = 2;
#endif
const float CONTENT_EMPTY = -1;
const float CONTENT_SOLID = -2;
const float CONTENT_WATER = -3;
const float CONTENT_SLIME = -4;
const float CONTENT_LAVA = -5;
const float CONTENT_SKY = -6;
const float CHAN_AUTO = 0;
const float CHAN_WEAPON = 1;
const float CHAN_VOICE = 2;
const float CHAN_ITEM = 3;
const float CHAN_BODY = 4;
const float ATTN_NONE = 0;
const float ATTN_NORM = 1;
#ifdef SSQC
const float MSG_BROADCAST = 0;
const float MSG_ONE = 1;
const float MSG_ALL = 2;
const float MSG_INIT = 3;
const float MSG_MULTICAST = 4;
const float MSG_ENTITY = 5;
const float MULTICAST_ALL = 0;
const float MULTICAST_PHS = 1;
const float MULTICAST_PVS = 2;
const float MULTICAST_ONE = 6;
const float MULTICAST_ALL_R = 3;
const float MULTICAST_PHS_R = 4;
const float MULTICAST_PVS_R = 5;
const float MULTICAST_ONE_R = 7;
#endif
#if defined(SSQC) && !defined(NETQUAKE)
const float PRINT_LOW = 0;
const float PRINT_MEDIUM = 1;
const float PRINT_HIGH = 2;
const float PRINT_CHAT = 3;
#endif
const float FL_FLY = 1;
const float FL_SWIM = 2;
const float FL_CLIENT = 8;
const float FL_INWATER = 16;
const float FL_MONSTER = 32;
#ifdef SSQC
const float FL_GODMODE = 64;
const float FL_NOTARGET = 128;
#endif
const float FL_ITEM = 256;
const float FL_ONGROUND = 512;
const float FL_PARTIALGROUND = 1024;
const float FL_WATERJUMP = 2048;
#if defined(CSQC) || (defined(SSQC) && defined(NETQUAKE))
const float FL_JUMPRELEASED = 4096;
#endif
const float FL_FINDABLE_NONSOLID = 16384;
#ifdef SSQC
const float FL_LAGGEDMOVE = 65536;
#endif
const float MOVE_NORMAL = 0;
const float MOVE_NOMONSTERS = 1;
const float MOVE_MISSILE = 2;
const float MOVE_HITMODEL = 4;
const float MOVE_TRIGGERS = 16;
const float MOVE_EVERYTHING = 32;
#ifdef SSQC
const float MOVE_LAGGED = 64;
#endif
const float MOVE_ENTCHAIN = 128;
const float EF_BRIGHTFIELD = 1;
#if defined(CSQC) || (defined(SSQC) && defined(NETQUAKE))
const float EF_MUZZLEFLASH = 2;
#endif
const float EF_BRIGHTLIGHT = 4;
const float EF_DIMLIGHT = 8;
#if defined(SSQC) && !defined(NETQUAKE)
const float EF_FLAG1 = 16;
const float EF_FLAG2 = 32;
#endif
#if defined(CSQC) || (defined(SSQC) && defined(NETQUAKE))
const float EF_ADDITIVE = 32;
#endif
const float EF_BLUE = 64;
const float EF_RED = 128;
const float EF_FULLBRIGHT = 512;
const float EF_NODEPTHTEST = 8192;
#ifdef CSQC
const float STAT_HEALTH = 0;
const float STAT_WEAPON = 2;
const float STAT_AMMO = 3;
const float STAT_ARMOR = 4;
const float STAT_WEAPONFRAME = 5;
const float STAT_SHELLS = 6;
const float STAT_NAILS = 7;
const float STAT_ROCKETS = 8;
const float STAT_CELLS = 9;
const float STAT_ACTIVEWEAPON = 10;
const float STAT_TOTALSECRETS = 11;
const float STAT_TOTALMONSTERS = 12;
const float STAT_SECRETS = 13;
const float STAT_MONSTERS = 14;
const float STAT_ITEMS = 15;
const float STAT_VIEWHEIGHT = 16;
const float STAT_VIEW2 = 20;
const float STAT_VIEWZOOM = 21;
const float VF_MIN = 1;
const float VF_MIN_X = 2;
const float VF_MIN_Y = 3;
const float VF_SIZE = 4;
const float VF_SIZE_X = 5;
const float VF_SIZE_Y = 6;
const float VF_VIEWPORT = 7;
const float VF_FOV = 8;
const float VF_FOVX = 9;
const float VF_ORIGIN = 11;
const float VF_ORIGIN_X = 12;
const float VF_ORIGIN_Y = 13;
const float VF_ORIGIN_Z = 14;
const float VF_ANGLES = 15;
const float VF_ANGLES_X = 16;
const float VF_ANGLES_Y = 17;
const float VF_ANGLES_Z = 18;
const float VF_DRAWWORLD = 19;
const float VF_DRAWENGINESBAR = 20;
const float VF_DRAWCROSSHAIR = 21;
const float VF_CL_VIEWANGLES = 33;
const float VF_CL_VIEWANGLES_X = 34;
const float VF_CL_VIEWANGLES_Y = 35;
const float VF_CL_VIEWANGLES_Z = 36;
const float VF_PERSPECTIVE = 200;
const float VF_LPLAYER = 202;
const float VF_AFOV = 203;
const float RF_VIEWMODEL = 1;
const float RF_EXTERNALMODEL = 2;
const float RF_DEPTHHACK = 4;
const float RF_ADDITIVE = 8;
const float RF_USEAXIS = 16;
const float RF_NOSHADOW = 32;
const float RF_FRAMETIMESARESTARTTIMES = 64;
const float RF_NOAUTOADD = 128;
const float IE_KEYDOWN = 0;
const float IE_KEYUP = 1;
const float IE_MOUSEDELTA = 2;
const float IE_MOUSEABS = 3;
const float IE_ACCELEROMETER = 4;
#endif
const float FILE_READ = 0;
const float FILE_APPEND = 1;
const float FILE_WRITE = 2;
const float FILE_READNL = 4;
const float FILE_MMAP_READ = 5;
const float FILE_MMAP_RW = 6;
#ifdef CSQC
const float MOVE_LAGGED = 64;
const float MASK_ENGINE = 1;
const float MASK_VIEWMODEL = 2;
const float LFIELD_ORIGIN = 0;
const float LFIELD_COLOUR = 1;
const float LFIELD_RADIUS = 2;
const float LFIELD_FLAGS = 3;
const float LFIELD_STYLE = 4;
const float LFIELD_ANGLES = 5;
const float LFIELD_FOV = 6;
const float LFIELD_CORONA = 7;
const float LFIELD_CORONASCALE = 8;
const float LFIELD_CUBEMAPNAME = 9;
const float LFIELD_AMBIENTSCALE = 10;
const float LFIELD_DIFFUSESCALE = 11;
const float LFIELD_SPECULARSCALE = 12;
const float LFLAG_NORMALMODE = 1;
const float LFLAG_REALTIMEMODE = 2;
const float LFLAG_LIGHTMAP = 4;
const float LFLAG_FLASHBLEND = 8;
const float LFLAG_NOSHADOWS = 256;
const float LFLAG_SHADOWMAP = 512;
const float LFLAG_CREPUSCULAR = 1024;
#endif
void(vector) makevectors = #1;
void(entity e, vector o) setorigin = #2;
void(entity e, string m) setmodel = #3;
void(entity e, vector min, vector max) setsize = #4;
#ifdef SSQC
void(float style, float val) lightstylestatic = #5;
void() break = #6;
#endif
float() random = #7;
void(entity e, float chan, string samp, float vol, float atten, optional float speedpct, optional float flags) sound = #8;
vector(vector v) normalize = #9;
void(string e) error = #10;
void(string e) objerror = #11;
float(vector v) vlen = #12;
float(vector v) vectoyaw = #13;
entity() spawn = #14;
void(entity e) remove = #15;
void(vector v1, vector v2, float nomonsters, entity ent) traceline = #16;
#ifdef SSQC
entity() checkclient = #17;
#endif
entity(entity start, .string fld, string match) find = #18;
void(string s) precache_sound = #19;
void(string s) precache_model = #20;
#ifdef SSQC
void(entity client, string s) stuffcmd = #21;
#endif
entity(vector org, float rad) findradius = #22;
#ifdef SSQC
void(string s) bprint = #23;
void(entity client, string s) sprint = #24;
#endif
void(string s, ...) dprint = #25;
string(float val) ftos = #26;
string(vector val) vtos = #27;
void() coredump = #28;
void() traceon = #29;
void() traceoff = #30;
void(entity e) eprint = #31;
float(float yaw, float dist) walkmove = #32;
float() droptofloor = #34;
void(float lightstyle, string stylestring) lightstyle = #35;
float(float) rint = #36;
float(float) floor = #37;
float(float) ceil = #38;
//float(vector v) qtest_canreach = #39;
float(entity ent) checkbottom = #40;
float(vector pos) pointcontents = #41;
float(float) fabs = #43;
#ifdef SSQC
vector(entity player, float missilespeed) aim = #44;
#endif
float(string) cvar = #45;
void(string, ...) localcmd = #46;
entity(entity) nextent = #47;
void(vector pos, vector dir, float colour, float count) particle = #48;
#define ChangeYaw changeyaw
void() changeyaw = #49;
vector(vector fwd, optional vector up) vectoangles = #51;
#ifdef SSQC
void(float to, float val) WriteByte = #52;
void(float to, float val) WriteChar = #53;
void(float to, float val) WriteShort = #54;
void(float to, float val) WriteLong = #55;
void(float to, float val) WriteCoord = #56;
void(float to, float val) WriteAngle = #57;
void(float to, string val) WriteString = #58;
void(float to, entity val) WriteEntity = #59;
#endif
float(float angle) sin = #60;
float(float angle) cos = #61;
float(float value) sqrt = #62;
void(entity ent) changepitch = #63;
void(entity ent, entity ignore) tracetoss = #64;
string(entity ent) etos = #65;
void(float step) movetogoal = #67;
#ifdef SSQC
void(string s) precache_file = #68;
#endif
void(entity e) makestatic = #69;
#ifdef SSQC
void(string mapname, optional string newmapstartspot) changelevel = #70;
#endif
void(string cvarname, string valuetoset) cvar_set = #72;
#ifdef SSQC
void(entity ent, string text, ...) centerprint = #73;
#endif
void (vector pos, string samp, float vol, float atten) ambientsound = #74;
void(string str) precache_model2 = #75;
void(string str) precache_sound2 = #76;
#ifdef SSQC
void(string str) precache_file2 = #77;
void() setspawnparms = #78;
void(entity killer, entity killee) logfrag = #79;
string(entity e, string key) infokey = #80;
#endif
float(string) stof = #81;
#ifdef SSQC
void(vector where, float set) multicast = #82;
#endif
void(vector start, vector mins, vector maxs, vector end, float nomonsters, entity ent) tracebox = #90;
vector() randomvec = #91;
vector(vector org) getlight = #92;
void(string cvarname, string defaultvalue) registercvar = #93;
float(float a, float b, ...) min = #94;
float(float a, float b, ...) max = #95;
float(float minimum, float val, float maximum) bound = #96;
float(float value, float exp) pow = #97;
entity(entity start, .float fld, float match) findfloat = #98;
float(string extname) checkextension = #99;
#ifdef SSQC
float(string builtinname) builtin_find = #100;
float(float value) anglemod = #102;
void(string slot, string picname, float x, float y, float zone, optional entity player) showpic = #104;
void(string slot, optional entity player) hidepic = #105;
void(string slot, float x, float y, float zone, optional entity player) movepic = #106;
void(string slot, string picname, optional entity player) changepic = #107;
#endif
float(string filename, float mode, optional float mmapminsize) fopen = #110;
void(float fhandle) fclose = #111;
string(float fhandle) fgets = #112;
void(float fhandle, string s) fputs = #113;
float(string s) strlen = #114;
string(string s1, optional string s2, ...) strcat = #115;
string(string s, float start, float length) substring = #116;
vector(string s) stov = #117;
string(string s) strzone = #118;
void(string s) strunzone = #119;
#ifdef SSQC
void(string cvar, float val) cvar_setf = #176;
#endif
float(string modelname, optional float queryonly) getmodelindex = #200;
__variant(float prnum, string funcname, ...) externcall = #201;
float(string progsname) addprogs = #202;
__variant(float prnum, string varname) externvalue = #203;
void(float prnum, __variant newval, string varname) externset = #204;
float(string input, string token) instr = #206;
void(float portal, float state) openportal = #207;
#ifdef SSQC
float(float attributes, string effectname, ...) RegisterTempEnt = #208;
void(float type, vector pos, ...) CustomTempEnt = #209;
float(optional float sleeptime) fork = #210;
#endif
void(optional __variant ret) abort = #211;
#ifdef SSQC
void(float sleeptime) sleep = #212;
void(entity player, string key, string value) forceinfokey = #213;
void(string filename, float starttag, entity edict) chat = #214;
#endif
void(vector org, vector dmin, vector dmax, float colour, float effect, float count) particle2 = #215;
void(vector org, vector box, float colour, float effect, float count) particle3 = #216;
void(vector org, float radius, float colour, float effect, float count) particle4 = #217;
float(float number, float quantity) bitshift = #218;
void(vector pos) te_lightningblood = #219;
float(string s1, string sub, optional float startidx) strstrofs = #221;
float(string str, float index) str2chr = #222;
string(float chr, ...) chr2str = #223;
string(float ccase, float redalpha, float redchars, string str, ...) strconv = #224;
string(float pad, string str1, ...) strpad = #225;
string(string old, string key, string value) infoadd = #226;
string(string info, string key) infoget = #227;
float(string s1, string s2, float len) strncmp = #228;
float(string s1, string s2) strcasecmp = #229;
float(string s1, string s2, float len) strncasecmp = #230;
void() calltimeofday = #231;
#ifdef SSQC
void(float num, float type, .void fld) clientstat = #232;
void(float num, float type, string name) globalstat = #233;
void(entity player) isbackbuffered = #234;
#endif
void(vector angle) rotatevectorsbyangle = #235;
void(vector fwd, vector right, vector up) rotatevectorsbyvectors = #236;
float(float mdlindex, string skinname) skinforname = #237;
#ifdef CSQC
float(string shadername, optional string defaultshader, ...) shaderforname = #238;
#endif
void(vector org, optional float count) te_bloodqw = #239;
#ifdef SSQC
float(vector viewpos, entity entity) checkpvs = #240;
entity(string match, optional float matchnum) matchclientname = #241;
void(string dest, string content) sendpacket = #242;
#endif
#ifdef CSQC
vector(entity ent, float tagnum) rotatevectorsbytag = #244;
#endif
int(string) stoi = #259;
string(int) itos = #260;
int(string) stoh = #261;
string(int) htos = #262;
float(float modlindex, optional float useabstransforms) skel_create = #263;
float(float skel, entity ent, float modelindex, float retainfrac, float firstbone, float lastbone, optional float addfrac) skel_build = #264;
float(float skel) skel_get_numbones = #265;
string(float skel, float bonenum) skel_get_bonename = #266;
float(float skel, float bonenum) skel_get_boneparent = #267;
float(float skel, string tagname) skel_find_bone = #268;
vector(float skel, float bonenum) skel_get_bonerel = #269;
vector(float skel, float bonenum) skel_get_boneabs = #270;
void(float skel, float bonenum, vector org, optional vector fwd, optional vector right, optional vector up) skel_set_bone = #271;
void(float skel, float bonenum, vector org, optional vector fwd, optional vector right, optional vector up) skel_mul_bone = #272;
void(float skel, float startbone, float endbone, vector org, optional vector fwd, optional vector right, optional vector up) skel_mul_bones = #273;
void(float skeldst, float skelsrc, float startbone, float entbone) skel_copybones = #274;
void(float skel) skel_delete = #275;
void(float modidx, string framename) frameforname = #276;
float(float modidx, float framenum) frameduration = #277;
void(float action, vector pos, float radius, float quant) terrain_edit = #278;
void() touchtriggers = #279;
#ifdef SSQC
void(float buf, float fl) writefloat = #280;
#endif
float*(float skel) skel_mmap = #282;
void(entity ent, float bonenum, vector org, optional vector angorfwd, optional vector right, optional vector up) skel_set_bone_world = #283;
void*(int size) memalloc = #384;
void(void *ptr) memfree = #385;
void(void *dst, void *src, int size) memcpy = #386;
void(void *dst, int val, int size) memset = #387;
#ifdef CSQC
void() clearscene = #300;
void(float mask) addentities = #301;
void(entity ent) addentity = #302;
#define setviewprop setproperty
float(float property, ...) setproperty = #303;
void() renderscene = #304;
float(vector org, float radius, vector lightcolours) dynamiclight_add = #305;
void(string texturename, optional float flags) R_BeginPolygon = #306;
void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex = #307;
void() R_EndPolygon = #308;
#define getviewprop getproperty
__variant(float property) getproperty = #309;
vector (vector v) unproject = #310;
vector (vector v) project = #311;
void(float width, vector pos1, vector pos2) drawline = #315;
float(string name) iscachedpic = #316;
string(string name, float trywad) precache_pic = #317;
vector(string picname) draw_getimagesize = #318;
void(string name) freepic = #319;
float(vector position, float character, vector scale, vector rgb, float alpha, optional float flag) drawcharacter = #320;
float(vector position, string text, vector scale, vector rgb, float alpha, optional float flag) drawrawstring = #321;
float(vector position, string pic, vector size, vector rgb, float alpha, optional float flag) drawpic = #322;
float(vector position, vector size, vector rgb, float alpha, optional float flag) drawfill = #323;
void(float x, float y, float width, float height) drawsetcliparea = #324;
void(void) drawresetcliparea = #325;
float(vector position, string text, vector scale, vector rgb, float alpha, float flag) drawstring = #326;
float(string text, float usecolours, optional vector fontsize) stringwidth = #327;
void(vector pos, vector sz, string pic, vector srcpos, vector srcsz, vector rgb, float alpha, float flag) drawsubpic = #328;
float(float stnum) getstati = #330;
#define getstatf getstatbits
float(float stnum, optional float firstbit, optional float bitcount) getstatbits = #331;
string(float firststnum) getstats = #332;
void(entity e, float mdlindex) setmodelindex = #333;
string(float mdlindex) modelnameforindex = #334;
#endif
float(string effectname) particleeffectnum = #335;
void(float effectnum, entity ent, vector start, vector end) trailparticles = #336;
void(float effectnum, vector origin, optional vector dir, optional float count) pointparticles = #337;
#ifdef CSQC
void(string s, ...) cprint = #338;
#endif
void(string s, ...) print = #339;
#ifdef CSQC
string(float keynum) keynumtostring = #340;
float(string keyname) stringtokeynum = #341;
string(float keynum) getkeybind = #342;
vector() getmousepos = #344;
float(float framenum) getinputstate = #345;
void(float sens) setsensitivityscaler = #346;
#endif
void(entity ent) runstandardplayerphysics = #347;
#ifdef CSQC
string(float playernum, string keyname) getplayerkeyvalue = #348;
float() isdemo = #349;
float() isserver = #350;
void(vector origin, vector forward, vector right, vector up) SetListener = #351;
void(string cmdname) registercommand = #352;
#endif
float(entity ent) wasfreed = #353;
#ifdef CSQC
string(string key) serverkey = #354;
string() getentitytoken = #355;
void(string evname, string evargs, ...) sendevent = #359;
float() readbyte = #360;
float() readchar = #361;
float() readshort = #362;
float() readlong = #363;
float() readcoord = #364;
float() readangle = #365;
string() readstring = #366;
float() readfloat = #367;
float() readentitynum = #368;
float(string modelname, float(float isnew) updatecallback, float flags) deltalisten = #371;
__variant(float lno, float fld) dynamiclight_get = #372;
void(float lno, float fld, __variant value) dynamiclight_set = #373;
string(float efnum, float body) particleeffectquery = #374;
#endif
void(entity from, entity to) copyentity = #400;
#ifdef SSQC
void(entity from, entity to) setcolors = #401;
#endif
entity(string field, string match) findchain = #402;
entity(float fld, float match) findchainfloat = #403;
void(vector org, string modelname, float startframe, float endframe, float framerate) effect = #404;
void(vector org, vector dir, float count) te_blood = #405;
void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower = #406;
void(vector org, vector color) te_explosionrgb = #407;
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube = #408;
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlerain = #409;
void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlesnow = #410;
void(vector org, vector vel, float howmany) te_spark = #411;
void(vector org) te_gunshotquad = #412;
void(vector org) te_spikequad = #413;
void(vector org) te_superspikequad = #414;
void(vector org) te_explosionquad = #415;
void(vector org) te_smallflash = #416;
void(vector org, float radius, float lifetime, vector color) te_customflash = #417;
void(vector org) te_gunshot = #418;
void(vector org) te_spike = #419;
void(vector org) te_superspike = #420;
void(vector org) te_explosion = #421;
void(vector org) te_tarexplosion = #422;
void(vector org) te_wizspike = #423;
void(vector org) te_knightspike = #424;
void(vector org) te_lavasplash = #425;
void(vector org) te_teleport = #426;
void(vector org, float color, float colorlength) te_explosion2 = #427;
void(entity own, vector start, vector end) te_lightning1 = #428;
void(entity own, vector start, vector end) te_lightning2 = #429;
void(entity own, vector start, vector end) te_lightning3 = #430;
void(entity own, vector start, vector end) te_beam = #431;
void(vector dir) vectorvectors = #432;
void(vector org) te_plasmaburn = #433;
float(entity e, float s) getsurfacenumpoints = #434;
vector(entity e, float s, float n) getsurfacepoint = #435;
vector(entity e, float s) getsurfacenormal = #436;
string(entity e, float s) getsurfacetexture = #437;
float(entity e, vector p) getsurfacenearpoint = #438;
#ifdef SSQC
void(entity e, string s) clientcommand = #440;
#endif
float(string s) tokenize = #441;
string(float n) argv = #442;
#ifdef SSQC
void(entity e, entity tagentity, string tagname) setattachment = #443;
#endif
float(string pattern, float caseinsensitive, float quiet) search_begin = #444;
void(float handle) search_end = #445;
float(float handle) search_getsize = #446;
string(float handle, float num) search_getfilename = #447;
string(string cvarname) cvar_string = #448;
entity(entity start, .entity fld, float match) findflags = #449;
entity(.float fld, float match) findchainflags = #450;
float(entity ent, string tagname) gettagindex = #451;
vector(entity ent, float tagindex) gettaginfo = #452;
#ifdef SSQC
void(entity player) dropclient = #453;
entity() spawnclient = #454;
float(entity client) clienttype = #455;
void(float target, string str) WriteUnterminatedString = #456;
#endif
entity(float entnum) edict_num = #459;
float() buf_create = #460;
void(float bufhandle) buf_del = #461;
float(float bufhandle) buf_getsize = #462;
void(float bufhandle_from, float bufhandle_to) buf_copy = #463;
void(float bufhandle, float sortpower, float backward) buf_sort = #464;
string(float bufhandle, string glue) buf_implode = #465;
string(float bufhandle, float string_index) bufstr_get = #466;
void(float bufhandle, float string_index, string str) bufstr_set = #467;
float(float bufhandle, string str, float order) bufstr_add = #468;
void(float bufhandle, float string_index) bufstr_free = #469;
float(float s) asin = #471;
float(float c) acos = #472;
float(float t) atan = #473;
float(float c, float s) atan2 = #474;
float(float a) tan = #475;
float(string s) strlennocol = #476;
string(string s) strdecolorize = #477;
string(float uselocaltime, string format, ...) strftime = #478;
float(string s, string separator1, ...) tokenizebyseparator = #479;
string(string s) strtolower = #480;
string(string s) strtoupper = #481;
string(string s) cvar_defstring = #482;
#ifdef CSQC
void(vector origin, string sample, float volume, float attenuation) pointsound = #483;
#endif
string(string search, string replace, string subject) strreplace = #484;
string(string search, string replace, string subject) strireplace = #485;
//vector(entity e, float s, float n, float a) getsurfacepointattribute = #486;
#ifdef CSQC
float(string name) gecko_create = #487;
void(string name) gecko_destroy = #488;
void(string name, string URI) gecko_navigate = #489;
float(string name, float key, float eventtype) gecko_keyevent = #490;
void(string name, float x, float y) gecko_mousemove = #491;
void(string name, float w, float h) gecko_resize = #492;
vector(string name) gecko_get_texture_extent = #493;
#endif
float(float caseinsensitive, string s, ...) crc16 = #494;
float(string name) cvar_type = #495;
//float() numentityfields = #496;
//string(float fieldnum) entityfieldname = #497;
//float(float fieldnum) entityfieldtype = #498;
//string(float fieldnum, entity ent) getentityfieldstring = #499;
//float(float fieldnum, entity ent, string s) putentityfieldstring = #500;
//void(float to, string s, float sz) WritePicture = #501;
//string() ReadPicture = #501;
string(string filename) whichpack = #503;
#ifdef CSQC
__variant(float entnum, float fieldnum) getentity = #504;
#endif
string(string in) uri_escape = #510;
string(string in) uri_unescape = #511;
float(entity ent) num_for_edict = #512;
void(string str) tokenize_console = #514;
float(float idx) argv_start_index = #515;
float(float idx) argv_end_index = #516;
string(string cvarname) cvar_description = #518;
#ifdef CSQC
float(optional float timetype) gettime = #519;
#endif
void(string s) loadfromdata = #529;
void(string s) loadfromfile = #530;
#ifdef SSQC
float(string mname) precache_vwep_model = #532;
#endif
void(.../*, string funcname*/) callfunction = #605;
void(float fh, entity e) writetofile = #606;
float(string s) isfunction = #607;
void(entity e, string s) parseentitydata = #608;
string(...) sprintf = #627;
#pragma noref 0
