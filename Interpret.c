/* Interpret.c
	Once a feature has been reduced to a one-dimensional even and odd filter,
	this interprets the values to determine where an edge is.

   Copyright (c) 2005, 2020  Tyler Folsom.  All rights reserved.

*/
//#include "stdafx.h"   // for Windows
#include "features.h"
#include <math.h>   /* for sqrt */

/* discard a signal if it has rolled off by more than this */
#define MIN_ROLL_OFF ((float) 0.02)

/*---------------------------------------------------------------------------*/
/* global variables */

extern struct FEATURE  Location[MAX_LOCS];
/* how many features are above THRESHHOLD */
extern int g_significantU;
extern int g_significantL;
/* how many locations are in the top half of image */
extern int g_2nd_half;
float steer_60(float *sampled, float theta);
float steer_90(float *sampled, int theta);

/*------------------------------------------------------------------------*/
/* 
    steer_60: Given input filters sampled at 60 degrees, interpolate
    to the filter response at angle theta.

    input:  sampled - response of filters at 0, 60, 120 degrees.
            theta   - angle for desired response in radians.
    returned value: the interpolated filter response at theta.

#define G2_ORN 3
float steer_60(
    float *sampled,
    float theta)
{
    float steered;
    int i_angle;

    steered = (float) 0.0;
    for (i_angle = 0; i_angle < G2_ORN; i_angle++)
        steered += sampled[i_angle] * ((float) 1.0 + (float) 2.0 *
         (float) cos( (2.0 * theta - PI_2 * (float)i_angle / 
         (float) G2_ORN)));
    steered /= G2_ORN;
    return (steered);
} */
/*------------------------------------------------------------------------*/
/* 
    steer_90: Given input filters sampled at 90 degrees, interpolate
    to the filter response at angle theta.

    input:  sampled - response of filters at 0, 90 degrees.
            theta   - angle for desired response in radians.
    returned value: the interpolated filter response at theta.

float steer_90( 
    float *sampled,
    int theta)
{
    switch (theta)
    {
    case 0:
        return sampled[0];
    case 90:
        return sampled[1];
    case 180:
        return -sampled[0];
    case 270:
        return -sampled[0];
    default:
        return ( (float) (sampled[0] * cos( theta)
                        + sampled[1] * sin( theta)) );
    }
} */
/*------------------------------------------------------------------------*/
/* Look up a value in a table and do linear interpolation.
   Assume that values in x_table are monotonic. */

float look_up( float x, int points, float *x_table, float *y_table)
{
    int last, i, i_low, i_high;
    float denom, y;

    last = points - 1;
    if (x_table[0] < x_table[last])
    {
        if (x <= x_table[0])
            return (y_table[0]);
        if (x >= x_table[last])
            return (y_table[last]);
        i_low = 0;
        i_high = last;
        while (i_high - i_low > 1)
        {
            if ((denom = x_table[i_high] - x_table[i_low]) == 0.0)
                return (y_table [i_low]);
            i = (int) (i_low + (i_high - i_low) * (x - x_table[i_low]) / denom);
            if (i <= i_low)
                i = i_low + 1;
            if (i >= i_high)
                i = i_high - 1;
            if (x_table[i] < x)
                i_low = i;
            else
                i_high = i;
        }
    }
    else
    {
        if (x >= x_table[0])
            return (y_table[0]);
        if (x <= x_table[last])
            return (y_table[last]);
        i_low = last;
        i_high = 0;
        while (i_low - i_high > 1)
        {
            if ((denom = x_table[i_high] - x_table[i_low]) == 0.0)
                return (y_table [i_low]);
            i = (int) (i_high + (i_low - i_high) * (x_table[i_high] - x) /
             denom);
            if (i >= i_low)
                i = i_low - 1;
            if (i <= i_high)
                i = i_high + 1;
            if (x_table[i] < x)
                i_low = i;
            else
                i_high = i;
        }
    }
    y = y_table [i_low] + (y_table [i_high] - y_table [i_low]) *
     (x - x_table[i_low]) / (x_table[i_high] - x_table[i_low]);
    return (y);

}
/*------------------------------------------------------------------------*/
/*  find_strength: given the type of edge, its phase and magnitude,
    compute what the magnitude would have been if the edge had been in
    the center of the receptive field. */

