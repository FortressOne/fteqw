#include "r_partset.h"


char *particle_set_spikeset =
#if 1
/////////////////////////////////////////////////
//rocket trails (derived from purplehaze's, with only minor tweeks)

"r_part rockettrail\n"
"{\n"
"	texture \"particles/smoke.tga\"\n"
"	count 0.25\n"
"	scale 30\n"
"	alpha 0.3\n"
"	die 1.4\n"
"	diesubrand 0.7\n"
"	randomvel 1\n"
"	veladd 0\n"
"	red 255\n"
"	green 50 \n"
"	blue 10\n"
"	reddelta -255\n"
"	greendelta -25\n"
"	bluedelta -5\n"
"	gravity -25\n"
"	scalefactor 1\n"
"	assoc rocketsmoke\n"
"}\n"

"r_part t_rocket\n"
"{\n"
"	texture \"particles/rfire\"\n"
"	count 0.5\n"
"	scale 10\n"
"	alpha 0.6\n"
"	die 0.25\n"
"	randomvel 0\n"
"	veladd 0\n"
"	red 255\n"
"	green 192\n"
"	blue 128\n"
"	reddelta -14\n"
"	greendelta -300\n"
"	bluedelta -300\n"
"	blend add\n"
"	assoc rockettrail\n"
"	gravity 0\n"
"	scalefactor 0.8\n"
"	scaledelta -10\n"
"}\n"

"r_part rocketsmoke\n"
"{\n"
"	texture \"particles/rtrail\"\n"
"	step 8\n"
"	scale 7.5\n"
"	alpha 0.8\n"
"	die 2\n"
"	diesubrand 0\n"
"	randomvel 3\n"
"	veladd 0\n"
"	red 10\n"
"	green 10\n"
"	blue 10\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 1\n"
"	blend modulate\n"
"	spawnmode spiral\n"
"	scalefactor 1\n"
"	offsetspread 10\n"
"	offsetspreadvert 10\n"
"	areaspread 0\n"
"	areaspreadvert 0\n"
"}\n"
#elif 0
"r_part rockettail\n"
"{\n"
"	texture \"particles/rtrail\"\n"
"	step 7\n"
"	scale 10\n"
"	alpha 0.3\n"
"	die 10\n"
"	randomvel 64\n"
"	veladd 512\n"
"	red 192\n"
"	green 192\n"
"	blue 192\n"
"	gravity 100\n"
"	cliptype rockettail\n"
"}\n"
"\n"
"r_part t_rocket\n"
"{\n"
"	texture \"particles/rtrail\"\n"
"	step 4\n"
"	scale 10\n"
"	alpha 0.3\n"
"	die 0.7\n"
"	randomvel 32\n"
"	veladd 32\n"
"	red 255\n"
"	green 198\n"
"	blue 128\n"
"	reddelta -64\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity -100\n"
"	blend add\n"
"	assoc rockettail\n"
"}\n"
#else
"r_part t_rocket\n"
"{\n"
"	texture \"\"\n"
"	gravity -200\n"
"	step 40\n"
"	scale 10\n"
"	scaledelta 50\n"
"	alpha 0.5\n"
"	die 0.5\n"
"	red 254\n"
"	green 128\n"
"	blue 64\n"
"	blend add\n"
"	isbeam\n"
"	spawnmode spiral\n"
"	offsetspread 5\n"
"}\n"
#endif

//TeamFortress railgun (by model - this is also the effect used with the TE_LIGHTNING1 extension)
"r_part te_railtrail\n"
"{\n"
"	texture \"particles/b_rocket3\"\n"
"	step 15\n"
"	scale 10\n"
"	alpha 1\n"
"	die 1\n"
"	red 255\n"
"	green 255\n"
"	blue 255\n"
"	blend add\n"
"	isbeam\n"
"	spawnmode spiral\n"
"	offsetspread	100\n"
"	cliptype te_railtrail\n"
"	friction 0.7\n"
"}\n"
"r_trail progs/e_spike1.mdl te_railtrail\n"


"r_part t_grenade\n"
"{\n"
"	texture \"particles/rtrail\"\n"
"	step 10\n"
"	scale 10\n"
"	alpha 0.3\n"
"	die 10\n"
"	randomvel 32\n"
"	veladd 16\n"
"	red 128\n"
"	green 128\n"
"	blue 128\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 100\n"
"}\n"

