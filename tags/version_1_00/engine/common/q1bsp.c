#include "quakedef.h"

#include "pr_common.h"
/*

============================================================================

Physics functions (common)
*/

void Q1BSP_CheckHullNodes(hull_t *hull)
{
	int num, c;
	dclipnode_t	*node;
	for (num = hull->firstclipnode; num < hull->lastclipnode; num++)
	{
		node = hull->clipnodes + num;
		for (c = 0; c < 2; c++)
			if (node->children[c] >= 0)
				if (node->children[c] < hull->firstclipnode || node->children[c] > hull->lastclipnode)
					Sys_Error ("Q1BSP_CheckHull: bad node number");

	}
}

/*
==================
SV_HullPointContents

==================
*/
static int Q1_HullPointContents (hull_t *hull, int num, vec3_t p)
{
	float		d;
	dclipnode_t	*node;
	mplane_t	*plane;

	while (num >= 0)
	{
		node = hull->clipnodes + num;
		plane = hull->planes + node->planenum;

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	return num;
}



#define	DIST_EPSILON	(0.03125)
#if 0
enum
{
	rht_solid,
	rht_empty,
	rht_impact
};
vec3_t rht_start, rht_end;
static int Q1BSP_RecursiveHullTrace (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	dclipnode_t	*node;
	mplane_t	*plane;
	float		t1, t2;
	vec3_t		mid;
	int			side;
	float		midf;
	int rht;

reenter:

	if (num < 0)
	{
		/*hit a leaf*/
		if (num == Q1CONTENTS_SOLID)
		{
			if (trace->allsolid)
				trace->startsolid = true;
			return rht_solid;
		}
		else
		{
			trace->allsolid = false;
			if (num == Q1CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
			return rht_empty;
		}
	}

	/*its a node*/

	/*get the node info*/
	node = hull->clipnodes + num;
	plane = hull->planes + node->planenum;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
	}

	/*if its completely on one side, resume on that side*/
	if (t1 >= 0 && t2 >= 0)
	{
		//return Q1BSP_RecursiveHullTrace (hull, node->children[0], p1f, p2f, p1, p2, trace);
		num = node->children[0];
		goto reenter;
	}
	if (t1 < 0 && t2 < 0)
	{
		//return Q1BSP_RecursiveHullTrace (hull, node->children[1], p1f, p2f, p1, p2, trace);
		num = node->children[1];
		goto reenter;
	}

	if (plane->type < 3)
	{
		t1 = rht_start[plane->type] - plane->dist;
		t2 = rht_end[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct (plane->normal, rht_start) - plane->dist;
		t2 = DotProduct (plane->normal, rht_end) - plane->dist;
	}

	side = t1 < 0;

	midf = t1 / (t1 - t2);
	if (midf < p1f) midf = p1f;
	if (midf > p2f) midf = p2f;
	VectorInterpolate(rht_start, midf, rht_end, mid);

	rht = Q1BSP_RecursiveHullTrace(hull, node->children[side], p1f, midf, p1, mid, trace);
	if (rht != rht_empty)
		return rht;
	rht = Q1BSP_RecursiveHullTrace(hull, node->children[side^1], midf, p2f, mid, p2, trace);
	if (rht != rht_solid)
		return rht;

	trace->fraction = midf;
	if (side)
	{
		/*we impacted the back of the node, so flip the plane*/
		trace->plane.dist = -plane->dist;
		VectorNegate(plane->normal, trace->plane.normal);
		midf = (t1 + DIST_EPSILON) / (t1 - t2);
	}
	else
	{
		/*we impacted the front of the node*/
		trace->plane.dist = plane->dist;
		VectorCopy(plane->normal, trace->plane.normal);
		midf = (t1 - DIST_EPSILON) / (t1 - t2);
	}
	VectorCopy (mid, trace->endpos);
	VectorInterpolate(rht_start, midf, rht_end, trace->endpos);

	return rht_impact;
}

qboolean Q1BSP_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	if (VectorEquals(p1, p2))
	{
		/*points cannot cross planes, so do it faster*/
		switch(Q1_HullPointContents(hull, num, p1))
		{
		case Q1CONTENTS_SOLID:
			trace->startsolid = true;
			break;
		case Q1CONTENTS_EMPTY:
			trace->allsolid = false;
			trace->inopen = true;
			break;
		default:
			trace->allsolid = false;
			trace->inwater = true;
			break;
		}
		return true;
	}
	else
	{
		VectorCopy(p1, rht_start);
		VectorCopy(p2, rht_end);
		return Q1BSP_RecursiveHullTrace(hull, num, p1f, p2f, p1, p2, trace) != rht_impact;
	}
}

#else
qboolean Q1BSP_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	dclipnode_t	*node;
	mplane_t	*plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

// check for empty
	if (num < 0)
	{
		if (num != Q1CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == Q1CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
		}
		else
			trace->startsolid = true;
		return true;		// empty
	}

//
// find the point distances
//
	node = hull->clipnodes + num;
	plane = hull->planes + node->planenum;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
	}