float find_strength 
(   float magn, 
    float phase
)
{


/*Data for G1-H1 edge response; position -0.37 to 0
Phase */
#define EDGE_ENTRIES 39
static float edge_phase [EDGE_ENTRIES] = {0.0,
0.410874286, 0.418921308, 0.42932989,  0.440681409, 0.452713616, 0.465384309,
0.47870702,  0.492714419, 0.507448578, 0.522958036, 0.539296881, 0.556524524,
0.574705726, 0.593910734, 0.614215438, 0.635701501, 0.658456414, 0.682573444,
0.708151388, 0.73529409,  0.764109624, 0.794709041, 0.827204564, 0.861707105,
0.898322957, 0.937149556, 0.978270205, 1.021747747, 1.067617264, 1.115878073,
1.166485447, 1.219342788, 1.27429516,  1.331125276, 1.389553042, 1.44923954,
1.509795892, 1.570796788};
static float edge_mags [EDGE_ENTRIES] = {0.0,
0.000106765, 0.000401429, 0.001177729, 0.002873849, 0.006085781, 0.011525575,
0.019955297, 0.032111702, 0.048635061, 0.070011394, 0.096532849, 0.128277275,
0.165105526, 0.206673605, 0.252456201, 0.30177812,  0.353850448, 0.407808742,
0.462751049, 0.517774065, 0.572006179, 0.624636484, 0.674939173, 0.722292909,
0.766194948, 0.806269892, 0.842273028, 0.874088269, 0.90172079,  0.925284532,
0.944984881, 0.961097056, 0.973940998, 0.983853892, 0.991161859, 0.996152632,
0.999051303, 1.0};


    float roll_off;

    phase = FL_ABS(phase);
    while (phase > PI_1)
    {
        if (phase < (float) (PI_1 +  0.01))
        {
            phase = PI_1;  /* avoid round-off errors */
            break;
        }
        else
            phase -= PI_1;
    }

    if (phase > PI_HALF)
        phase = PI_1 - phase;
    roll_off = (float)look_up( phase, EDGE_ENTRIES, edge_phase, edge_mags);
    if (roll_off > MIN_ROLL_OFF)
        magn /= roll_off;
    else
        magn = (float) 0;
    if (magn > 255)
        magn = 255;
    return (magn);
}
/*------------------------------------------------------------------------*/
/*  find_step: given the phase, find the step position.
    The return value is the edge position.
 */