//cool's blood trails (cos they're cooler)
"r_part t_gib\n"
"{\n"
"	texture \"particles/blood\"\n"
"	step 32\n"
"	scale 64\n"
"	alpha 0.6\n"
"	die 1\n"
"	randomvel 64\n"
"	veladd 10\n"
"	rotationspeed 90\n"
"	rotationstart 0 360\n"
"	red 128\n"
"	green 0\n"
"	blue 0\n"
"	blend blend\n"
"	gravity 200\n"
"	scalefactor 0.8\n"
"	scaledelta -10\n"
"	stains 5\n"
"}\n"
"r_part t_zomgib\n"
"{\n"
"	texture \"particles/blood\"\n"
"	step 64\n"
"	scale 64\n"
"	alpha 0.6\n"
"	die 1\n"
"	randomvel 64\n"
"	veladd 10\n"
"	rotationspeed 90\n"
"	rotationstart 0 360\n"
"	red 32\n"
"	green 0\n"
"	blue 0\n"
"	blend blend\n"
"	gravity 200\n"
"	scalefactor 0.8\n"
"	scaledelta -10\n"
"	stains 5\n"
"}\n"

"r_part t_tracer1\n"
"{\n"
"}\n"
"r_part t_tracer2\n"
"{\n"
"}\n"
"r_part t_tracer3\n"
"{\n"
"}\n"
"r_part te_lightningblood\n"
"{\n"
"	texture \"particles/bloodtrail\"\n"
"	count 1\n"
"	scale 15\n"
"	alpha 0.3\n"
"	die 10\n"
"	randomvel 32\n"
"	veladd 32\n"
"	red 192\n"
"	green 0\n"
"	blue 0\n"
"	reddelta -128\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 100\n"
"friction 1\n"
"	stains 1\n"
"	blend add\n"
"}\n"
"r_part te_blood\n"
"{\n"
"	texture \"particles/bloodtrail\"\n"
"	count 1\n"
"	scale 15\n"
"	alpha 0.3\n"
"	die 1\n"
"	randomvel 32\n"
"	veladd 32\n"
"	red 192\n"
"	green 0\n"
"	blue 0\n"
"	reddelta -128\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 100\n"
"friction 1\n"
"	stains 1\n"
"}\n"

#if 1
/////////////////////////////////////////////////
//rocket explosions

"r_part randomspark\n"
"{\n"
"	count 1\n"
"	texture \"\"\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	gravity 400\n"
"	spawnmode ball\n"
"	die 2\n"
"	blend add\n"
"	randomvel 128\n"
"	veladd 0\n"
"	cliptype randomspark\n"
"}\n"

"r_part insaneshrapnal\n"
"{\n"
"	count 24\n"
"	texture \"\"\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	gravity 400\n"
"	die 2\n"
"	blend add\n"
"	randomvel 512\n"
"	veladd 1\n"
"	cliptype randomspark\n"
"	clipcount 5\n"
"}\n"

"r_part ember\n"
"{\n"
"	count 1\n"
"	texture \"particles/explosion\"\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	alpha 0\n"
"	scale 15\n"
"	scalefactor 1\n"
"	friction 8\n"
"	gravity 50\n"
"	die 1\n"
"	blend add\n"
"	randomvel 5\n"
"	veladd 1\n"
"	rampmode delta\n"	//fade it in then out.
"	ramp 0    0    0    -0.5   0\n"
"	ramp 0    0    0    0.1    0\n"
"	ramp 0    0    0    0.1    0\n"
"	ramp 0    0    0    0.1    0\n"
"	ramp 0    0    0    0.1    0\n"
"	ramp 0    0    0    0.1    0\n"
"}\n"

//the bits that fly off
"r_part expgib\n"
"{\n"
"	cliptype expgib\n"
"	texture \"particles/explosion\"\n"
"	count	16\n"
"	scale 0\n"
"	die 1\n"
"	randomvel 128\n"
"	veladd 64\n"
"	veladd 0\n"
"	gravity 50\n"
"	friction 2\n"
"	emit ember\n"
"	emitinterval 0.01\n"
"	spawnmode circle\n"
"	assoc insaneshrapnal\n"
"}\n"