#if 1
	if (t1 >= 0 && t2 >= 0)
		return Q1BSP_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if (t1 < 0 && t2 < 0)
		return Q1BSP_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#else
	if ( (t1 >= DIST_EPSILON && t2 >= DIST_EPSILON) || (t2 > t1 && t1 >= 0) )
		return Q1BSP_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if ( (t1 <= -DIST_EPSILON && t2 <= -DIST_EPSILON) || (t2 < t1 && t1 <= 0) )
		return Q1BSP_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#endif

// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0)
		frac = (t1 + DIST_EPSILON)/(t1-t2);
	else
		frac = (t1 - DIST_EPSILON)/(t1-t2);
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	side = (t1 < 0);

// move up to the node
	if (!Q1BSP_RecursiveHullCheck (hull, node->children[side], p1f, midf, p1, mid, trace) )
		return false;

#ifdef PARANOID
	if (Q1BSP_RecursiveHullCheck (sv_hullmodel, mid, node->children[side])
	== Q1CONTENTS_SOLID)
	{
		Con_Printf ("mid PointInHullSolid\n");
		return false;
	}
#endif

	if (Q1_HullPointContents (hull, node->children[side^1], mid)
	!= Q1CONTENTS_SOLID)
// go past the node
		return Q1BSP_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);

	if (trace->allsolid)
		return false;		// never got out of the solid area

//==================
// the other side of the node is solid, this is the impact point
//==================
	if (!side)
	{
		VectorCopy (plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorNegate (plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (Q1_HullPointContents (hull, hull->firstclipnode, mid)
	== Q1CONTENTS_SOLID)
	{ // shouldn't really happen, but does occasionally
		if (!(frac < 10000000) && !(frac > -10000000))
		{
			trace->fraction = 0;
			VectorClear (trace->endpos);
			Con_Printf ("nan in traceline\n");
			return false;
		}
		frac -= 0.1;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy (mid, trace->endpos);
			Con_DPrintf ("backup past 0\n");
			return false;
		}
		midf = p1f + (p2f - p1f)*frac;
		for (i=0 ; i<3 ; i++)
			mid[i] = p1[i] + frac*(p2[i] - p1[i]);
	}

	trace->fraction = midf;
	VectorCopy (mid, trace->endpos);

	return false;
}
#endif

int Q1BSP_HullPointContents(hull_t *hull, vec3_t p)
{
	switch(Q1_HullPointContents(hull, hull->firstclipnode, p))
	{
	case Q1CONTENTS_EMPTY:
		return FTECONTENTS_EMPTY;
	case Q1CONTENTS_SOLID:
		return FTECONTENTS_SOLID;
	case Q1CONTENTS_WATER:
		return FTECONTENTS_WATER;
	case Q1CONTENTS_SLIME:
		return FTECONTENTS_SLIME;
	case Q1CONTENTS_LAVA:
		return FTECONTENTS_LAVA;
	case Q1CONTENTS_SKY:
		return FTECONTENTS_SKY;
	default:
		Sys_Error("Q1_PointContents: Unknown contents type");
		return FTECONTENTS_SOLID;
	}
}

unsigned int Q1BSP_PointContents(model_t *model, vec3_t axis[3], vec3_t point)
{
	if (axis)
	{
		vec3_t transformed;
		transformed[0] = DotProduct(point, axis[0]);
		transformed[1] = DotProduct(point, axis[1]);
		transformed[2] = DotProduct(point, axis[2]);
		return Q1BSP_HullPointContents(&model->hulls[0], transformed);
	}
	return Q1BSP_HullPointContents(&model->hulls[0], point);
}

qboolean Q1BSP_Trace(model_t *model, int forcehullnum, int frame, vec3_t axis[3], vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, trace_t *trace)
{
	hull_t *hull;
	vec3_t size;
	vec3_t start_l, end_l;
	vec3_t offset;

	memset (trace, 0, sizeof(trace_t));
	trace->fraction = 1;
	trace->allsolid = true;

	VectorSubtract (maxs, mins, size);
	if (forcehullnum >= 1 && forcehullnum <= MAX_MAP_HULLSM && model->hulls[forcehullnum-1].available)
		hull = &model->hulls[forcehullnum-1];
	else
	{
		if (model->hulls[5].available)
		{	//choose based on hexen2 sizes.

			if (size[0] < 3) // Point
				hull = &model->hulls[0];
			else if (size[0] <= 8 && model->hulls[4].available)
				hull = &model->hulls[4];	//Pentacles
			else if (size[0] <= 32 && size[2] <= 28)  // Half Player
				hull = &model->hulls[3];
			else if (size[0] <= 32)  // Full Player
				hull = &model->hulls[1];
			else // Golumn
				hull = &model->hulls[5];
		}
		else
		{
			if (size[0] < 3 || !model->hulls[1].available)
				hull = &model->hulls[0];
			else if (size[0] <= 32)
			{
				if (size[2] < 54 && model->hulls[3].available)
					hull = &model->hulls[3]; // 32x32x36 (half-life's crouch)
				else
					hull = &model->hulls[1];
			}
			else
				hull = &model->hulls[2];
		}
	}

// calculate an offset value to center the origin
	VectorSubtract (hull->clip_mins, mins, offset);
	if (axis)
	{
		vec3_t tmp;
		VectorSubtract(start, offset, tmp);
		start_l[0] = DotProduct(tmp, axis[0]);
		start_l[1] = DotProduct(tmp, axis[1]);
		start_l[2] = DotProduct(tmp, axis[2]);
		VectorSubtract(end, offset, tmp);
		end_l[0] = DotProduct(tmp, axis[0]);
		end_l[1] = DotProduct(tmp, axis[1]);
		end_l[2] = DotProduct(tmp, axis[2]);
		Q1BSP_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, start_l, end_l, trace);

		if (trace->fraction == 1)
		{
			VectorCopy (end, trace->endpos);
		}
		else
		{
			vec3_t iaxis[3];
			vec3_t norm;
			Matrix3_Invert_Simple((void *)axis, iaxis);
			VectorCopy(trace->plane.normal, norm);
			trace->plane.normal[0] = DotProduct(norm, iaxis[0]);
			trace->plane.normal[1] = DotProduct(norm, iaxis[1]);
			trace->plane.normal[2] = DotProduct(norm, iaxis[2]);

			/*just interpolate it, its easier than inverse matrix rotations*/
			VectorInterpolate(start, trace->fraction, end, trace->endpos);
		}
	}
	else
	{
		VectorSubtract(start, offset, start_l);
		VectorSubtract(end, offset, end_l);
		Q1BSP_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, start_l, end_l, trace);

		if (trace->fraction == 1)
		{
			VectorCopy (end, trace->endpos);
		}
		else
		{
			VectorAdd (trace->endpos, offset, trace->endpos);
		}
	}

	return trace->fraction != 1;
}

