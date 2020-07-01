/* features.h: Items used by ImageFeatures files.

   Copyright (c) 2005  Tyler Folsom.  All rights reserved.
*/

#ifndef FEATURES_H
#define FEATURES_H

/*  For fastest execution, undefine DEBUG.
	Otherwise it gives the level of debug.
	1:  Just show image with detected lines (detected.PGM)
		and shorter readable file (readable.txt)
	2:  Show a picture and write Fullinfo.txt with filter information.
	3:  Make images of kernels 
	4:  Make images of patches with lines.
		WARNING: DEBUG 4 is intended for use on small images. If you try it on
		a 1280 x 1022 image, you will get thousands of files!
*/
#define DEBUG 2
/* Define VERT_STEREO for upper and lower images;
   undefine it for left-right stereo 
	But VERT_STEREO is not fully implemented; you are better off writing a left-right
	image pair on its side as an up-down pair and proceding.  
	Do not undefine VERT_STEREO  TCF 8/23/05
*/
#define VERT_STEREO 1
/* maximum size of image, pixels */
// iPhone 2592
#define BOUNDS_RIGHT  1600
 // iPhone 1936
#define BOUNDS_BOTTOM 880
/* maximum number of locations */
#define MAX_LOCS (BOUNDS_RIGHT*BOUNDS_BOTTOM/FILTER_DIAM)

#define POLYLINE_SIZE (MAX_LOCS/2)
/* chunk size for reading and writing PGM images. */
#define LINE_BUFFER 256

typedef unsigned char PIXEL;

struct FEATURE
{
	int c;  /* column */
	int r;  /* row */
	int diam;
	int degrees;   /* 90, 270, 0 or 180 */
	float column;    /* subpixel location of feature */
	float row;
	float StrengthRaw; /* also scaled by 255 */
	int Type;
    int NbrUp;          /* indices to the six neighboring cells */
    int NbrUpLeft;
    int NbrDownLeft;
    int NbrDown;
    int NbrDownRight;
    int NbrUpRight;
    int Ahead;          /* index to forward neighbor */
    int Back;           /* index to reverse neighbor */
    int Number;      /* polyline number that this point belongs to */
    float Score;   
    float corrEven[3];  /* numbers resulting from correlating with 0, 60 and 120 degree filters */
    float corrOdd[2];   /* numbers from correlating with odd filters at 0 an 90 degrees */
};
struct FILTER
{
	float *pKern;
	int diam;		/* FILTER_DIAM or half as big or twice as big */
	int sampleSpacing;  /* 1 if we do every sample or bigger if we skip */
	int filterSize;     /* actual size of the filter, using sampleSpacing  */
	int orientations;  /* 2 or 3 */
	float offset;      /* variation to center of (c,r) based on filterSize. */
};
// The detected feature
enum feature_type
{
    eNOT_AVAILABLE,  // indicates no data (e.g. no answer)
    eNO_FEATURE,    // there is nothing in the circle
    eFAINT_EDGE,    // edge below threshold; only used to join polyline.
    eFAINT_PERP,
    eFAINT_CORNER,
    eEDGE,    /* Edge: A dark to light step at 90 or 270. */
    eEDGE_NO_STEREO,  // An edge at 0 or 180,
    eCORNER,  /* Corner: A corner or point of high curvature. */
	eFEATURE_TYPE_SIZE
};

/* TI 6000 chips used on Sleipnir */
#define DSP 0
/* Desktop test */
#define PC  1
#define LINUX 2
/* Small robot from Seattle Robotics Society */
#define SRS 3
#define PLATFORM PC
#define FL_ABS(x) ((x)>0?(x):-(x))

#if (PLATFORM == DSP)
#define sin(x)     sinf(x)
#define cos(x)     cosf(x)
#define atan2(x,y) atan2f(x,y)
#define sqrt(x)    sqrtf(x)
#define exp(x)     expf(x)
#elif (PLATFORM == LINUX)
//#define sin(x)     ((float)sin((double)(x)))
//#define cos(x)     ((float)cos((double)(x)))
//#define atan2(x,y) ((float)atan2((double)(x),(double)(y)))
//#define sqrt(x)    ((float)sqrt((double)(x)))
//#define exp(x)     ((float)exp((double)(x)))
#elif (PLATFORM == PC)
#ifdef _MSC_VER
// PRAGMA to supress warning C4244 and 4305 float/double conversion
#pragma warning( disable : 4244 4305)
#endif
#endif

#define HEAD1 "Degree\tColumn\tRow\tStrRaw\tline\tType\tdiam\tc\tr\t"
#define HEAD2 "EConv0\tEConv1\tEConv2\tOConv0\tOConv1"

#define PI_1    ((float)3.141592654)
#define PI_2    ((float)6.283185307)
#define PI_HALF (PI_1/2)
#define PI8TH   (PI_1/8)
#define ROOT_3  ((float)1.732050808)
#define FLHALF  ((float) 0.5)
#define FL4TH   ((float) 0.25)
#define FL2     ((float) 2)
#define FL3     ((float) 3)
#define FL4     ((float) 4)
#define EPSILON ((float)1.0e-6)
/* specifies range over which bump function is non-zero */
#define LIM_SQ  ((float) 0.203636)
/* specifies effective range for even and odd filters */
#define EFF_LIM ((float) 0.35)

#define MAX_COEFS  3
/* How many different sizes we can use for filtering */
#define MAX_FILTERS     3
#define UNUSED     -1000000
#define INC_UNUSED -1001000
#define TRUE       1
#define FALSE      0
/*-----------------------------------------------------------*/
/* Parameters that could come from a file or user interface */
/* These can control trade-off of accuracy vs. speed. */

/* Higher values of threshold will find more features and give us more to process. */
/* Minimum filter response we will consider (was 37) */
/* Ignore features with strength less than this */
#define MIN_RESPONSE  20
/* A corner is characterized by a step in the dominant orientation and
   an edge perpendicular to this.  Typically, the perpendicular strength is half as
   strong as the main step.  Classify a feature as a corner if the ratio of the
   magnitudes exceeds this threshold.
*/
#define CORNER_THRESH ((float)(0.15))


/* The size of the basic patch of image.
   Smaller diameters increase the accuracy.  
   There is a limit to how small you can get which is not yet established.  Maybe 5 ? 
   The intent is to use filters half this diameter in more important regions and
   twice this diameter in unimportant regions.  Varying size is not yet implemented. */
/* Initial diameter of image patch, in pixels. */
#define FILTER_DIAM    8

/* The size of the biggest Gabor function used for correlation with image patches.
   FILTER_DIAM can be bigger or less than BIGGEST_KERNEL
   When filter diameter is less than or equal to this value, use every point of the image patch.
   When it is bigger, decimate the image.  */
/* pKern->sampleSpacing = 1 + (pKern->diam-1) / BIGGEST_KERNEL ; integer division
   Should have SampleSpacing * (BIGGEST_KERNEL-1) = FILTER_DIAM-1  
            or  FILTER_DIAM <= BIGGEST_KERNEL */
#define BIGGEST_KERNEL   50

/* A hexagonal grid is normally used, though it could be square.
   If you draw a line from the center of one receptive field center to the center of its
   nearest neighbor, OVERLAP determines how the circles intersect.
   Values should be between 1 and 3.
   1.0 means that the line between centers is equal to the diameter, so there are holes in 
   the coverage.  3.0 is the highest overlap. */
#define OVERLAP        ((float) 1.3)



#endif // FEATURES_H