//the heart of the explosion
"r_part te_explosion\n"
"{\n"
"	texture \"particles/explosion\"\n"
"	count	1\n"
"	scale 200\n"
"	scalefactor 1\n"
"	alpha 1\n"
"	die 1\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	reddelta 0\n"
"	greendelta -32\n"
"	reddelta -32\n"
"	gravity 0\n"
"	friction 1\n"
"	stains 0\n"
"	blend add\n"
"	assoc expgib\n"
"}\n"
#else
"r_part sparks\n"
"{\n"
"	texture \"\"\n"
"	count	256\n"
"	scale 1\n"
"	alpha 0.7\n"
"	die 1\n"
"	randomvel 512\n"
"	veladd 128\n"
"	red 255\n"
"	green 128\n"
"	gravity 800\n"
"	blend add\n"
"	cliptype sparks\n"
"	clipcount 1\n"
"}\n"
"\n"
"r_part shrapnal\n"
"{\n"
"	texture \"\"\n"
"	count	256\n"
"	scale 1\n"
"	alpha 0.7\n"
"	die 1\n"
"	randomvel 512\n"
"	veladd 128\n"
"	red 255\n"
"	green 128\n"
"	gravity 800\n"
"	cliptype sparks\n"
"	clipcount 3\n"
"	blend add\n"
"	assoc sparks\n"
"}\n"
"\n"
"r_part smallshrapnal\n"
"{\n"
"	texture \"\"\n"
"	count	32\n"
"	scale 1\n"
"	alpha 0.7\n"
"	die 1\n"
"	randomvel 512\n"
"	veladd 128\n"
"	red 255\n"
"	green 128\n"
"	gravity 800\n"
"	cliptype sparks\n"
"	clipcount 3\n"
"	blend add\n"
"	assoc sparks\n"
"}\n"
"\n"
"r_part emittest2\n"
"{\n"
"	texture \"particles/bloodtrail\"\n"
"	step 4\n"
"	scalefactor 1\n"
"	scale 10\n"
"	alpha 1\n"
"	die 1\n"
"	diesubrand 0.5\n"
"	randomvel 0\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity -100\n"
"	blend add\n"
"friction 0\n"
"	stains 0\n"
"}\n"
"\n"
"r_part emittest\n"
"{\n"
"	texture \"particles/bloodtrail\"\n"
"	count 25\n"
"	scale 15\n"
"	scalefactor 1\n"
"	alpha 1\n"
"	die 10\n"
"	diesubrand 5\n"
"	randomvel 128\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	gravity 800\n"
"	blend add\n"
"friction 0\n"
"	cliptype emittest\n"
"	clipcount 1\n"
"	stains 0\n"
"	emit emittest2\n"
"	emitinterval -1\n"
"	assoc shrapnal\n"
"	emitintervalrand 0\n"
"}\n"
"\n"

//fixme: 16?!?!
"r_part te_explosion\n"
"{\n"
"	texture \"particles/explosion\"\n"
"	count	16\n"
"	scale 100\n"
"	alpha 0.7\n"
"	die 4\n"
"	randomvel 32\n"
"	veladd 0\n"
"	red  255\n"
"	green 128\n"
"	blue 76\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 0\n"
"friction 1\n"
"	stains 0\n"
"	blend add\n"
"	_assoc emittest\n"
"	assoc shrapnal\n"
"	scalefactor 1\n"
"}\n"

#endif