/*
Physics functions (common)

============================================================================

Rendering functions (Client only)
*/
#ifndef SERVERONLY

extern int	r_dlightframecount;

//goes through the nodes marking the surfaces near the dynamic light as lit.
void Q1BSP_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	float		l, maxdist;
	int			j, s, t;
	vec3_t		impact;

	if (node->contents < 0)
		return;

	splitplane = node->plane;
	if (splitplane->type < 3)
		dist = light->origin[splitplane->type] - splitplane->dist;
	else
		dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;

	if (dist > light->radius)
	{
		Q1BSP_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		Q1BSP_MarkLights (light, bit, node->children[1]);
		return;
	}

	maxdist = light->radius*light->radius;

// mark the polygons
	surf = currentmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		//Yeah, you can blame LordHavoc for this alternate code here.
		for (j=0 ; j<3 ; j++)
			impact[j] = light->origin[j] - surf->plane->normal[j]*dist;

		// clamp center of light to corner and check brightness
		l = DotProduct (impact, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3] - surf->texturemins[0];
		s = l+0.5;if (s < 0) s = 0;else if (s > surf->extents[0]) s = surf->extents[0];
		s = l - s;
		l = DotProduct (impact, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3] - surf->texturemins[1];
		t = l+0.5;if (t < 0) t = 0;else if (t > surf->extents[1]) t = surf->extents[1];
		t = l - t;
		// compare to minimum light
		if ((s*s+t*t+dist*dist) < maxdist)
		{
			if (surf->dlightframe != r_dlightframecount)
			{
				surf->dlightbits = bit;
				surf->dlightframe = r_dlightframecount;
			}
			else
				surf->dlightbits |= bit;
		}
	}

	Q1BSP_MarkLights (light, bit, node->children[0]);
	Q1BSP_MarkLights (light, bit, node->children[1]);
}

#define MAXFRAGMENTTRIS 256
vec3_t decalfragmentverts[MAXFRAGMENTTRIS*3];

typedef struct {
	vec3_t center;

	vec3_t normal;
//	vec3_t tangent1;
//	vec3_t tangent2;

	vec3_t planenorm[6];
	float planedist[6];
	int numplanes;

	vec_t radius;
	int numtris;

} fragmentdecal_t;