float find_step (float phase)
{

#define STEP_ENTRIES2  80

static float step_pos [STEP_ENTRIES2] = {
-1.0, -0.37, -0.36, -0.35, -0.34, -0.33,
-0.32, -0.31, -0.3, 
-0.29, -0.28, -0.27, -0.26, -0.25, -0.24, -0.23, 
-0.22, -0.21, -0.2, -0.19, -0.18, -0.17, -0.16, 
-0.15, -0.14, -0.13, -0.12, -0.11, -0.1, -0.09, 
-0.08, -0.07, -0.06, -0.05, -0.04, -0.03, -0.02, 
-0.01, 0.0, 0.01, 0.02, 0.03, 0.04, 0.05,
0.06, 0.07, 0.08, 0.09, 0.1, 0.11, 0.12, 
0.13, 0.14, 0.15, 0.16, 0.17, 0.18, 0.19, 
0.2, 0.21, 0.22, 0.23, 0.24, 0.25, 0.26, 
0.27, 0.28, 0.29, 0.3, 0.31, 0.32, 
0.33, 0.34, 0.35, 0.36, 0.37, 0.38, 0.39, 0.4, 1.0};

/* Step phase: 0 to pi matches -0.31 to 0.31 */

static float step_phase2 [STEP_ENTRIES2] = { 0.0,
0.410874286, 0.418921308, 0.42932989,  0.440681409, 0.452713616, 0.465384309, 0.47870702,
0.492714419, 0.507448578, 0.522958036, 0.539296881, 0.556524524, 0.574705726, 0.593910734,
0.614215438, 0.635701501, 0.658456414, 0.682573444, 0.708151388, 0.73529409,  0.764109624,
0.794709041, 0.827204564, 0.861707105, 0.898322957, 0.937149556, 0.978270205, 1.021747747,
1.067617264, 1.115878073, 1.166485447, 1.219342788, 1.27429516,  1.331125276, 1.389553042,
1.44923954,  1.509795892, 1.570796788, 1.631797683, 1.692354033, 1.752040527, 1.810468289,
1.86729840,  1.922250767, 1.975108104, 2.025715477, 2.073976286, 2.119845808, 2.163323357,
2.20444402,  2.243270639, 2.279886519, 2.314389096, 2.346884667, 2.377484146, 2.40629976,
2.433442563, 2.459020637, 2.483137835, 2.505892969, 2.527379322, 2.547684419, 2.566889967,
2.585071931, 2.602300683, 2.618641203, 2.634153298, 2.648891831, 2.662906939, 2.676244282,
2.688945371, 2.701048208, 2.712588774, 2.723604716, 2.734144316, 2.744286265, 2.75417223,
2.764009427, 3.1416};


    float pos;
    float abs_phase;

    abs_phase = FL_ABS(phase);
    pos = (float) look_up (abs_phase, STEP_ENTRIES2, step_phase2, step_pos);
    
    if (phase < 0.0)
		pos = -pos;
    return (pos);
}
/*------------------------------------------------------------------------*/
void find_pos( int i, struct FILTER *pKern )
{

/*  find_pos: given a convolution of an image with even G2 and odd H2
    filters at two different scales, find the edge position and
    edge strength.  
    The position is in pixels from the center of the receptive field. 
    The results are placed in a structure. */

    float phase;
    float magn, Pos;

	Location[i].Type = eEDGE;
	/* use unsteered position for stereo. */
	phase = (float) atan2 (Location[i].corrOdd[0], Location[i].corrEven[0]);
	magn = (float) sqrt(Location[i].corrEven[0] * Location[i].corrEven[0] +
		Location[i].corrOdd[0] * Location[i].corrOdd[0]);
	if (FL_ABS(phase) < 0.43  || FL_ABS(phase) > 2.7 || magn < MIN_RESPONSE)
	{
		//This is where all of the no features are
		Location[i].Type = eNO_FEATURE;
		//Location[i].StrengthRaw = find_strength( magn, phase);
		return;
	}
    i < g_2nd_half? g_significantU++: g_significantL++;

	//
	//Location[i].degrees = 90;
	Location[i].StrengthRaw = find_strength( magn, phase);
	if (Location[i].corrOdd[0] < 0)
    {
		Location[i].StrengthRaw = -Location[i].StrengthRaw;
    	//Location[i].degrees = 270;
    }
	Pos = (Location[i].diam+1)/(2*EFF_LIM) * find_step(phase) / pKern->sampleSpacing;
#ifdef VERT_STEREO
	/* unsteered filters both have an orientation that is horizontal. */
	Location[i].column = (Location[i].c + pKern->offset);
	Location[i].row =    (Location[i].r + pKern->offset + Pos);
#else  // Left-right stereo
	/* unsteered filters both have an orientation that is vertical. */
	Location[i].column = (Location[i].c + pKern->offset + Pos);
	Location[i].row =    (Location[i].r + pKern->offset);
#endif

}
/*------------------------------------------------------------------------*/
void corner( int i, struct FILTER *pKern )
{

/*  corner: Like find_pos, but angle is at right angles to principle feature. */

    float steeredEven;
    float steeredOdd;
    float magn;
    float phase;
	float Pos, theta;

	if (Location[i].Type == eNO_FEATURE)
		return;

    theta = PI_HALF;
	/* first, steer to this angle */

	// Check whether we want this angle or + 180.
	// The dark side of the image should be to our left.
	steeredOdd = steer_90 (Location[i].corrOdd, 90);

	steeredEven = steer_60 (Location[i].corrEven, theta);
    magn = (float) sqrt(steeredOdd * steeredOdd + steeredEven * steeredEven);

	if (magn < FL_ABS(Location[i].StrengthRaw) * CORNER_THRESH)
		return;
	
	phase = (float) atan2 (steeredOdd, steeredEven);

	if (FL_ABS(phase) < 0.43  || FL_ABS(phase) > 2.7)
	{
		return;
	}
    Pos = (Location[i].diam+1)/(2*EFF_LIM) * find_step (phase) / pKern->sampleSpacing;
#ifdef VERT_STEREO
	/* unsteered filters both have an orientation that is horizontal. */
	Location[i].column = (Location[i].c + pKern->offset + Pos);
#else  // Left-right stereo
	/* unsteered filters both have an orientation that is vertical. */
	Location[i].row =    (Location[i].r + pKern->offset + Pos);
#endif
	Location[i].Type = eCORNER;
}
/*------------------------------------------------------------------------*/
void find_perp( int i, struct FILTER *pKern )
{

/*  find_perp: Like find_pos, but angle is at right angles to stereo. */

    float steeredEven;
    float steeredOdd;
    float magn;
    float phase;
	float Pos;
    float theta;

   	theta = PI_HALF;
	/* first, steer to this angle */

	// Check whether we want this angle or + 180.
	// The dark side of the image should be to our left.
	steeredOdd = steer_90 (Location[i].corrOdd, 90);

	steeredEven = steer_60 (Location[i].corrEven, theta);
    magn = (float) sqrt(steeredOdd * steeredOdd + steeredEven * steeredEven);

	/* use unsteered position for stereo. */
	phase = (float) atan2 (steeredOdd, steeredEven);
	if (FL_ABS(phase) < 0.43  || FL_ABS(phase) > 2.7 || magn < MIN_RESPONSE)
	{
		return;
	}

	Location[i].degrees = 0;
	Location[i].StrengthRaw = find_strength( magn, phase);
	if (Location[i].corrOdd[1] < 0)
    {
		Location[i].StrengthRaw = -Location[i].StrengthRaw;
    	Location[i].degrees = 180;
    }
	Pos = (Location[i].diam+1)/(2*EFF_LIM) * find_step(phase) / pKern->sampleSpacing;
#ifdef VERT_STEREO
	/* unsteered filters both have an orientation that is horizontal. */
	Location[i].column = (Location[i].c + pKern->offset + Pos);
	Location[i].row =    (Location[i].r + pKern->offset);
#else  // Left-right stereo
	/* unsteered filters both have an orientation that is vertical. */
	Location[i].column = (Location[i].c + pKern->offset);
	Location[i].row =    (Location[i].r + pKern->offset + Pos);
#endif
	Location[i].Type = eEDGE_NO_STEREO;
    i < g_2nd_half? g_significantU++: g_significantL++;
}