"r_part empcentral\n"
"{\n"
"	texture \"particles/emp\"\n"
"	count	100\n"
"	scale 100\n"
"	alpha 0.4\n"
"	die 6\n"
"	randomvel 0\n"
"	veladd -1\n"
"	red 128\n"
"	green 128\n"
"	blue 255\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 0\n"
"friction 0.2\n"
"	stains 0\n"
"	blend add\n"
"	assoc shrapnal\n"
"	spawnmode circle\n"
"	areaspread 64\n"
"	areaspreadvert 64\n"
"	offsetspread 72\n"
"	offsetspreadvert 0\n"
"}\n"
"r_part empinner\n"
"{\n"
"	texture \"particles/emp\"\n"
"	count	75\n"
"	scale 100\n"
"	alpha 0.4\n"
"	die 4\n"
"	randomvel 0\n"
"	veladd -1\n"
"	red 128\n"
"	green 128\n"
"	blue 255\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 0\n"
"frictioon 0.2\n"
"	stains 0\n"
"	blend add\n"
"	assoc empcentral\n"
"	spawnmode circle\n"
"	areaspread 8\n"
"	areaspreadvert 0\n"
"	offsetspread 64\n"
"	offsetspreadvert 0\n"
"}\n"
"//the blob tempent is used quite a bit with teamfortress emp grenades.\n"
"r_part te_blob\n"
"{\n"
"	texture \"particles/emp\"\n"
"	count	100\n"
"	scale 100\n"
"	alpha 0.4\n"
"	die 4\n"
"	randomvel 0\n"
"	veladd -1\n"
"	red 128\n"
"	green 255\n"
"	blue 128\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 0\n"
"friction 1\n"
"	stains 0\n"
"	blend add\n"
"	assoc empinner\n"
"	spawnmode circle\n"
"	areaspread 64\n"
"	areaspreadvert 0\n"
"	offsetspread 256\n"
"	offsetspreadvert 0\n"
"}\n"
"\n"
"\n"
"r_part te_gunshotsparks\n"
"{\n"
"	texture \"\"\n"
"	count	0.5\n"
"	scale 1\n"
"	alpha 0.7\n"
"	die 10\n"
"	randomvel 64\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	gravity 200\n"
"	blend add\n"
"	cliptype te_gunshotsparks\n"
"	clipcount 1\n"
"}\n"
"\n"
"r_part te_gunshot\n"
"{\n"
"	texture \"\"\n"
"	count	0.5\n"
"	scale 1\n"
"	alpha 0.7\n"
"	die 10\n"
"	randomvel 64\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	gravity 200\n"
"	cliptype te_gunshotsparks\n"
"	clipcount 3\n"
"	blend add\n"
"	assoc te_gunshotsparks\n"
"}\n"
"\n"
"r_part te_lavasplash\n"
"{\n"
"	texture \"\"\n"
"	count	654\n"
"	scale 15\n"
"	alpha 0.7\n"
"	die 10\n"
"	randomvel 64\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	blue 128\n"
"	gravity 50\n"
"	blend add\n"
"	areaspread 512\n"
"	areaspreadvert 64\n"
"}\n"
"\n"
//////////////////////////////////////////////////
//Teleport splash