//#define SHOWCLIPS
//#define FRAGMENTASTRIANGLES	//works, but produces more fragments.

#ifdef FRAGMENTASTRIANGLES

//if the triangle is clipped away, go recursive if there are tris left.
void Fragment_ClipTriToPlane(int trinum, float *plane, float planedist, fragmentdecal_t *dec)
{
	float *point[3];
	float dotv[3];

	vec3_t impact1, impact2;
	float t;

	int i, i2, i3;
	int clippedverts = 0;

	for (i = 0; i < 3; i++)
	{
		point[i] = decalfragmentverts[trinum*3+i];
		dotv[i] = DotProduct(point[i], plane)-planedist;
		clippedverts += dotv[i] < 0;
	}

	//if they're all clipped away, scrap the tri
	switch (clippedverts)
	{
	case 0:
		return;	//plane does not clip the triangle.

	case 1:	//split into 3, disregard the clipped vert
		for (i = 0; i < 3; i++)
		{
			if (dotv[i] < 0)
			{	//This is the vertex that's getting clipped.

				if (dotv[i] > -DIST_EPSILON)
					return;	//it's only over the line by a tiny ammount.

				i2 = (i+1)%3;
				i3 = (i+2)%3;

				if (dotv[i2] < DIST_EPSILON)
					return;
				if (dotv[i3] < DIST_EPSILON)
					return;

				//work out where the two lines impact the plane
				t = (dotv[i]) / (dotv[i]-dotv[i2]);
				VectorInterpolate(point[i], t, point[i2], impact1);

				t = (dotv[i]) / (dotv[i]-dotv[i3]);
				VectorInterpolate(point[i], t, point[i3], impact2);

#ifdef SHOWCLIPS
				if (dec->numtris != MAXFRAGMENTTRIS)
				{
					VectorCopy(impact2,					decalfragmentverts[dec->numtris*3+0]);
					VectorCopy(decalfragmentverts[trinum*3+i],	decalfragmentverts[dec->numtris*3+1]);
					VectorCopy(impact1,					decalfragmentverts[dec->numtris*3+2]);
					dec->numtris++;
				}
#endif


				//shrink the tri, putting the impact into the killed vertex.
				VectorCopy(impact2, point[i]);


				if (dec->numtris == MAXFRAGMENTTRIS)
					return;	//:(

				//build the second tri
				VectorCopy(impact1,					decalfragmentverts[dec->numtris*3+0]);
				VectorCopy(decalfragmentverts[trinum*3+i2],	decalfragmentverts[dec->numtris*3+1]);
				VectorCopy(impact2,					decalfragmentverts[dec->numtris*3+2]);
				dec->numtris++;

				return;
			}
		}
		Sys_Error("Fragment_ClipTriToPlane: Clipped vertex not founc\n");
		return;	//can't handle it
	case 2:	//split into 3, disregarding both the clipped.
		for (i = 0; i < 3; i++)
		{
			if (!(dotv[i] < 0))
			{	//This is the vertex that's staying.

				if (dotv[i] < DIST_EPSILON)
					break;	//only just inside

				i2 = (i+1)%3;
				i3 = (i+2)%3;

				//work out where the two lines impact the plane
				t = (dotv[i]) / (dotv[i]-dotv[i2]);
				VectorInterpolate(point[i], t, point[i2], impact1);

				t = (dotv[i]) / (dotv[i]-dotv[i3]);
				VectorInterpolate(point[i], t, point[i3], impact2);

				//shrink the tri, putting the impact into the killed vertex.

#ifdef SHOWCLIPS
				if (dec->numtris != MAXFRAGMENTTRIS)
				{
					VectorCopy(impact1,					decalfragmentverts[dec->numtris*3+0]);
					VectorCopy(point[i2],	decalfragmentverts[dec->numtris*3+1]);
					VectorCopy(point[i3],					decalfragmentverts[dec->numtris*3+2]);
					dec->numtris++;
				}
				if (dec->numtris != MAXFRAGMENTTRIS)
				{
					VectorCopy(impact1,					decalfragmentverts[dec->numtris*3+0]);
					VectorCopy(point[i3],	decalfragmentverts[dec->numtris*3+1]);
					VectorCopy(impact2,					decalfragmentverts[dec->numtris*3+2]);
					dec->numtris++;
				}
#endif

				VectorCopy(impact1, point[i2]);
				VectorCopy(impact2, point[i3]);
				return;
			}
		}
	case 3://scrap it
		//fill the verts with the verts of the last and go recursive (due to the nature of Fragment_ClipTriangle, which doesn't actually know if we clip them away)
#ifndef SHOWCLIPS
		dec->numtris--;
		VectorCopy(decalfragmentverts[dec->numtris*3+0], decalfragmentverts[trinum*3+0]);
		VectorCopy(decalfragmentverts[dec->numtris*3+1], decalfragmentverts[trinum*3+1]);
		VectorCopy(decalfragmentverts[dec->numtris*3+2], decalfragmentverts[trinum*3+2]);
		if (trinum < dec->numtris)
			Fragment_ClipTriToPlane(trinum, plane, planedist, dec);
#endif
		return;
	}
}

