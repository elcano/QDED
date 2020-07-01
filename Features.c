/* Features.c
   June 25, 2020  Tyler Folsom
   The 2005 version had been used to find stereo disparity,
   with both stereo views appearing in a single image.
   Present code removes stereo and an unnecessary addition of OpenCV.
   Code is intended for an embedded processor (such as Raspberry PI)
   Current version of code compiles on Microsoft visual Studio.

   March 27, 2005   Tyler Folsom
   This is the processing file for ImageFeatures version 4.0
   This is a repackaging of Image Features version 3.x
   The previous version was MFC in C++ providing a user interface to show results.
   The new version is intended for real-time use on board a robot.
   It is in C, does not use MFC, and does no displays.
   Display should be in a separate program, such as ImageShow.
   Further processing is done in Image Match, which typically runs on a
   separate processor.

   Copyright (c) 2005, 2020  Tyler Folsom.  All rights reserved.
   */
//#include "stdafx.h"
#include "features.h"
#include <math.h>   /* for sqrt */
#include <stdlib.h> /* for malloc */
#include <stdio.h>  /* for debug  */
//extern float GetAngle( float *dom_resp, float *steeredEven, float *steeredOdd);
extern void find_pos(  int i, struct FILTER *pKern);
extern void corner(    int i, struct FILTER *pKern );
extern void find_perp( int i, struct FILTER *pKern );
extern float find_step (float phase);
extern float find_strength (float magn, float phase);
#ifdef DEBUG
extern void WriteKernel( int diam,  float *image, char *filename);
//extern void WritePGM( int Width, int Height, PIXEL *image, char *filename, 
//					  int i, struct FILTER *pKern);
extern float StartResults(FILE *fp, int version);
extern void WriteALL( int Width, int Height, PIXEL *image, char *filename, int Magic);
extern void DrawLine( int i, PIXEL *image, int rowLength);
extern void DrawKernels(struct FILTER *pKern, char *Name);
extern float GetAngle(float *dom_resp, float *steeredEven, float *steeredOdd);
#endif

/*---------------------------------------------------------------------------*/
/* global variables */

/* how many locations are used in the grid */
extern int g_locations;
/* how many of them are above THRESHHOLD */
extern int g_significantU;
extern int g_significantL;

extern struct FEATURE  Location[MAX_LOCS];
// The 3 allows us to write color images, but only monochrome are read
extern PIXEL Image[3 * BOUNDS_RIGHT * BOUNDS_BOTTOM];
extern int ImageWidth;
extern int ImageHeight;

float corrEven[3], corrOdd[2];
/*------------------------------------------------------------------------*/
/* global to this file */
static struct FILTER even[MAX_FILTERS];
static struct FILTER  odd[MAX_FILTERS];


/*------------------------------------------------------------------------*/
void initialize()
{
	int i;
	for (i=0; i < MAX_FILTERS; i++)
	{
		even[i].diam = odd[i].diam = UNUSED;
	}
}
/*------------------------------------------------------------------------*/
/* Write one line to "readable.txt" */

void Readable( int i, FILE *fp, int Number, float *EvenResponse, float *OddResponse )
{
	char format[8] = "%.2f\t";
	char fixed[8]  = "%i\t";
	fprintf(fp, fixed,  Location[i].degrees);
//	fprintf(fp, format, Location[i].strength);
	fprintf(fp, format, Location[i].column);
	fprintf(fp, format, Location[i].row);
//	fprintf(fp, format, Location[i].stereo);
	fprintf(fp, format, Location[i].StrengthRaw);
	fprintf(fp, fixed,  Number );  /* scan line  7 */
	fprintf(fp, fixed,  Location[i].Type );  // 8
	fprintf(fp, fixed,  Location[i].diam );    // 9
//	fprintf(fp, format, Location[i].Pos);
	fprintf(fp, fixed,  Location[i].c);
	fprintf(fp, fixed,  Location[i].r);
#if (DEBUG >= 2)
	/* other stuff is not retained from feature to feature. */
	fprintf(fp, format, Location[i].corrEven[0]);
	fprintf(fp, format, Location[i].corrEven[1]);
	fprintf(fp, format, Location[i].corrEven[2]);
	fprintf(fp, format, Location[i].corrOdd[0]);
	fprintf(fp, format, Location[i].corrOdd[1]);
#endif
	fprintf(fp, fixed, i);
	fputc('\n', fp);
}
/*------------------------------------------------------------------------*/