#if 1	//a simpler effect.
"r_part te_teleportsplash\n"
"{\n"
"	count 64\n"
"	spawnmode ball\n"
"	areaspread 48\n"
"	areaspreadvert 48\n"
"	texture \"textures/smoke\"\n"
"	blend blend\n"
"	die 1\n"
"	friction 2\n"
"	gravity 0\n"
"	scale 64\n"
"	alpha 0.2\n"
"	randomvel 64\n"
"	red 255\n"
"	green 255\n"
"	blue 255\n"
"	scalefactor 1\n"
"}\n"
#else // the old red ball effect.
"r_part te_teleportsplash\n"
"{\n"
"	texture \"particles/teleport\"\n"
"	count	4192\n"		//EGAD!!!! 4192?!??! What was I thinking?!?!? (it's a very pwetty effect though...)
"	scale 2\n"
"	scalefactor 1\n"
"	alpha 1\n"
"	die 1\n"
"	randomvel 90\n"
"	veladd -100\n"
"	red 255\n"
"	green 0\n"
"	blue 0\n"
"	gravity 200\n"
"	friction 2\n"
"	blend add\n"
"	areaspread 32\n"
"	areaspreadvert 32\n"
"	offsetspread -128\n"
"	offsetspreadvert 64\n"
"spawnmode circle\n"
"}\n"
#endif
"\n"
"//flame effect\n"
"r_part cu_flame\n"
"{\n"
"	texture \"particles/flame\"\n"
"	count	1024\n"
"	scale 0.4\n"
"	scalerand  6\n"
"	scalefactor 1\n"
"	alpha 0.4\n"
"	die 0.8\n"
"	randomvel 4 24\n"
"	veladd -24\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 0\n"
"friction 0\n"
"	stains 0\n"
"	blend add\n"
"	areaspread 6\n"
"	up -8\n"
"	areaspreadvert 0\n"
"	spawnmode box\n"
"	offsetspread -15\n"
"}\n"
"//flame effect\n"
"r_part cu_torch\n"
"{\n"
"	texture \"particles/flame\"\n"
"		count	256\n"
"	scale 3\n"
"	scalefactor 1\n"
"	alpha 0.7\n"
"	die 0.5\n"
"	randomvel 8\n"
"	veladd -32\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 0\n"
"friction 0\n"
"	stains 0\n"
"	blend add\n"
"	areaspread 4\n"
"	areaspreadvert 1\n"
"	spawnmode circle\n"
"	offsetspread -12\n"
"	offsetspreadvert -8\n"
"}\n"
"\n"
"r_part explodesprite\n"
"{\n"
"	texture \"particles/flame\"\n"
"	count	1\n"
"	scale 80\n"
"	scalefactor 1\n"
"	alpha 0.2\n"
"	die 2\n"
"	randomvel 23\n"
"	veladd -20\n"
"	red 255\n"
"	green 128\n"
"	blue 76\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 0\n"
"friction 0\n"
"	stains 0\n"
"	blend add\n"
"	areaspread 4\n"
"	areaspreadvert 1\n"
"	spawnmode box\n"
"	offsetspread -8\n"
"	offsetspreadvert -8\n"
"	assoc smallshrapnal\n"
"}\n"
"r_effect \"progs/s_explod.spr\" explodesprite 1\n"
"r_effect \"progs/flame.spr\" explodesprite 1\n"
"\n"
"r_effect \"progs/flame2.mdl\" cu_flame 1\n"
"r_effect \"progs/flame.mdl\" cu_torch\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"//you'll probably never see this one\n"
"r_part ef_darkfield\n"
"{\n"
"	texture \"fgh\"\n"
"	count	1\n"
"	scale 2\n"
"	scaledelta 2\n"
"	alpha 0.3\n"
"	die 5\n"
"	rrandomvel 8\n"
"	veladd 0\n"
"	red 255\n"
"	green 0\n"
"	blue 0\n"
"	gravity 0\n"
"	blend add\n"
"	areaspread 0\n"
"	areaspreadvert 0\n"
"}\n"
"\n"
"//you'll probably never see this one\n"
"r_part ef_entityparticles\n"
"{\n"
"	texture \"j\"\n"
"	count	1\n"
"	scale 15\n"
"	alpha 0.2\n"
"	die 0\n"
"	randomvel 0\n"
"	veladd 16\n"
"	red 255\n"
"	green 128\n"
"	blue 128\n"
"	gravity 0\n"
"	blend add\n"
"	areaspread 0\n"
"	areaspreadvert 0\n"
"}\n";