void Fragment_ClipTriangle(fragmentdecal_t *dec, float *a, float *b, float *c)
{
	//emit the triangle, and clip it's fragments.
	int start, i;

	int p;

	if (dec->numtris == MAXFRAGMENTTRIS)
		return;	//:(

	start = dec->numtris;

	VectorCopy(a, decalfragmentverts[dec->numtris*3+0]);
	VectorCopy(b, decalfragmentverts[dec->numtris*3+1]);
	VectorCopy(c, decalfragmentverts[dec->numtris*3+2]);
	dec->numtris++;

	//clip all the fragments to all of the planes.
	//This will produce a quad if the source triangle was big enough.

	for (p = 0; p < 6; p++)
	{
		for (i = start; i < dec->numtris; i++)
			Fragment_ClipTriToPlane(i, dec->planenorm[p], dec->plantdist[p], dec);
	}
}

#else

#define MAXFRAGMENTVERTS 360
int Fragment_ClipPolyToPlane(float *inverts, float *outverts, int incount, float *plane, float planedist)
{
#define C 4
	float dotv[MAXFRAGMENTVERTS+1];
	char keep[MAXFRAGMENTVERTS+1];
#define KEEP_KILL 0
#define KEEP_KEEP 1
#define KEEP_BORDER 2
	int i;
	int outcount = 0;
	int clippedcount = 0;
	float d, *p1, *p2, *out;
#define FRAG_EPSILON 0.5

	for (i = 0; i < incount; i++)
	{
		dotv[i] = DotProduct((inverts+i*C), plane) - planedist;
		if (dotv[i]<-FRAG_EPSILON)
		{
			keep[i] = KEEP_KILL;
			clippedcount++;
		}
		else if (dotv[i] > FRAG_EPSILON)
			keep[i] = KEEP_KEEP;
		else
			keep[i] = KEEP_BORDER;
	}
	dotv[i] = dotv[0];
	keep[i] = keep[0];

	if (clippedcount == incount)
		return 0;	//all were clipped
	if (clippedcount == 0)
	{	//none were clipped
		for (i = 0; i < incount; i++)
			VectorCopy((inverts+i*C), (outverts+i*C));
		return incount;
	}

	for (i = 0; i < incount; i++)
	{
		p1 = inverts+i*C;
		if (keep[i] == KEEP_BORDER)
		{
			out = outverts+outcount++*C;
			VectorCopy(p1, out);
			continue;
		}
		if (keep[i] == KEEP_KEEP)
		{
			out = outverts+outcount++*C;
			VectorCopy(p1, out);
		}
		if (keep[i+1] == KEEP_BORDER || keep[i] == keep[i+1])
			continue;
		p2 = inverts+((i+1)%incount)*C;
		d = dotv[i] - dotv[i+1];
		if (d)
			d = dotv[i] / d;

		out = outverts+outcount++*C;
		VectorInterpolate(p1, d, p2, out);
	}
	return outcount;
}

void Fragment_ClipPoly(fragmentdecal_t *dec, int numverts, float *inverts)
{
	//emit the triangle, and clip it's fragments.
	int p;
	float verts[MAXFRAGMENTVERTS*C];
	float verts2[MAXFRAGMENTVERTS*C];
	float *cverts;
	int flip;

	if (numverts > MAXFRAGMENTTRIS)
		return;
	if (dec->numtris == MAXFRAGMENTTRIS)
		return;	//don't bother

	//clip to the first plane specially, so we don't have extra copys
	numverts = Fragment_ClipPolyToPlane(inverts, verts, numverts, dec->planenorm[0], dec->planedist[0]);

	//clip the triangle to the 6 planes.
	flip = 0;
	for (p = 1; p < dec->numplanes; p++)
	{
		flip^=1;
		if (flip)
			numverts = Fragment_ClipPolyToPlane(verts, verts2, numverts, dec->planenorm[p], dec->planedist[p]);
		else
			numverts = Fragment_ClipPolyToPlane(verts2, verts, numverts, dec->planenorm[p], dec->planedist[p]);

		if (numverts < 3)	//totally clipped.
			return;
	}

	if (flip)
		cverts = verts2;
	else
		cverts = verts;

	//decompose the resultant polygon into triangles.

	while(numverts>2)
	{
		if (dec->numtris == MAXFRAGMENTTRIS)
			return;

		numverts--;

		VectorCopy((cverts+C*0),			decalfragmentverts[dec->numtris*3+0]);
		VectorCopy((cverts+C*(numverts-1)),	decalfragmentverts[dec->numtris*3+1]);
		VectorCopy((cverts+C*numverts),		decalfragmentverts[dec->numtris*3+2]);
		dec->numtris++;
	}
}

