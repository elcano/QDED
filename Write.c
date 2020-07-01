/* Write.c
   This file contains stuff only used for debugging.
   Tyler Folsom  April 17, 2005

   Copyright (c) 2005, 2020  Tyler Folsom.  All rights reserved.
*/
#define BRIEF
#include "features.h"
#include "basetypes.h"
#include <math.h>   /* for sqrt */
#include <stdlib.h> /* for malloc */
#include <stdio.h>  /* for fprintf  */

#define ABS(x)  ((x)>=0?(x):-(x))

// NO_OP is a place holder for the IN/DE-CREMENT_X/Y functions.
void NO_OP(void) {;};

/*---------------------------------------------------------------------------*/
/* global variables */
/* how many locations are used in the grid */
extern int g_locations;
extern struct FEATURE  Location[MAX_LOCS];
extern int ImageWidth;
extern int ImageHeight;
// size is 3 * BOUNDS_RIGHT * BOUNDS_BOTTOM
extern PIXEL Image[];

/*---------------------------------------------------------------------------*/
// global to this module
static int g_column, g_row;
static int g_rowlength;
static int g_max_row, g_max_col;
static int g_min_row, g_min_col;
static int g_red, g_green, g_blue;
static PIXEL *g_buffer;

/*---------------------------------------------------------------------------*/
float StartResults(FILE *fp, int version)
{
	float space;
    space =  (float)(ROOT_3 / 4) *  (FILTER_DIAM / OVERLAP); 
    if (space < 2)
        space = 2;

	fprintf( fp, "%i\n", g_locations );
	if (version >= 2)
	{
		fprintf(fp, HEAD1);
#if(DEBUG>=2)
		fprintf(fp, HEAD2);
#endif
		fputc('\n', fp);
	}
	return space;
}
/*---------------------------------------------------------------------------*/
/* Write a PGM image of a kernel for debugging */
void WriteKernel( int diam,  float *image, char *filename)
{
	FILE *fp;
	PIXEL line[100];
	errno_t error;
	float *kernel;
	int i, j;
	float minimum, maximum, scale;
	minimum = maximum = 0;
	/* find the min and max values and use them for scaling. */
	kernel = image;
 	for(i=0; i < diam; i++)
	{
 		for(j=0; j < diam; j++)
		{
			if (*kernel > maximum)
				maximum = *kernel;
			if (*kernel < minimum)
				minimum = *kernel;
			kernel++;
		}
	}
	maximum = -minimum > maximum ? -minimum : maximum;
	scale =  maximum < EPSILON? 1 : 127 / maximum;
	kernel = image;
	error = fopen_s(&fp, filename, "w");
	if (error || fp == NULL)
	{
		printf("WriteKernel Error %i", error);
		return;
	}
	fprintf_s(fp, "P5 %i %i 255 ", diam, diam);
 	for(i=0; i < diam; i++)
	{
		for (j=0; j<diam; j++)
			line[j] = (PIXEL) (128 + *kernel++ * scale);
		// copy filter to line
		fwrite(line, sizeof(PIXEL), diam, fp);
	}
	fclose(fp);
}
/*---------------------------------------------------------------------------*/
void DrawKernels(struct FILTER *pKern, char *Name)
{
	float *kernel;
    int orn;
	char name[20];

	kernel = pKern->pKern;
	for (orn = 0; orn < pKern->orientations; orn++)
	{
		sprintf_s( name, 13, "%4s%i_%i.PGM", Name, pKern->diam, orn);
		WriteKernel (pKern->filterSize, kernel, name);
		kernel += (pKern->filterSize) * (pKern->filterSize);
	}
}
/*---------------------------------------------------------------------------*/

int ROUND(float x)
{ return (int)(x >= 0? x + (float)0.5 : x - (float)0.5); }

void GOTO_POINT(int x, int y)
{
	g_column = x;
	g_row = y;
}


void INCREMENT_X(void)
{ g_column++; }

void INCREMENT_Y(void)
{ g_row++; }

void DECREMENT_X(void)
{ g_column--; }

void DECREMENT_Y(void)
{ g_row--; }

void WRITE_PIXEL(void)
{ 
	static PIXEL last_wrote;
	if (last_wrote == 255)
	{
		last_wrote = g_buffer[g_column+g_row*g_rowlength] = 0;
	}
	else
	{
		last_wrote = g_buffer[g_column+g_row*g_rowlength] = 255;
	}
}
void SET_COLOR( int red, int green, int blue)
{
	g_red = red;
	g_green = green;
	g_blue = blue;
}
void WRITE_COLOR(void)
{ 
	g_buffer[  3*(g_column+g_row*g_rowlength)] = g_red;
	g_buffer[1+3*(g_column+g_row*g_rowlength)] = g_green;
	g_buffer[2+3*(g_column+g_row*g_rowlength)] = g_blue;
}