char *particle_set_highfps =	//submitted by 'ShadowWalker'
"r_part t_gib\n"
"{\n"
"	texture \"particles/bloodtrail\"\n"
"	step 4\n"
"	scale 40\n"
"	scaledelta 0\n"
"	alpha 0.5\n"
"	die 3\n"
"	randomvel 256\n"
"	veladd 128\n"
"	red 64\n"
"	green 0\n"
"	blue 0\n"
"	reddelta -128\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 1000\n"
"friction 1\n"
"	stains 1\n"
"}\n"
"r_part t_zomgib\n"
"{\n"
"	texture \"particles/bloodtrail\"\n"
"	step 4\n"
"	scale 15\n"
"	alpha 0.3\n"
"	die 10\n"
"	randomvel 256\n"
"	veladd 128\n"
"	red 192\n"
"	green 0\n"
"	blue 0\n"
"	reddelta -128\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 1000\n"
"friction 1\n"
"	stains 0\n"
"}\n"
"r_part t_tracer1\n"
"{\n"
"}\n"
"r_part t_tracer2\n"
"{\n"
"}\n"
"r_part t_tracer3\n"
"{\n"
"}\n"
"r_part te_lightningblood\n"
"{\n"
"	texture \"particles/bloodtrail\"\n"
"	count 4\n"
"	scale 15\n"
"	alpha 0.3\n"
"	die 10\n"
"	randomvel 128\n"
"	veladd 128\n"
"	red 192\n"
"	green 0\n"
"	blue 0\n"
"	reddelta -128\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 100\n"
"friction 1\n"
"	stains 0\n"
"	blend add\n"
"}\n"
"r_part te_blood\n"
"{\n"
"	texture \"particles/bloodtrail\"\n"
"	count 4\n"
"	scale 30\n"
"	alpha 0.8\n"
"	die 5\n"
"	randomvel 32\n"
"	veladd 64\n"
"	offsetspreadvert 10\n"
"	red 32\n"
"	green 0\n"
"	blue 0\n"
"	reddelta -128\n"
"	greendelta 0\n"
"	reddeelta 0\n"
"	gravity 20\n"
"friction 1\n"
"	stains 0\n"
"}\n"
"r_part sparks\n"
"{\n"
"	texture \"\"\n"
"	count	128\n"
"	scale 1\n"
"	alpha 0.7\n"
"	die 0.75\n"
"	randomvel 512\n"
"	veladd 128\n"
"	red 255\n"
"	green 128\n"
"	gravity 800\n"
"	blend add\n"
"	cliptype sparks\n"
"	clipcount 1\n"
"}\n"
"\n"
"//the blob tempent is used quite a bit with teamfortress emp grenades.\n"
"r_part te_blob\n"
"{\n"
"	texture \"particles/emp\"\n"
"	count	100\n"
"	scale 100\n"
"	alpha 0.4\n"
"	die 4\n"
"	rrandomvel 0\n"
"	veladd -1\n"
"	red 128\n"
"	green 255\n"
"	blue 128\n"
"	reddelta 0\n"
"	greendelta 0\n"
"	reddelta 0\n"
"	gravity 0\n"
"friction 1\n"
"	stains 0\n"
"	blend add\n"
"	assoc empinner\n"
"	spawnmode circle\n"
"	areaspread 64\n"
"	areaspreadvert 0\n"
"	offsetspread 256\n"
"	offsetspreadvert 0\n"
"}\n"
"\n"
"\n"
"r_part te_gunshotsparks\n"
"{\n"
"	texture \"\"\n"
"	count	0.5\n"
"	scale 0.75\n"
"	alpha 0.7\n"
"	die 10\n"
"	randomvel 64\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	gravity 200\n"
"	blend add\n"
"	cliptype te_gunshotsparks\n"
"	clipcount 1\n"
"}\n"
"\n"
"r_part te_gunshot\n"
"{\n"
"	texture \"\"\n"
"	count	2\n"
"	scale 1\n"
"	alpha 0.7\n"
"	die 0.75\n"
"	randomvel 64\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	gravity 200\n"
"	cliptype te_gunshotsparks\n"
"	clipcount 3\n"
"	blend add\n"
"	assoc te_gunshotsparks\n"
"}\n"
"\n"
"r_part te_lavasplash\n"
"{\n"
"	texture \"\"\n"
"	count	654\n"
"	scale 15\n"
"	alpha 0.7\n"
"	die 10\n"
"	randomvel 64\n"
"	veladd 0\n"
"	red 255\n"
"	green 128\n"
"	blue 128\n"
"	gravity 50\n"
"	blend add\n"
"	areaspread 512\n"
"	areaspreadvert 64\n"
"}\n"
"\n"
"r_part te_teleportsplash\n"
"{\n"
"	texture \"particles/teleport\"\n"
"	count	128\n"
"	scale 40\n"
"	scalefactor 1\n"
"	alpha 1\n"
"	die 1\n"
"	randomvel 63\n"
"	veladd 0\n"
"	red 128\n"
"	green 128\n"
"	blue 128\n"
"	gravity 200\n"
"	friction 2\n"
"	blend add\n"
"	areaspread 4\n"
"	areaspreadvert 32\n"
"	offsetspread 50\n"
"	offsetspreadvert 8\n"
"spawnmode telesquare\n"
"}\n"
"\n"
"\n"
"//you'll probably never see this one\n"
"r_part ef_darkfield\n"
"{\n"
"	texture \"fgh\"\n"
"	count	1\n"
"	scale 2\n"
"	scaledelta 2\n"
"	alpha 0.3\n"
"	die 0.5\n"
"	randomvel 8\n"
"	veladd 25\n"
"	red 255\n"
"	green 0\n"
"	blue 0\n"
"	gravity 0\n"
"	blend add\n"
"	areaspread 0\n"
"	areaspreadvert 0\n"
"}\n"
"\n"
"//you'll probably never see this one\n"
"r_part ef_entityparticles\n"
"{\n"
"	texture \"j\"\n"
"	count	1\n"
"	scale 10\n"
"	alpha 0.3\n"
"	die 0\n"
"	randomvel 0\n"
"	veladd 16\n"
"	red 128\n"
"	green 128\n"
"	blue 0\n"
"	gravity 0\n"
"	blend add\n"
"	areaspread 0\n"
"	areaspreadvert 0\n"
"}\n";