#endif

//this could be inlined, but I'm lazy.
void Fragment_Mesh (fragmentdecal_t *dec, mesh_t *mesh)
{
	int i;

	vecV_t verts[3];

	/*if its a triangle fan/poly/quad then we can just submit the entire thing without generating extra fragments*/
	if (mesh->istrifan)
	{
		Fragment_ClipPoly(dec, mesh->numvertexes, mesh->xyz_array[0]);
		return;
	}

	//Fixme: optimise q3 patches (quad strips with bends between each strip)

	/*otherwise it goes in and out in weird places*/
	for (i = 0; i < mesh->numindexes; i+=3)
	{
		if (dec->numtris == MAXFRAGMENTTRIS)
			break;

		VectorCopy(mesh->xyz_array[mesh->indexes[i+0]], verts[0]);
		VectorCopy(mesh->xyz_array[mesh->indexes[i+1]], verts[1]);
		VectorCopy(mesh->xyz_array[mesh->indexes[i+2]], verts[2]);
		Fragment_ClipPoly(dec, 3, verts[0]);
	}
}

void Q1BSP_ClipDecalToNodes (fragmentdecal_t *dec, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents < 0)
		return;

	splitplane = node->plane;
	dist = DotProduct (dec->center, splitplane->normal) - splitplane->dist;

	if (dist > dec->radius)
	{
		Q1BSP_ClipDecalToNodes (dec, node->children[0]);
		return;
	}
	if (dist < -dec->radius)
	{
		Q1BSP_ClipDecalToNodes (dec, node->children[1]);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_PLANEBACK)
		{
			if (-DotProduct(surf->plane->normal, dec->normal) > -0.5)
				continue;
		}
		else
		{
			if (DotProduct(surf->plane->normal, dec->normal) > -0.5)
				continue;
		}
		Fragment_Mesh(dec, surf->mesh);
	}

	Q1BSP_ClipDecalToNodes (dec, node->children[0]);
	Q1BSP_ClipDecalToNodes (dec, node->children[1]);
}

#ifdef RTLIGHTS
extern int sh_shadowframe;
#else
static int sh_shadowframe;
#endif
#ifdef Q3BSPS
void Q3BSP_ClipDecalToNodes (fragmentdecal_t *dec, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	**msurf;
	msurface_t	*surf;
	mleaf_t		*leaf;
	int			i;

	if (node->contents != -1)
	{
		leaf = (mleaf_t *)node;
	// mark the polygons
		msurf = leaf->firstmarksurface;
		for (i=0 ; i<leaf->nummarksurfaces ; i++, msurf++)
		{
			surf = *msurf;

			//only check each surface once. it can appear in multiple leafs.
			if (surf->shadowframe == sh_shadowframe)
				continue;
			surf->shadowframe = sh_shadowframe;

			Fragment_Mesh(dec, surf->mesh);
		}
		return;
	}

	splitplane = node->plane;
	dist = DotProduct (dec->center, splitplane->normal) - splitplane->dist;

	if (dist > dec->radius)
	{
		Q3BSP_ClipDecalToNodes (dec, node->children[0]);
		return;
	}
	if (dist < -dec->radius)
	{
		Q3BSP_ClipDecalToNodes (dec, node->children[1]);
		return;
	}
	Q3BSP_ClipDecalToNodes (dec, node->children[0]);
	Q3BSP_ClipDecalToNodes (dec, node->children[1]);
}
#endif

int Q1BSP_ClipDecal(vec3_t center, vec3_t normal, vec3_t tangent1, vec3_t tangent2, float size, float **out)
{	//quad marks a full, independant quad
	int p;
	fragmentdecal_t dec;

	VectorCopy(center, dec.center);
	VectorCopy(normal, dec.normal);
	dec.radius = size/2;
	dec.numtris = 0;

	VectorCopy(tangent1,	dec.planenorm[0]);
	VectorNegate(tangent1,	dec.planenorm[1]);
	VectorCopy(tangent2,	dec.planenorm[2]);
	VectorNegate(tangent2,	dec.planenorm[3]);
	VectorCopy(dec.normal,		dec.planenorm[4]);
	VectorNegate(dec.normal,	dec.planenorm[5]);
	for (p = 0; p < 6; p++)
		dec.planedist[p] = -(dec.radius - DotProduct(dec.center, dec.planenorm[p]));
	dec.numplanes = 6;

	sh_shadowframe++;

	if (cl.worldmodel->fromgame == fg_quake)
		Q1BSP_ClipDecalToNodes(&dec, cl.worldmodel->nodes);
#ifdef Q3BSPS
	else if (cl.worldmodel->fromgame == fg_quake3)
		Q3BSP_ClipDecalToNodes(&dec, cl.worldmodel->nodes);
#endif

	*out = (float *)decalfragmentverts;
	return dec.numtris;
}

