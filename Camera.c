/* Camera.c
   March 27, 2005   Tyler Folsom
   This is the main file for ImageFeatures version 4.0
   This is a repackaging of Image Features version 3.x
   The previous version was MFC in C++ providing a user interface to show results.
   The new version is intended for real-time use on board a robot.
   It is in C, does not use MFC, and does no displays.
   Display should be in a separate program, such as ImageShow.
   This program processes an image to a readable.txt file.
   Further processing is done in Image Match, which typically runs on a
   separate processor.

   Copyright (c) 2005  Tyler Folsom.  All rights reserved.
   */
//#include "stdafx.h"   // for Windows
#include "features.h"
#include <math.h>   /* for sqrt */
#include <stdio.h>  /* for FILE, fopen, fputc */
#include <ctype.h>  /* for isspace */

extern int  FindFeatures();
extern void Readable( int i, FILE *fp, int Number, float evenResp, float oddResp );
//extern void do_savebmp(char *name);
//extern void do_video1(void);
//extern int  CorrectImage();


/*---------------------------------------------------------------------------*/
/* global variables */

/* how many locations are used in the grid */
int g_locations;
/* how many of them are above TRHESHHOLD */
int g_significantU;
int g_significantL;
/* how many locations are in the top half of image */
int g_2nd_half;
struct FEATURE  Location[MAX_LOCS];

extern int ImageWidth;
extern int ImageHeight;

/*---------------------------------------------------------------------------*/
/* Look for an updated grid based on last processing */
void GetNewGrid( int time, int desired_time)
{
}
/*---------------------------------------------------------------------------*/
void GetGrid()
{
	int row, col, col_space, row_space;
	int c, r, format;
	int lastWidth, lastHeight;
    int start, half;
	float centerToCenter;
    int items;


    row = FILTER_DIAM/2; // filter just hits row 0
    col = FILTER_DIAM/2; // filter just hits column 0
    centerToCenter = FILTER_DIAM / OVERLAP;
    row_space = (int) centerToCenter;  
    col_space = (int) (ROOT_3/2 *  centerToCenter);
    if (col_space < 2)
        col_space = 2;
    if (row_space < 2)
        row_space = 2;
	g_locations = 0;
	lastWidth = ImageWidth - col;
	lastHeight = ImageHeight - row;
#ifdef VERT_STEREO
    /* Do top and bottom of image separately */
	lastHeight = ImageHeight/2 - row;
    start = 0;
    for (half = 0; half < 2; half++)
    {
        items = (lastHeight - start - row) / row_space;
	    for (c = col; c < lastWidth; c += col_space/2)
	    {
		    for (format = 0; format < 2; format++) // overlapped grids
		    {
			    for (r = start + row; r < lastHeight; r += row_space)
			    {
				    Location[g_locations].c = c;
				    Location[g_locations].r = r;
				    Location[g_locations].diam = FILTER_DIAM;
                    if (r == start + row)
                        Location[g_locations].NbrUp = UNUSED;
                    else
                    {
                        Location[g_locations].NbrUp = g_locations - 1;
                        Location[g_locations-1].NbrDown = g_locations;
                    }
                    if (c == col)
                    {
                        Location[g_locations].NbrUpLeft = UNUSED;
                        Location[g_locations].NbrDownLeft = UNUSED;
                    }
                    else
                    {
                        if (Location[g_locations - items].r < 
                            Location[g_locations].r)
                        {
                            Location[g_locations].NbrDownLeft = UNUSED;
                        }
                        else
                        {
                            Location[g_locations].NbrDownLeft = g_locations - items;
                            Location[g_locations - items].NbrUpRight = g_locations;
                        }
                        if (Location[g_locations - items - 1].r > 
                            Location[g_locations].r)
                            Location[g_locations].NbrUpLeft = UNUSED;
                        else
                        {
                            Location[g_locations].NbrUpLeft = g_locations - items - 1;
                            Location[g_locations - items - 1].NbrDownRight = g_locations;
                        }

                    }
                    Location[g_locations].NbrDown = UNUSED;
                    Location[g_locations].NbrDownRight = UNUSED;
                    Location[g_locations].NbrUpRight = UNUSED;
                    Location[g_locations].Ahead = UNUSED;
                    Location[g_locations].Back = UNUSED;
				    if (g_locations++ >= MAX_LOCS)
					    return;
			    }	// loop on rows
			    if (format == 0)
			    {
				    c   += col_space/2;
				    if (c >= lastWidth)
					    break;
                    items = (lastHeight - start - row + row_space - 1) / row_space - 1;
				    row += row_space/2;
			    }
			    else
			    {
                    items = (lastHeight - start - row + row_space - 1) / row_space;
				    row -= row_space/2;
			    }
            }	// loop on overlapped grids
        }
        if (half == 0)
        {
            g_2nd_half = g_locations;
            start = ImageHeight/2;
        	lastHeight = ImageHeight - row;
        }
    }
#else
	lastHeight = ImageHeight - row;
	for (r = row; r < laastHeight; r += col_space/2)
	{
		for (format = 0; format < 2; format++) // overlapped grids
		{
			for (c = col; c < lastWidth; c += row_space)
			{
				Location[g_locations].c = c;
				Location[g_locations].r = r;
				Location[g_locations].diam = FILTER_DIAM;
				if (g_locations++ >= MAX_LOCS)
					return;
			}	// loop on columns
			if (format == 0)
			{
				r   += col_space/2;
				if (r >= lastHeight)
					break;
				col += row_space/2;
			}
			else
			{
				col -= row_space/2;
			}
        }	// loop on overlapped grids
    }	
#endif
}
/*------------------------------------------------------------------------*/
/* write one line to "results.txt" in a binary format. */
void Binary( int i, FILE *fp, int Number )
{
	return;
}
/*------------------------------------------------------------------------*/
/* Routine in C to replace WriteFeatureList
   Tyler Folsom   Dec. 13, 2004   */