char *particle_set_faithful =
"r_part t_gib\n"
"{\n"
"	texture \"particles/quake\"\n"
"	step 3\n"
"	scale 4\n"
"	alpha 1\n"
"	die 2\n"
"	alphachange 0\n"
"	randomvel 80\n"
"	veladd 100\n"
"	colorindex 67\n"
"	colorrand 4\n"
"	gravity 40\n"
"	areaspread 3\n"
"	areaspreadvert 3\n"
"	spawnmode box\n"
"	stains 1\n"
"}\n"
"\n"
"r_part t_zomgib\n"
"{\n"
"	texture \"particles/quake\"\n"
"	step 6\n"
"	scale 4\n"
"	alpha 1\n"
"	die 2\n"
"	alphachange 0\n"
"	randomvel 72\n"
"	veladd 100\n"
"	colorindex 67\n"
"	colorrand 4\n"
"	gravity 40\n"
"	areaspread 3\n"
"	areaspreadvert 3\n"
"	spawnmode box\n"
"	stains 1\n"
"}\n"
"\n"
"r_part t_tracer3\n"
"{\n"
"	texture \"particles/quake\"\n"
"	step 3\n"
"	scale 4\n"
"	alpha 1\n"
"	die 0.3\n"
"	alphachange 0\n"
"	colorindex 152\n"
"	colorrand 4\n"
"	areaspread 8\n"
"	areaspreadvert 8\n"
"	spawnmode box\n"
"}\n"
"\n"
"r_part t_tracer\n"
"{\n"
"	texture \"particles/quake\"\n"
"	step 3\n"
"	scale 4\n"
"	alpha 1\n"
"	die 0.5\n"
"	alphachange 0\n"
"	colorindex 52\n"
"	citracer 1\n"
"	offsetspread 30\n"
"	spawnmode tracer\n"
"}\n"
"\n"
"r_part t_tracer2\n"
"{\n"
"	texture \"particles/quake\"\n"
"	step 3\n"
"	scale 4\n"
"	alpha 1\n"
"	die 0.5\n"
"	alphachange 0\n"
"	colorindex 230\n"
"	citracer 1\n"
"	offsetspread 30\n"
"	spawnmode tracer\n"
"}\n"
"\n"
"r_part t_rocket\n"
"{\n"
"	texture \"particles/quake\"\n"
"	step 3\n"
"	scale 4\n"
"	die 1.2\n"
"	diesubrand 0.6\n"
"	rampmode absolute\n"
"	rampindex 109 1.0\n"
"	rampindex 107 0.833\n"
"	rampindex 6 0.667\n"
"	rampindex 5 0.5\n"
"	rampindex 4 0.333\n"
"	rampindex 3 0.167\n"
"	areaspread 3\n"
"	areaspreadvert 3\n"
"	gravity -40\n"
"	spawnmode box\n"
"}\n"
"\n"
"r_part t_grenade\n"
"{\n"
"	texture \"particles/quake\"\n"
"	step 3\n"
"	scale 4\n"
"	die 0.8\n"
"	diesubrand 0.6\n"
"	rampmode absolute\n"
"	rampindex 6 0.667\n"
"	rampindex 5 0.5\n"
"	rampindex 4 0.333\n"
"	rampindex 3 0.167\n"
"	areaspread 3\n"
"	areaspreadvert 3\n"
"	gravity -40\n"
"	spawnmode box\n"
"}\n"
"\n"
"r_part pe_size3\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 1\n"
"	scale 4\n"
"	veladd 15\n"
"	alpha 1\n"
"	die 0.4\n"
"	alphachange 0\n"
"	diesubrand 0.4\n"
"	gravity 40\n"
"	areaspread 24\n"
"	areaspreadvert 24\n"
"	spawnmode box	\n"
"}\n"
"\n"
"r_part pe_size2\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 1\n"
"	scale 4\n"
"	veladd 15\n"
"	alpha 1\n"
"	die 0.4\n"
"	alphachange 0\n"
"	diesubrand 0.4\n"
"	gravity 40\n"
"	areaspread 16\n"
"	areaspreadvert 16\n"
"	spawnmode box	\n"
"}\n"
"\n"
"r_part pe_default\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 1\n"
"	scale 4\n"
"	veladd 15\n"
"	alpha 1\n"
"	die 0.4\n"
"	alphachange 0\n"
"	diesubrand 0.4\n"
"	gravity 40\n"
"	areaspread 8\n"
"	areaspreadvert 8\n"
"	spawnmode box	\n"
"}\n"
"\n"
"r_part explode2\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 512\n"
"	alpha 1\n"
"	scale 4\n"
"	alphachange 0\n"
"	die 0.5333\n"
"	diesubrand 0.2667\n"
"	rampmode absolute\n"
"	rampindexlist 111 110 109 108 107 106 104 102 \n"
"	randomvel 256\n"
"	gravity 40\n"
"	friction 1\n"
"	areaspread 16\n"
"	areaspreadvert 16\n"
"	spawnmode box\n"
"}\n"
"\n"
"r_part te_explosion\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 512\n"
"	alpha 1\n"
"	scale 4\n"
"	die 0.8\n"
"	diesubrand 0.4\n"
"	randomvel 256\n"
"	rampmode absolute\n"
"	rampindexlist 111 109 107 105 103 101 99 97 \n"
"	gravity 40\n"
"	friction -4\n"
"	areaspread 16\n"
"	areaspreadvert 16\n"
"	spawnmode box\n"
"	assoc explode2\n"
"}\n"
"\n"
"r_part blobexp2b\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 256\n"
"	alpha 1\n"
"	scale 4\n"
"	alphachange 0\n"
"	die 1.4\n"
"	colorindex 150\n"
"	colorrand 6\n"
"	gravity 40\n"
"	friction 4 0\n"
"	areaspread 16\n"
"	areaspreadvert 16\n"
"	randomvel 256\n"
"	spawnmode box\n"
"}\n"
"r_part blobexp1b\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 256\n"
"	alpha 1\n"
"	scale 4\n"
"	alphachange 0\n"
"	die 1.4\n"
"	colorindex 66\n"
"	colorrand 6\n"
"	gravity 40\n"
"	friction -4 0\n"
"	areaspread 16\n"
"	areaspreadvert 16\n"
"	randomvel 256\n"
"	spawnmode box\n"
"	assoc blobexp2b\n"
"}\n"
"\n"
"r_part blobexp2\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 256\n"
"	alpha 1\n"
"	scale 4\n"
"	alphachange 0\n"
"	die 1\n"
"	colorindex 150\n"
"	colorrand 6\n"
"	gravity 40\n"
"	friction 4 0\n"
"	areaspread 16\n"
"	areaspreadvert 16\n"
"	randomvel 256\n"
"	spawnmode box\n"
"	assoc blobexp1b\n"
"}\n"
"r_part te_blob\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 256\n"
"	alpha 1\n"
"	scale 4\n"
"	alphachange 0\n"
"	die 1\n"
"	colorindex 66\n"
"	colorrand 6\n"
"	gravity 40\n"
"	friction -4 0\n"
"	areaspread 16\n"
"	areaspreadvert 16\n"
"	randomvel 256\n"
"	spawnmode box\n"
"	assoc blobexp2\n"
"}\n"
"\n"
"r_part te_teleportsplash\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 896\n"
"	alpha 1\n"
"	scale 4\n"
"	alphachange 0\n"
"	die 0.34\n"
"	diesubrand 0.14\n"
"	colorindex 7\n"
"	colorrand 8\n"
"	gravity 40\n"
"	areaspread 16\n"
"	areaspreadvert 28\n"
"	offsetspread 113\n"
"	offsetspreadvert 113\n"
"	up 4\n"
"	spawnmode telebox\n"
"}\n"
"\n"
"r_part te_lavasplash\n"
"{\n"
"	texture \"particles/quake\"\n"
"	count 1024\n"
"	alpha 1\n"
"	scale 4\n"
"	alphachange 0\n"
"	die 2.62\n"
"	diesubrand 0.62\n"
"	colorindex 224\n"
"	colorrand 8\n"
"	gravity 40\n"
"	areaspread 128\n"
"	areaspreadvert 63\n"
"	offsetspread 113\n"
"	offsetspreadvert 113\n"
"	spawnmode lavasplash\n"
"}\n";

