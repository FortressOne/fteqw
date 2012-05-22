#ifndef _CLASSES_QC_
	#define _CLASSES_QC_


#define eclasses	eclass(CLASS_ROCKET, ParseRocketClass)	eclass(CLASS_GIB, ParseGibbing)	eclass(CLASS_PLAYER, ParsePlayer) eclass(CLASS_EXPLOSION, ParseExplosion) eclass(CLASS_NAIL, ParseNailClass)

#ifdef CSQC
#define eclass(eid,func) void(float isnew) func;
		eclasses
#undef eclass
#endif


	enum {
#define eclass(eid,func) eid,
		eclasses CLASS_MAX
#undef eclass
		};

	enum {
		GIB_BAD,
		GIB_PLAYER,
		GIB_SOLDIER,
		GIB_ZOMBIE,
		GIB_DEMON,
		GIB_OGRE,
		GIB_DOG,
		GIB_WIZARD,
		GIB_SHAMBLER,
		GIB_KNIGHT,
		GIB_HELLKNIGHT,
		GIB_FISH,
		GIB_ENFORCER,
		GIB_SHALRATH,
		GIB_MAX
	};
#else
	#ifdef CSQC
		nonstatic var void(float isnew) ParseEntityClass[ ] = {
#define eclass(eid,func) func ,
		eclasses
#undef eclass
__NULL__
		};
	#endif
#endif



.float action;
enum
{
	ACT_IDLE,
	ACT_WALK,
	ACT_RUN,
	ACT_ATTACK1,
	ACT_ATTACK2,
	ACT_ATTACK3,
	ACT_ATTACK4,
	ACT_PAIN1,
	ACT_PAIN2,
	ACT_DIE1,
	ACT_DIE2,
	ACT_DIE3,
	ACT_DIE4,
	ACT_DIE5,

	ACT_COUNT
};
#define ACT_DEATH1 ACT_DIE1
#define ACT_DEATH2 ACT_DIE2
#define ACT_DEATH3 ACT_DIE3
#define ACT_DEATH4 ACT_DIE4
#define ACT_DEATH5 ACT_DIE5

enumflags
{
	SF_INITIAL = 1,	//automatically set by the engine as part of a 'entity newly visible' sendflags|=everything
	SF_ACTION = 2,		//.action changed
	SF_ANGLES = 4,		//changeyawed
	SF_ORIGIN = 8,		//walkmoved
	SF_VELOCITY = 16,	//velocity changed
	SF_FLAGS = 32,		//flags
	SF_FORCE = 65536	//forces *something* to be sent, even if there's no point to it.
};