void Results(char *name, int version, int upper)
{
	FILE *fp;
	int i, Number;
	float space;
	int border;
    int start, end;

    space =  (float)(ROOT_3 / 4) *  (FILTER_DIAM / OVERLAP); 
    if (space < 2)
        space = 2;

    border = (FILTER_DIAM/2 - 1); // filter just hits column/row 0
	border += (int) space/2;    // right side of column or bottom of row 

	Number = 0;

	fopen_s(&fp, name, "w");
	if (upper)
    {
        fprintf( fp, "%i", g_significantU );
        start = 0;
        end = g_2nd_half;
    }
    else
    {
        fprintf( fp, "%i", g_significantL );
        start = g_2nd_half;
        end = g_locations;
    }
	fputc('\n', fp);
	if (version == 2)
	{
		fprintf(fp, HEAD1);
#if(DEBUG>=2)
		fprintf(fp, HEAD2);
#endif
		fputc('\n', fp);
	}
	for (i = start; i < end; i++)
	{
		/* Location[] must be ordered by scan lines. */
		/* Scan lines are vertical if we are doing top and bottom stereo;
		   they are horizontal if we do left - right stereo. */
		/* See GetGrid() above for scan order. */
#ifdef VERT_STEREO
		while (Location[i].c > border)
#else			
		while (Location[i].r > border)
#endif
		{
			Number++;
			border += (int) space;
		}
 		if (FL_ABS(Location[i].StrengthRaw) >= MIN_RESPONSE && Location[i].Type >= eEDGE)
		{
			switch(version)
			{
			case 1:
			case 2:
				Readable(i, fp, Number, 0, 0);
				break;
			case 3:
				Binary(i, fp, Number);
				break;
			}
		}
	}
	fclose (fp);
}
/*---------------------------------------------------------------------------*/

int frame(int Live, int i)
{
#ifdef LIVE_VIDEO
	char name[25];

	while (Live)
	{
		/* capture an image from the camera */
//		do_video1();
		ImageHeight = CorrectImage();
		sprintf_s( name, 24, "Can%i.bmp", i);
		/* write the image from the camera */
//		do_savebmp(name);
		printf("Hit any key for next image.");
		getchar();
	}
	/* Process it */
	FindFeatures();
	/* Write results */
	/* May want to take the filename from the command line instead of hard-coding it. */
#ifdef DEBUG
	Results("Ureadable.txt", 2, TRUE);
	Results("Lreadable.txt", 2, FALSE);
#endif  // DEBUG
	/* Look for an updated grid based on last processing */
//	GetNewGrid( time, desired_time);
#endif // LIVE_VIDEO
	return 0;
}
/*---------------------------------------------------------------------------*/
// for robot control, use main.c and eliminate this routine.
// for stereo image processing only, use this routine and do not include main.c
#include <time.h>   /* for clock */
//#include "Version.h"  /* requires stdio.h */

// extern void initialize();
// extern void image_match();   // Image_match.c has a lot of code for stereo correspondence.
extern int  ReadPGM(char *image_name);  // In io.c; reads an image in .pgm format.
extern void GetGrid();
extern int  frame(int Live, int i);
extern int  CorrectImage();  // In io.c: matches a steereo pair in the upper and lower halves of the image.

#if (DEBUG >= 3)
extern void ShowKernel();
#endif
/*---------------------------------------------------------------------------*/
/* global variables */

extern int ImageWidth;
extern int ImageHeight;

/*---------------------------------------------------------------------------*/
// rename this to main() if not using module top.c; otherwise call it main2()
int main(int argc, char *argv[])
{
	// argv: prog_name  image_name.pgm
	char *image_name;
	clock_t time;  /* time in CLOCKS_PER_SEC to do processing */
	clock_t startTime;
	int Live = FALSE;
	int frame_number = 0;
	float elapsed;

	image_name = argc < 2? "Images\\Carla5.pgm" : argv[1];
	//initialize();
	ImageWidth = BOUNDS_RIGHT;
	ImageHeight = BOUNDS_BOTTOM;
	if (!Live)
	{
		if (0 != ReadPGM(image_name))
			return 1;  // Simulation mode, but no input file.
	}

	/* get the initial sampling grid */
	GetGrid();

	time = startTime = clock();
	/* initial delay */
	//printf("waiting\n");
	while ((time - startTime) < DELAY * CLOCKS_PER_SEC)
	{
		time = clock();
	}
	//printf("moving\n");

//	frameTime = clock();
	frame(Live, ++frame_number);    // Image Features
//	image_match();			// Combine edge segments into polylines and find stereo correspondences.
#if (DEBUG >= 3)
    ShowKernel();
#endif
	time = clock();
	elapsed = (float) (time - startTime) / CLOCKS_PER_SEC;
	printf("Elapsed time: %.3f seconds/n", elapsed);
	return 0;  /* simulation */
}


