/* basetypes.h
 *
 * $Header: /usr/local/cvsroot/Level5/Sleipnir/match/basetypes.h,v 1.9 2005/05/05 18:24:32 jalbers Exp $
 *
 * Common constants and types used by all components.
 */

#ifndef MATCH_BASETYPES_H
#define MATCH_BASETYPES_H

/* return codes */
#define OK       0


// The number of nearest neighbors
#define NEIGHBORS 6
typedef short int BOOL;


struct flags
{
    unsigned int Closed: 1;    /* TRUE if this is a closed line */
	unsigned int NoMatch: 1;   /* TRUE if whole curve does not match at any point */
    unsigned int OffLeft:1;      /* curve exits image on left side. */
    unsigned int OffBottom:1;    /* curve exits image on bottom side. */
    unsigned int OffRight:1;     /* curve exits image on right side. */
    unsigned int OffTop:1;       /* curve exits image on top side. */
};
union alias
{
    unsigned int fields;
    struct flags f;
};

struct Polyline
{
    int Vertices;
    union alias u;
    int first;  /* Location[first] is first point */
    int last;   /* Location[last] is last point */
	/* polyline may be a line or a collection of lines. 
	   The next items are most relevant to a line. */
/*	int horizontal; // TRUE, FALSE or UNKNOWN. Whether to use X or Y as the abscissa.
	float slope;   // X/Y or Y/X depending on horizontal 
	float intercept; // in pixels; 2D 
	float std_dev;	*/
};
struct LimitPoints
{
	int goal1;	//  preferred matching curve on L1
	int goal2;	//  second choice on L2
	int first1;	//  first point on U matching goal1
	int first2;	//  first point on U matching goal2
	int last1;	//  last point on U matching goal1
	int last2;	//  last point on U matching goal2
	BOOL first_12;	//  TRUE if see goal1 before goal2 from front
	BOOL last_12;	//  TRUE if see goal1 before goal2 from back
	BOOL overlap;   //  TRUE if matches of L1 and L2 overlap on U.
};
#endif