/*---------------------------------------------------------------------------*/
void LINE_BRESENHAM_FLOAT(float xa, float ya, float angle, BOOL color)
{
	int col_a, row_a;
	register float D, deltaDplus, deltaDminus, deltaDequal;
	float deltaX, deltaY;
	void (*bumpIndex)(void);
	void (*bumpLess)(void);
	void (*bumpMore)(void);
	void (*bumpSame)(void);
	register short int i;
	float radians;
	int pass;

	col_a = ROUND(xa);
	row_a = ROUND(ya);
	GOTO_POINT( col_a, row_a);
	if(g_column >= g_min_col && g_column < g_max_col &&
		g_row >= g_min_row && g_row < g_max_row)
	    g_buffer[g_column+g_row*g_rowlength] = 100;
	if (angle < 0)
		angle += 360;
	if (angle >= 360)
		angle -= 360;

	radians = angle * PI_1 / 180;
	deltaY = FILTER_DIAM * (float) cos(radians);
	deltaX = FILTER_DIAM * (float) sin(radians);

	for (pass = 0; pass < 2; pass++)
	{
		if ((angle >= 45 && angle < 135) || (angle >= 225 && angle < 315))
		{   // more change in x	
			if (angle >= 45 && angle < 135)
			{
				if (angle < 90)
				{   // row 1 of Table 2.1 Hanson CS200 notes
					D            = deltaX  - 2*deltaY;
					deltaDplus =            -2*deltaY;
					deltaDminus = 2*deltaX - 2*deltaY;
					bumpMore  = &NO_OP;
					bumpLess  = &INCREMENT_Y;
				}
				else // deltaY negative
				{   // row 3
					D             = -deltaX - 2*deltaY;
					deltaDplus =  -2*deltaX - 2*deltaY;
					deltaDminus =            -2*deltaY;
					bumpMore =  &DECREMENT_Y;
					bumpLess =  &NO_OP;
				}
				bumpIndex = &INCREMENT_X;
				deltaDequal = deltaDplus;
				bumpSame = bumpMore;
			}
			else // deltaX negative
			{
				if (angle >= 270)
				{   // row 5 of Table 2.1
					D             = deltaX + 2*deltaY;
					deltaDplus =  2*deltaX + 2*deltaY;
					deltaDminus =            2*deltaY;
					bumpMore =  &INCREMENT_Y;
					bumpLess =  &NO_OP;
				}
				else // deltaY negative
				{   // row 7
					D             = -deltaX + 2*deltaY;
					deltaDplus =              2*deltaY;
					deltaDminus = -2*deltaX + 2*deltaY;
					bumpMore  = &NO_OP;
					bumpLess  = &DECREMENT_Y;
				}
				bumpIndex = &DECREMENT_X;
				deltaDequal = deltaDminus;
				bumpSame = bumpLess;
			}

		}
		else   // more change in y
		{
			if (angle >= 315 || angle < 45)
			{
				if (deltaX >= 0)
				{   // row 2 of Table 2.1
					D           = 2*deltaX -   deltaY;
					deltaDplus =  2*deltaX - 2*deltaY;
					deltaDminus = 2*deltaX;
					bumpMore  = &INCREMENT_X;
					bumpLess  = &NO_OP;
				}
				else // deltaX negative
				{   // row 6 of Table 2.1
					D           = 2*deltaX +   deltaY;
					deltaDplus =  2*deltaX;
					deltaDminus = 2*deltaX + 2*deltaY;
					bumpMore  = &NO_OP;
					bumpLess  = &DECREMENT_X;
				}
				bumpIndex = &INCREMENT_Y;
				deltaDequal = deltaDminus;
				bumpSame = bumpLess;
			}
			else // deltaY negative
			{
				if (angle < 180)
				{   // row 4
					D           = -2*deltaX - deltaY;
					deltaDplus =  -2*deltaX;
					deltaDminus = -2*deltaX - 2*deltaY;
					bumpMore =  &NO_OP;
					bumpLess =  &INCREMENT_X;
				}
				else // deltaX negative
				{   // row 8
					D           = -2*deltaX + deltaY;
					deltaDplus =  -2*deltaX + 2*deltaY;
					deltaDminus = -2*deltaX;
					bumpMore =  &DECREMENT_X;
					bumpLess =  &NO_OP;
				}
				bumpIndex = &DECREMENT_Y;
				deltaDequal = deltaDplus;
				bumpSame = bumpMore;
			}
		}
		// Now draw the line
		for (i = 0; i < FILTER_DIAM; i++)
		{
			bumpIndex();
			if (D > 0)
			{
				bumpMore();
				D += deltaDplus;
			}
			else if (D < 0)
			{
				bumpLess();
				D += deltaDminus;
			}
			else  // D == 0
			{
				bumpSame();
				D += deltaDequal;
			}
			if(g_column < g_min_col || g_column >= g_max_col ||
				g_row < g_min_row || g_row >= g_max_row)
				break;
			if (color)
				WRITE_COLOR();
			else
				WRITE_PIXEL();
		}
		GOTO_POINT( col_a, row_a);
		deltaX = -deltaX;
		deltaY = -deltaY;
		angle += 180;
		if (angle >= 360)
			angle -= 360;
	}  // do it once more in the other direction

}
/*---------------------------------------------------------------------------*/
void LINE_BRESENHAM_COLOR(float xa, float ya, float xb, float yb)
{
	int col_a, row_a;
	int col_b, row_b;
	register float D, deltaDplus, deltaDminus, deltaDequal;
	float deltaX, deltaY;
	void (*bumpIndex)(void);
	void (*bumpLess)(void);
	void (*bumpMore)(void);
	void (*bumpSame)(void);
	register short int i;
	short int istart;
	register short int iend;

	col_a = ROUND(xa);
	row_a = ROUND(ya);
	col_b = ROUND(xb);
	row_b = ROUND(yb);
	GOTO_POINT( col_a, row_a);
	WRITE_COLOR();
	if (col_a == col_b && row_a == row_b)
		return;

	deltaX = xb - xa;
	deltaY = yb - ya;

	if( ABS(deltaY) <= ABS(deltaX) )
	{   // more change in x		
		if (deltaX >= 0)
		{
			if (deltaY >= 0)
			{   // row 1 of Table 2.1
				D            = deltaX  - 2*deltaY;
				deltaDplus =            -2*deltaY;
				deltaDminus = 2*deltaX - 2*deltaY;
				bumpMore  = &NO_OP;
				bumpLess  = &INCREMENT_Y;
			}
			else // deltaY negative
			{   // row 3
				D             = -deltaX - 2*deltaY;
				deltaDplus =  -2*deltaX - 2*deltaY;
				deltaDminus =            -2*deltaY;
				bumpMore =  &DECREMENT_Y;
				bumpLess =  &NO_OP;
			}
			istart = col_a;
			iend = col_b;
			bumpIndex = &INCREMENT_X;
			deltaDequal = deltaDplus;
			bumpSame = bumpMore;
		}
		else // deltaX negative
		{
			if (deltaY >= 0)
			{   // row 5 of Table 2.1
				D             = deltaX + 2*deltaY;
				deltaDplus =  2*deltaX + 2*deltaY;
				deltaDminus =            2*deltaY;
				bumpMore =  &INCREMENT_Y;
				bumpLess =  &NO_OP;
			}
			else // deltaY negative
			{   // row 7
				D             = -deltaX + 2*deltaY;
				deltaDplus =              2*deltaY;
				deltaDminus = -2*deltaX + 2*deltaY;
				bumpMore  = &NO_OP;
				bumpLess  = &DECREMENT_Y;
			}
			istart = col_b;
			iend = col_a;
			bumpIndex = &DECREMENT_X;
			deltaDequal = deltaDminus;
			bumpSame = bumpLess;
		}

	}
	else   // more change in y
	{
		if (deltaY >= 0)
		{
			if (deltaX >= 0)
			{   // row 2 of Table 2.1
				D           = 2*deltaX -   deltaY;
				deltaDplus =  2*deltaX - 2*deltaY;
				deltaDminus = 2*deltaX;
				bumpMore  = &INCREMENT_X;
				bumpLess  = &NO_OP;
			}
			else // deltaX negative
			{   // row 6 of Table 2.1
				D           = 2*deltaX +   deltaY;
				deltaDplus =  2*deltaX;
				deltaDminus = 2*deltaX + 2*deltaY;
				bumpMore  = &NO_OP;
				bumpLess  = &DECREMENT_X;
			}
			istart = row_a;
			iend = row_b;
			bumpIndex = &INCREMENT_Y;
			deltaDequal = deltaDminus;
			bumpSame = bumpLess;
		}
		else // deltaY negative
		{
			if (deltaX >= 0)
			{   // row 4
				D           = -2*deltaX - deltaY;
				deltaDplus =  -2*deltaX;
				deltaDminus = -2*deltaX - 2*deltaY;
				bumpMore =  &NO_OP;
				bumpLess =  &INCREMENT_X;
			}
			else // deltaX negative
			{   // row 8
				D           = -2*deltaX + deltaY;
				deltaDplus =  -2*deltaX + 2*deltaY;
				deltaDminus = -2*deltaX;
				bumpMore =  &DECREMENT_X;
				bumpLess =  &NO_OP;
			}
			istart = row_b;
			iend = row_a;
			bumpIndex = &DECREMENT_Y;
			deltaDequal = deltaDplus;
			bumpSame = bumpMore;
		}
	}
	// Now draw the line
	for (i = istart; i < iend; i++)
	{
		bumpIndex();
		if (D > 0)
		{
			bumpMore();
			D += deltaDplus;
		}
		else if (D < 0)
		{
			bumpLess();
			D += deltaDminus;
		}
		else  // D == 0
		{
			bumpSame();
			D += deltaDequal;
		}
		WRITE_COLOR();
	}
}
/*---------------------------------------------------------------------------*/
void DrawLine( int i, PIXEL *image, int rowLength)
{
	float xa, ya;
	float angle;

	if (i < 0 || Location[i].Type < eEDGE)
		return;
	/* Line contains (Location[i].column, Location[i].row) and is at angle
	   Location[i].degrees.
	   Drawing area has upper left corner at *image and is a square of size 
	   Location[i].diam 
	   Assume that if rowLength < BOUNDS_RIGHT we are drawing into a subimage. */
	g_buffer = image;
	g_rowlength = rowLength;
	/* Find the points at which line cuts the square. */
	angle = (float) Location[i].degrees;
	if (rowLength < BOUNDS_RIGHT)
	{  // subimage
		xa = Location[i].column - Location[i].c + Location[i].diam/2;
		ya = Location[i].row    - Location[i].r + Location[i].diam/2;
		g_max_row = g_max_col = Location[i].diam;
		g_min_row = g_min_col = 0;
		if (xa < g_min_col)
			xa = (float) g_min_col;
		if (xa >= g_max_col)
			xa = (float) g_max_col-1;
		if (ya < g_min_row)
			ya = (float) g_min_row;
		if (ya >= g_max_row)
			ya = (float) g_max_row-1;
	}
	else
	{  // full image
		xa = Location[i].column;
		ya = Location[i].row;
		g_min_row = Location[i].r - Location[i].diam/2;
		g_max_row = g_min_row + Location[i].diam;
		g_min_col = Location[i].c - Location[i].diam/2;
		g_max_col = g_min_col + Location[i].diam;
	}

	LINE_BRESENHAM_FLOAT(xa, ya, angle, FALSE);

}
/*---------------------------------------------------------------------------*/
/*  test the line plotting routine */
void TestLine()
{
	FILE *fp;
	errno_t error;
	int i, j;
	PIXEL test[FILTER_DIAM * FILTER_DIAM];
	char filename[25];
	PIXEL *pImage = test;
	int index = 0;


	Location[0].degrees = -45;
	Location[0].column = 0;
	Location[0].row =15;
	Location[0].Type = eCORNER;
	Location[0].c = 9;
	Location[0].r = 9;
	Location[0].diam = FILTER_DIAM;
	for (index = 0; index < 8; index++)
	{
		pImage = test;
		Location[0].degrees += 45;
		for (i = 0; i < FILTER_DIAM; i++)
			for (j = 0; j < FILTER_DIAM; j++)
				test[i+FILTER_DIAM*j] = 128;
		DrawLine( 0, test, FILTER_DIAM);
		sprintf_s(filename, 25, "Debug\\Test%.0i.PGM", Location[0].degrees);
		error = fopen_s(&fp, filename, "w");
		if (error || fp == NULL)
		{
			printf("TestLine Error %i", error);
			return;
		}
		fprintf_s(fp, "P5 %i %i 255 ", FILTER_DIAM, FILTER_DIAM);
 		for(i=0; i < FILTER_DIAM; i++)
		{
			fwrite(pImage, sizeof(PIXEL), FILTER_DIAM, fp);
			pImage += FILTER_DIAM;
		}
		fclose(fp);
	}

}
/*---------------------------------------------------------------------------*/
/* Write a PGM image for debugging 
void WritePGM( int Width, int Height, PIXEL *image, char *filename, 
			   int featureIndex, struct FILTER *pKern)
{
	FILE *fp;
	int i, j;
	int c;
	PIXEL *r, *pPixel;
	PIXEL ImageCopy[LINE_BUFFER * LINE_BUFFER];
	PIXEL *pImage = ImageCopy;
	PIXEL *pLocation, *last_row;
	unsigned char image_line[LINE_BUFFER];
	int spacing = 1;
	if(pKern)
	{
		spacing = pKern->sampleSpacing;
	}
	pLocation = image;
	last_row = pLocation + BOUNDS_RIGHT * spacing * Height;
	i = 0;
	for (r = pLocation; r < last_row; r+=spacing*BOUNDS_RIGHT)
	{
		pPixel = r;
		for (j = c = 0; c < spacing * Width; c+=spacing)
		{
 			ImageCopy[(j++) + LINE_BUFFER*i] = pPixel[c];
		}
		i++;
	}

	DrawLine( featureIndex, ImageCopy, FILTER_DIAM);
	fopen_s(&fp, filename, "wb");
	fprintf_s(fp, "P5 %i %i 255 ", Width, Height);
 	for(i=0; i < Height; i++)
	{
		for (j=0; j < Width; j++)
		{
			image_line[j] = (unsigned char) pImage++;
		}
		fwrite(image_line, sizeof(unsigned char), Width, fp);
	}
	fclose(fp);
}*/
/*---------------------------------------------------------------------------*/
/* Write a PGM or PPM image of the whole input plus found lines */
void WriteALL( int Width, int Height, PIXEL *image, char *filename, int Magic)
{
	FILE *fp;
	errno_t error;
	int i;
	int width_pixels;
	int pixel_bytes;
	if (Magic < 1 || Magic >6)
		Magic = 5;  // Gray
	pixel_bytes = (Magic == 5) ? 1 : 3;
	width_pixels = (Magic == 5) ? Width : 3* Width;
	error = fopen_s(&fp, filename, "wb");
	if (error || fp == NULL)
	{
		printf("WriteALL Error %i", error);
		return;
	}
	fprintf_s(fp, "P%i %i %i 255 ", Magic, Width, Height);
 	for(i=0; i < Height; i++)
	{
		fwrite(image, sizeof(PIXEL), width_pixels, fp);
		image += pixel_bytes*BOUNDS_RIGHT;
	}
	fclose(fp);
}
/*---------------------------------------------------------------------------*/
// distance in meters
#define MIN_INDEX ((float) 0.5)
#define MAX_INDEX 90
#define PURPLE_SIZE 5
#define COLORS_SIZE 18
#define PURPLE_DELTA ((float) 0.4921875)
#define PURPLE  g_purples[1]
struct RGB_COLOR
{ 
	PIXEL red;
	PIXEL green;
	PIXEL blue;
};
struct RGB_COLOR g_colors[COLORS_SIZE] =
{
	128,   0,   0, // red-brown
	255,   0,   0, // red
	255,   0, 128, // deep pink
	255, 128, 128, // flesh
	255, 255, 128, // light yellow
	255, 255,   0, // yellow
	255, 128,   0, // orange
	128, 128,   0, // olive
	  0, 128,   0, // dark green
	  0, 255,   0, // green
    128, 255,   0, // bright green
	128, 255, 128, // light green
	128, 255, 255, // aqua
	  0, 255, 255, // cyan
	  0, 255, 128, // medium blue
	  0, 128, 128, // blue green
	  0,   0, 128, // dark blue
	  0,   0, 255  // blue
};
struct RGB_COLOR g_purples[PURPLE_SIZE] =
{
	128,   0, 128, // plum
	128,   0, 255, // purple
	255,   0, 255, // magenta
	255, 128, 255, // lavender
	128, 128, 255  // light purple
};
/*---------------------------------------------------------------------------*/
void SelectColor( float index)
{
	int i;
	float fraction;
	static float last_purple = 0;

	if (index == UNUSED)
	{  /* use purples */
		while (last_purple >= PURPLE_SIZE-1)
			last_purple -= (PURPLE_SIZE-1);
		i =  (int)(last_purple);
		fraction = last_purple - i;
		g_red = g_purples[i].red == g_purples[i+1].red ?
			g_purples[i].red:
			g_purples[i].red + fraction * (g_purples[i+1].red - g_purples[i].red);
		g_green = g_purples[i].green == g_purples[i+1].green ?
			g_purples[i].green:
			g_purples[i].green + fraction * (g_purples[i+1].green - g_purples[i].green);
		g_blue = g_purples[i].blue == g_purples[i+1].blue ?
			g_purples[i].blue:
			g_purples[i].blue + fraction * (g_purples[i+1].blue - g_purples[i].blue);
		return;
	}
	else if (index == INC_UNUSED)
	{
		last_purple += PURPLE_DELTA;
		return;
	}
	else 
		index = FL_ABS(index);
	if (index <= MIN_INDEX)
	{  /* red-brown */
		g_red   = g_colors[0].red;
		g_green = g_colors[0].green;
		g_blue  = g_colors[0].blue;
	}
	else if (index >= MAX_INDEX)
	{   /* blue */
		g_red   = g_colors[COLORS_SIZE-1].red;
		g_green = g_colors[COLORS_SIZE-1].green;
		g_blue  = g_colors[COLORS_SIZE-1].blue;
	}
	else
	{
		i =  (int)(COLORS_SIZE * (index - MIN_INDEX) / (MAX_INDEX - MIN_INDEX));
		fraction = COLORS_SIZE * (index - MIN_INDEX) / (MAX_INDEX - MIN_INDEX) - i;
		g_red = g_colors[i].red == g_colors[i+1].red ?
			g_colors[i].red:
			g_colors[i].red + fraction * (g_colors[i+1].red - g_colors[i].red);
		g_green = g_colors[i].green == g_colors[i+1].green ?
			g_colors[i].green:
			g_colors[i].green + fraction * (g_colors[i+1].green - g_colors[i].green);
		g_blue = g_colors[i].blue == g_colors[i+1].blue ?
			g_colors[i].blue:
			g_colors[i].blue + fraction * (g_colors[i+1].blue - g_colors[i].blue);
	}
}
/*---------------------------------------------------------------------------*/
//  The monochrome image read as P2 or P5 pgm is no longer needed.
//  Overwite a conversion to color ppm.
void makeColorImage(int Height, int Width, PIXEL *image)
{
	int j;
	PIXEL pel;
	g_buffer = image;
	g_rowlength = BOUNDS_RIGHT;

	for(j = Height*Width-1; j >=0; j--)
	{
		pel = g_buffer[j];
		g_buffer[3*j] = pel;
		g_buffer[3*j+1] = pel;
		g_buffer[3*j+2] = pel;
	}
}
/*---------------------------------------------------------------------------*/
static int PlotPoly( struct Polyline *pL)
{
    int i;
    int v, v_old;  /* for each vertex */

    v = pL->first;
	if (v == UNUSED)
		return OK;
	if ( pL->Vertices == 1)
	{
		SelectColor(UNUSED); // Location[v].Depth );

		g_min_row = Location[v].r - Location[v].diam/2;
		g_max_row = g_min_row + Location[v].diam;
		g_min_col = Location[v].c - Location[v].diam/2;
		g_max_col = g_min_col + Location[v].diam;
		LINE_BRESENHAM_FLOAT(Location[v].column, Location[v].row,
			Location[v].degrees, TRUE);
	}
	v_old = v;
    v = Location[v].Ahead;
    for (i = 1; i < pL->Vertices; i++)
    {   /* Column, Row */
		if (v == UNUSED)
			return UNUSED;
		SelectColor(UNUSED);

		LINE_BRESENHAM_COLOR(Location[v_old].column, Location[v_old].row,
			Location[v].column, Location[v].row);
		v_old = v;
        v = Location[v].Ahead;
        if (v == UNUSED)
             break;
    }
	if (pL->u.f.Closed)
	{
	    v = pL->first;
		v_old = pL->last;
		SelectColor(UNUSED);

		LINE_BRESENHAM_COLOR(Location[v_old].column, Location[v_old].row,
			Location[v].column, Location[v].row);
	}
	SelectColor( INC_UNUSED );
    return OK;
}
/*---------------------------------------------------------------------------*/
void GraphPolylines(char *name, struct Polyline *pUPoly, int Unum,
								struct Polyline *pLPoly, int Lnum)
{
	int i;
	makeColorImage(ImageHeight, ImageWidth, Image);
	for (i=0; i<Unum; i++)
	{
		PlotPoly(pUPoly++);
	}
	for (i=0; i<Lnum; i++)
	{
		PlotPoly(pLPoly++);
	}
	WriteALL( ImageWidth, ImageHeight, g_buffer, name, 6);
}