//This is spike's testing function, and is only usable by gl. :)
/*
#include "glquake.h"
void Q1BSP_TestClipDecal(void)
{
	int i;
	int numtris;
	vec3_t fwd;
	vec3_t start;
	vec3_t center, normal, tangent, tangent2;
	float *verts;

	if (cls.state != ca_active)
		return;

	VectorCopy(r_origin, start);
//	start[2]+=22;
	VectorMA(start, 10000, vpn, fwd);

	if (!TraceLineN(start, fwd, center, normal))
	{
		VectorCopy(start, center);
		normal[0] = 0;
		normal[1] = 0;
		normal[2] = 1;
	}

	CrossProduct(fwd, normal, tangent);
	VectorNormalize(tangent);

	CrossProduct(normal, tangent, tangent2);

	numtris = Q1BSP_ClipDecal(center, normal, tangent, tangent2, 128, &verts);
	PPL_RevertToKnownState();
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglDisable(GL_DEPTH_TEST);

	qglColor3f(1, 0, 0);
	qglShadeModel(GL_SMOOTH);
	qglBegin(GL_TRIANGLES);
	for (i = 0; i < numtris; i++)
	{
		qglVertex3fv(verts+i*9+0);
		qglVertex3fv(verts+i*9+3);
		qglVertex3fv(verts+i*9+6);
	}
	qglEnd();

	qglColor3f(1, 1, 1);
	qglBegin(GL_LINES);
	for (i = 0; i < numtris; i++)
	{
		qglVertex3fv(verts+i*9+0);
		qglVertex3fv(verts+i*9+3);
		qglVertex3fv(verts+i*9+3);
		qglVertex3fv(verts+i*9+6);
		qglVertex3fv(verts+i*9+6);
		qglVertex3fv(verts+i*9+0);
	}

	qglVertex3fv(center);
	VectorMA(center, 10, normal, fwd);
	qglVertex3fv(fwd);

	qglColor3f(0, 1, 0);
	qglVertex3fv(center);
	VectorMA(center, 10, tangent, fwd);
	qglVertex3fv(fwd);

	qglColor3f(0, 0, 1);
	qglVertex3fv(center);
	CrossProduct(tangent, normal, fwd);
	VectorMA(center, 10, fwd, fwd);
	qglVertex3fv(fwd);

	qglColor3f(1, 1, 1);

	qglEnd();
	qglEnable(GL_TEXTURE_2D);
	qglEnable(GL_DEPTH_TEST);
	PPL_RevertToKnownState();
}
*/

#endif
/*
Rendering functions (Client only)

==============================================================================

Server only functions
*/
#ifndef CLIENTONLY

//does the recursive work of Q1BSP_FatPVS
void SV_Q1BSP_AddToFatPVS (model_t *mod, vec3_t org, mnode_t *node, qbyte *buffer, unsigned int buffersize)
{
	int		i;
	qbyte	*pvs;
	mplane_t	*plane;
	float	d;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != Q1CONTENTS_SOLID)
			{
				pvs = Q1BSP_LeafPVS (mod, (mleaf_t *)node, NULL, 0);
				for (i=0; i<buffersize; i++)
					buffer[i] |= pvs[i];
			}
			return;
		}

		plane = node->plane;
		d = DotProduct (org, plane->normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{	// go down both
			SV_Q1BSP_AddToFatPVS (mod, org, node->children[0], buffer, buffersize);
			node = node->children[1];
		}
	}
}

