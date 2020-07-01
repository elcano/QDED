/*
Cone.c     Tyler Folsom  June 30,2020
Routines to convert color RGB to HSI
Routines for orange blob detection
*/
#include "features.h"
#include <math.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define TWO_PI 6.283185307179586476925286
/*---------------------------------------------------------------------------*/
/* global variables */
/* how many locations are used in the grid */
extern int g_locations;
extern struct FEATURE  Location[MAX_LOCS];
extern int ImageWidth;
extern int ImageHeight;
// size is 3 * BOUNDS_RIGHT * BOUNDS_BOTTOM
extern PIXEL Image[];

PIXEL Hue[BOUNDS_RIGHT * BOUNDS_BOTTOM];
PIXEL Saturation[BOUNDS_RIGHT * BOUNDS_BOTTOM];
PIXEL Intensity[BOUNDS_RIGHT * BOUNDS_BOTTOM];

/*
form the intensity image by taking the average of the R,G,B at eaach pixel.
*/
void RGB2Gray(PIXEL* RGBimage, PIXEL* Grayimage, int height, int width)
{
	int i, j, k, k3;
	for (i = 0; i < height; i++)
	{
		k = i * BOUNDS_RIGHT;
		k3 = i * 3*BOUNDS_RIGHT;
		for (j = 0; j < width; j++)
		{
			Grayimage[k++] = (RGBimage[k3] +RGBimage[k3 + 1] + RGBimage[k3 + 2]) / 3;
			k3 += 3;
		}
	}
}

/*
form the saturated image by taking  1 - 3* min(R,G,B) / (R+G+B) on a scale of 0 to 1
*/
void RGB2Saturation(PIXEL* RGBimage, PIXEL* Satimage, int height, int width)
{
	int i, j, k, k3;
	for (i = 0; i < height; i++)
	{
		k = i * BOUNDS_RIGHT;
		k3 = i * 3 * BOUNDS_RIGHT;
		for (j = 0; j < width; j++)
		{   // S = 1 - 3* min(R,G,B) / (R+G+B);
			PIXEL rg = MIN(RGBimage[k3], RGBimage[k3 + 1]);		
			int min_rgb = 3 * 255 * MIN(rg, RGBimage[k3 + 2]);
			int sum = RGBimage[k3] + RGBimage[k3 + 1] + RGBimage[k3 + 2];
			Satimage[k++] = 255 - min_rgb / sum;
			k3 += 3;
		}
	}
}
/*
form the Hue image by Theta = acos( (R-(G+B)/2) / sqrt( (R-G)*(R-G) + (R-B)(G-B)));
	H = (B>G)? 2*PI - Theta : Theta; 0n scale of 0 to 2 Pi
*/
void RGB2Hue(PIXEL* RGBimage, PIXEL* Hueimage, int height, int width)
{
	int i, j, k, k3;
	for (i = 0; i < height; i++)
	{
		k = i * BOUNDS_RIGHT;
		k3 = i * 3 * BOUNDS_RIGHT;
		for (j = 0; j < width; j++)
		{   
			double red = RGBimage[k3];
			double green = RGBimage[k3 + 1];
			double blue = RGBimage[k3 + 2];
			double root = sqrt((red - green) * (red - green) + (red - blue) * (green - blue));
			double theta = acos((red-(green+blue))/(2*root));
			double color = (blue > green) ? TWO_PI - theta: theta;
			Hueimage[k++] = 255 * color / TWO_PI;
			k3 += 3;
		}
	}
}