/* This is used for correlating an image with a kernel. */

// Correlate the image at the given location with the filters.
// returned value is the squared sum of magnitudes at all orientations.
float correlate( 
		PIXEL  *pLocation,    /* the address of the upper corner of image */
		struct FILTER *pFilter,		  /* start of the filter */
		float *correlation)   /* where the results go. */
							  /* use a piece of image diam * diam pixels */
{

    int orn, c, size;
	PIXEL *r;
    float result;
	PIXEL *pPixel, *last_row;
	float magSqr = 0;
	float *pKern;

	size = pFilter->filterSize;
	pKern = pFilter->pKern;

  // Gray level byte image
	if (pFilter->sampleSpacing == 1)
	{
		/* assume that Image is stored by scan lines with no padding */
		last_row = pLocation + BOUNDS_RIGHT * size;
		for (orn = 0; orn < pFilter->orientations; orn ++)
		{
			result = (float) 0.0;
			for (r = pLocation; r < last_row; r += BOUNDS_RIGHT)
			{
				pPixel = r;
				for (c = 0; c < size; c++)
				{
					result += *pPixel++ * *pKern++;
				}
			}
			correlation[orn] = result;
			magSqr += result * result;
		}
	}
	else  /* subsample  */
	{
		/* assume that Image is stored by scan lines with no padding */
		last_row = pLocation + BOUNDS_RIGHT * pFilter->sampleSpacing * size;
		for (orn = 0; orn < pFilter->orientations; orn ++)
		{
			result = (float) 0.0;
			for (r = pLocation; r < last_row; r+=pFilter->sampleSpacing*BOUNDS_RIGHT)
			{
				pPixel = r;
				for (c = 0; c < pFilter->sampleSpacing*size; c+=pFilter->sampleSpacing)
				{
					result += pPixel[c] * *pKern++;
				}
			}
			correlation[orn] = result;
			magSqr += result * result;
		}
	}
	return magSqr;
}
/*----------------------------------------------------------------------*/
float bump( float x)
/*
  This function generates a bump function mapping the interval
  [-sqrt(LIM_SQ),sqrt(LIM_SQ)] onto [0,1].
  If called with a line such as "y=bump(x)", where x
  has been specified, it will automatically return the corresponding value.
  bump(x) is supported on the interval specified by LIM_SQ, clamped to 0
  outside it, and normalized to peak at y(x) = 1.0 when x = 0.0

*/
{
    float xx;

    xx = x * x;
    if (xx >= (float) (LIM_SQ - EPSILON))
        return ((float) 0.0);
    xx = (float) exp( (float) (4.0 * xx / (xx - LIM_SQ)));
    return (xx);
}
/*----------------------------------------------------------------------*/
// construct the kernels
void setKernel(int scale, struct FILTER *pKern)
{
    // Use positive diameters for even filters,
    // negative diameters for odd.
    /* kernel is a square of "filterSize * filterSize" */
    int i, j, k, orn;
    float poly;
    float u [MAX_COEFS], v [MAX_COEFS], x, y, z;
    float gaus, gaus1, gaus2;
    float th [MAX_COEFS];
    float incr_x, incr_y, x_min, y_min;
	float area;
    int index;
	float coef[3];
	int kernSize;
	int order;

    if (scale > 0)
    {	/* even */
        pKern->diam = scale;
        pKern->orientations = 3;    // Number of even filters
		order = 2;
        coef[0] =  (float) -502.48;      /* x*x term */
        coef[1] =  (float) 0.0;          /* x   term */
        coef[2] =  (float) 7.8287;       /* constant */    }
    else  /* odd */
    {
        pKern->diam = -scale;        // diameter of receptive field (pixels)
        pKern->orientations = 2;    // Number of odd filters
		order = 1;
        coef[0] =   (float) 72.0232;      /* x   term */
        coef[1] =   (float) 0.0;          /* constant */
    }
	/* compute sample spacing */
	pKern->sampleSpacing = 1 + (pKern->diam - 1) / BIGGEST_KERNEL;
	pKern->filterSize = 1 + (pKern->diam-1) / pKern->sampleSpacing;
	kernSize = (pKern->filterSize) * (pKern->filterSize);
	/* allocate memory */
	pKern->pKern = (float *)malloc( sizeof(float) * pKern->orientations * kernSize);

	/* Find the correction between center of kernel and center of image patch. */
	if (pKern->filterSize & 1)
		pKern->offset = 0.0;  /* size is an odd number. */
	else
		pKern->offset = 0.5;  /* size is an even number. */
    /* If kernel and image patch are not the same size, 
	   how many points of image were ignored? */
	pKern->offset -= (pKern->diam - 
		(pKern->sampleSpacing * (pKern->filterSize - 1) + 1))/2;

    for (orn = 0; orn < pKern->orientations; orn++)
    {
#ifdef VERT_STEREO
		// unsteered filters both have an orientation that is horizontal.
        th [orn] = (float) (-orn * PI_1 / pKern->orientations + PI_HALF);
#else
		// unsteered filters both have an orientation that is vertical.
        th [orn] = (float) (PI_1 - orn * PI_1 / pKern->orientations);
#endif
        u[orn] = (float) cos( th[orn]);
        v[orn] = (float) sin( th[orn]);
    }
/* if filterSize is odd, middle value is (x,y) (0,0) 
	 if it is even, filter is symmetric.  */
    incr_x = (float) (2.0 * EFF_LIM / (pKern->filterSize + 1));
    /* It is assumed that the filter values are essentially zero at
       +/-EFF_LIM; thus values are not computed there. */
    x_min = -EFF_LIM + incr_x;
    incr_y = (float) (2.0 * EFF_LIM / (pKern->filterSize + 1));
	y_min = -EFF_LIM + incr_y;
    index = 0;

// new version 5/19/03 with x in inner loop
    for (orn = 0; orn < pKern->orientations; orn++)
    {
        y = y_min;  
        for (i = 0; i < pKern->filterSize; i++)
        {
            gaus1 = bump(y);
            x = x_min;
            for (j = 0; j < pKern->filterSize; j++)
            {
                z = (float) (u[orn] * x + v[orn] * y);
                poly = (float) 0;
                for (k = 0; k < order; k++)
                    poly = z * (poly + coef[k]);
                poly += coef[k];

                gaus2 = bump(x);
                gaus = gaus1 * gaus2;
                pKern->pKern[index++] = (float) gaus * poly;
                x += incr_x;
            }
            y += incr_y;
        }
    }
	/* compute base strength */
	index = 0;
	area = 0;
	for (i = 0; i < pKern->filterSize; i++)
        for (j = 0; j < pKern->filterSize; j++, index++)
		{
			if (pKern->pKern[index] > 0)
				area += pKern->pKern[index];
		}
	area = 1 / (area);
	/* normalize filter for base strength */
	index = 0;
    for (orn = 0; orn < pKern->orientations; orn++)
		for (i = 0; i < pKern->filterSize; i++)
			for (j = 0; j < pKern->filterSize; j ++, index++)
				pKern->pKern[index] *= area;
}
/*---------------------------------------------------------------------------*/
void ShowKernel()
{
#if (DEBUG >= 3)
	DrawKernels( even, "Even" );
	DrawKernels( odd, "Odd_" );
#endif
}
/*---------------------------------------------------------------------------*/
int getFilter( int diam )
{
	int i;

	/* see if we already have the requested size. */
	for (i=0; i < MAX_FILTERS; i++)
	{
		if (even[i].diam == diam)
			return i;
	}
	/* have to make one. */
	for (i=0; i < MAX_FILTERS; i++)
	{
		if (even[i].diam == UNUSED)
			break;
	}
	/* to do: if fall thru we have run out of room */
	setKernel (diam, &even[i]);
	setKernel (-diam, &odd[i]);
	return i;
}
/*---------------------------------------------------------------------------*/
int FindFeatures()
{
	int i, filterIndex;
	int radius;
	PIXEL *pPixel;
	float evenSqr, oddSqr, totalSqr;
	int Number = 0;		/* Number, space and border are only used in DEBUG */
	float space = 0;
	int border = 0;
#ifdef DEBUG
#if (DEBUG >= 4)
	char name[25];
#endif
#if (DEBUG >= 2)
	FILE *fp;
	fopen_s(&fp, "FullInfo.txt", "w");
	space = StartResults(fp, 2);  // Write the header
#else
    space =  (float)(ROOT_3 / 4) *  (FILTER_DIAM / OVERLAP); 
#endif
    border = (FILTER_DIAM/2 - 1); // filter just hits column/row 0
	border += (int) space/2;    // right side of column or bottom of row 
	
#endif
	g_significantU = g_significantL = 0;

	for (i=0; i<g_locations; i++)
	{
		/* if this filter size does not already exist, make one */
		filterIndex = getFilter( Location[i].diam );
		radius = (Location[i].diam - 1) / 2;
		/* correlate the image and the filters */
		pPixel = &Image[ Location[i].c - radius + BOUNDS_RIGHT * (Location[i].r - radius)];

		evenSqr = correlate( pPixel, &even[filterIndex], Location[i].corrEven );
		oddSqr  = correlate( pPixel, &odd[filterIndex], Location[i].corrOdd );
		/* if response, is too small, go to next location */
		if (evenSqr + oddSqr < MIN_RESPONSE)
		{
			Location[i].Type = eNO_FEATURE;
		}
		else
		{
			corrEven[0] = Location[i].corrEven[0];
			corrEven[1] = Location[i].corrEven[1];
			corrEven[2] = Location[i].corrEven[2];
			corrOdd[0] = Location[i].corrOdd[0];
			corrOdd[1] = Location[i].corrOdd[1];
			//Location[i].StrengthRaw = GetAngle(&totalSqr,&evenSqr,&oddSqr);
			Location[i].degrees = GetAngle(&totalSqr,&evenSqr,&oddSqr) *90/PI_HALF;
            find_pos(i, &odd[filterIndex]);  /* is this correct?  TCF 4/17/18 */
			Location[i].Type = eEDGE;
#ifdef DEBUG
#ifdef VERT_STEREO
	    while (Location[i].c > border  && Number < ImageWidth)
	    {
		    Number++;
		    border += (int) space;
	    }
#else
	    while (Location[i].r > border && Number < ImageHeight)
	    {
		    Number++;
		    border += (int) space;
	    }
#endif
#endif
			corner (i, &odd[filterIndex]);
            if (Location[i].Type == eNO_FEATURE)
                find_perp (i, &odd[filterIndex]);
#if (DEBUG >= 4)
			// for debug, look at the image piece
			sprintf (name, "Debug\\Im%i_%i.PGM", Location[i].c, Location[i].r);
			WritePGM (Location[i].diam, Location[i].diam, pPixel, name, 
				      i, &odd[filterIndex]);
#endif
#if (DEBUG >= 2)
			Readable( i, fp, Number, Location[i].corrEven, Location[i].corrOdd );
#endif
		}
	}
#ifdef DEBUG
#if (DEBUG >= 2)
	fclose( fp );
#endif
	for (i=0; i<g_locations; i++)
	{
		DrawLine( i, Image, BOUNDS_RIGHT);
	}
	WriteALL( ImageWidth, ImageHeight, Image, "Lines.PGM", 5); // gray image
//	WriteALL( ImageWidth, ImageHeight, Image, "Lines.PGM", 6); // color image
#endif
	return TRUE;

}