/*
=============
Q1BSP_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
unsigned int Q1BSP_FatPVS (model_t *mod, vec3_t org, qbyte *pvsbuffer, unsigned int buffersize, qboolean add)
{
	unsigned int fatbytes = (mod->numleafs+31)>>3;
	if (fatbytes > buffersize)
		Sys_Error("map had too much pvs data (too many leaves)\n");;
	if (!add)
		Q_memset (pvsbuffer, 0, fatbytes);
	SV_Q1BSP_AddToFatPVS (mod, org, mod->nodes, pvsbuffer, fatbytes);
	return fatbytes;
}

#endif
qboolean Q1BSP_EdictInFatPVS(model_t *mod, struct pvscache_s *ent, qbyte *pvs)
{
	int i;

	if (ent->num_leafs == MAX_ENT_LEAFS+1)
		return true;	//it's in too many leafs for us to cope with. Just trivially accept it.

	for (i=0 ; i < ent->num_leafs ; i++)
		if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
			return true;	//we might be able to see this one.

	return false;	//none of this ents leafs were visible, so neither is the ent.
}

/*
===============
SV_FindTouchedLeafs

Links the edict to the right leafs so we can get it's potential visability.
===============
*/
void Q1BSP_RFindTouchedLeafs (model_t *wm, struct pvscache_s *ent, mnode_t *node, float *mins, float *maxs)
{
	mplane_t	*splitplane;
	mleaf_t		*leaf;
	int			sides;
	int			leafnum;

	if (node->contents == Q1CONTENTS_SOLID)
		return;

// add an efrag if the node is a leaf

	if ( node->contents < 0)
	{
		if (ent->num_leafs >= MAX_ENT_LEAFS)
		{
			ent->num_leafs = MAX_ENT_LEAFS+1;	//too many. mark it as such so we can trivially accept huge mega-big brush models.
			return;
		}

		leaf = (mleaf_t *)node;
		leafnum = leaf - wm->leafs - 1;

		ent->leafnums[ent->num_leafs] = leafnum;
		ent->num_leafs++;
		return;
	}

// NODE_MIXED

	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE(mins, maxs, splitplane);

// recurse down the contacted sides
	if (sides & 1)
		Q1BSP_RFindTouchedLeafs (wm, ent, node->children[0], mins, maxs);

	if (sides & 2)
		Q1BSP_RFindTouchedLeafs (wm, ent, node->children[1], mins, maxs);
}
void Q1BSP_FindTouchedLeafs(model_t *mod, struct pvscache_s *ent, float *mins, float *maxs)
{
	ent->num_leafs = 0;
	if (mins && maxs)
		Q1BSP_RFindTouchedLeafs (mod, ent, mod->nodes, mins, maxs);
}


/*
Server only functions

==============================================================================

PVS type stuff
*/

/*
===================
Mod_DecompressVis
===================
*/
qbyte *Q1BSP_DecompressVis (qbyte *in, model_t *model, qbyte *decompressed, unsigned int buffersize)
{
	int		c;
	qbyte	*out;
	int		row;

	row = (model->numleafs+7)>>3;
	out = decompressed;

	if (buffersize < row)
		row = buffersize;

#if 0
	memcpy (out, in, row);
#else
	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
#endif

	return decompressed;
}

static qbyte	mod_novis[MAX_MAP_LEAFS/8];

qbyte *Q1BSP_LeafPVS (model_t *model, mleaf_t *leaf, qbyte *buffer, unsigned int buffersize)
{

	static qbyte	decompressed[MAX_MAP_LEAFS/8];

	if (leaf == model->leafs)
		return mod_novis;

	if (!buffer)
	{
		buffer = decompressed;
		buffersize = sizeof(decompressed);
	}

	return Q1BSP_DecompressVis (leaf->compressed_vis, model, buffer, buffersize);
}

qbyte *Q1BSP_LeafnumPVS (model_t *model, int leafnum, qbyte *buffer, unsigned int buffersize)
{
	return Q1BSP_LeafPVS(model, model->leafs + leafnum, buffer, buffersize);
}

//returns the leaf number, which is used as a bit index into the pvs.
int Q1BSP_LeafnumForPoint (model_t *model, vec3_t p)
{
	mnode_t		*node;
	float		d;
	mplane_t	*plane;

	if (!model)
	{
		Sys_Error ("Mod_PointInLeaf: bad model");
	}
	if (!model->nodes)
		return 0;

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node - model->leafs;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return 0;	// never reached
}

mleaf_t *Q1BSP_LeafForPoint (model_t *model, vec3_t p)
{
	return model->leafs + Q1BSP_LeafnumForPoint(model, p);
}



/*
PVS type stuff

==============================================================================

Init stuff
*/

void Q1BSP_Init(void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));
}


//sets up the functions a server needs.
//fills in bspfuncs_t
void Q1BSP_SetModelFuncs(model_t *mod)
{
#ifndef CLIENTONLY
	mod->funcs.FatPVS				= Q1BSP_FatPVS;
#endif
	mod->funcs.EdictInFatPVS		= Q1BSP_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs		= Q1BSP_FindTouchedLeafs;
	mod->funcs.LightPointValues		= NULL;
	mod->funcs.StainNode			= NULL;
	mod->funcs.MarkLights			= NULL;

	mod->funcs.LeafnumForPoint		= Q1BSP_LeafnumForPoint;
	mod->funcs.LeafPVS				= Q1BSP_LeafnumPVS;
	mod->funcs.Trace				= Q1BSP_Trace;
	mod->funcs.PointContents		= Q1BSP_PointContents;
}
