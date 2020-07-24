/*
   QDED - Quadrature Disambiguation Edge Detection
   June 25, 2020  Tyler Folsom

   Code is intended for an embedded processor (such as Raspberry PI) on an autonomous vehicle.
   Current version of code compiles on Microsoft visual Studio.

   Code is meant to find lane edges and any obstacles within the lane.
   It may also identify orange cones.

*/
#include <stdio.h>  /* for FILE, fopen, fputc */
#include <ctype.h>  /* for clock_t */
#include <time.h>
#include "features.h"  /* for PIXEL */
/* delay in seconds before starting robot in motion */
#define DELAY         0.0

extern PIXEL Hue[];
extern PIXEL Saturation[];
extern PIXEL Intensity[];
extern PIXEL Image[];
extern int ImageWidth;
extern int ImageHeight;

int qded(char* image_name);
void RGB2Gray(PIXEL* RGBimage, PIXEL* Grayimage, int height, int width);
void RGB2Saturation(PIXEL* RGBimage, PIXEL* Saturation, int height, int width);
void RGB2Hue(PIXEL* RGBimage, PIXEL* Hueimage, int height, int width);
void HueFilter(int minN,int minY, int maxY, int maxN,
	PIXEL* Hueimage, int height, int width);
void WriteALL(int Width, int Height, PIXEL* image, char* filename, int Magic);
int ReadPGM(char* image_name);
/*
Get the estimated vehicle state.
State is position (xw, yw), velocity, yaw, pitch, roll etc.
These are measured in the world coordinate system,based on a local origin.
xw is distance east of the origin in meters; yw distance north. If used, zw would be up
Negative velocity means vehicle is in reverse.
*/
// getVehicleState();  // TO DO

/*
Get the camera parameters.
State is position (xv, yv, zv), velocity, yaw, pitch, roll, focal length and camera projection coefficients.
These are measured in the vehicle coordinate system, with the origin at vehicle center.
xv points ahead (meters); yv to the left; zv up.
These parameters are not expected to change.
*/
// getCameraState();  // TO DO

/*
Get the road state near the vehicle.
Road state consists of lane width (meters), distance to next intersection,
Number of additional lanes to right, number of lanes to left, and
vehicle orientation (radians) relative to lane. A zero orientation means that vehicle
is pointing the same direction as the lane.
Road state may include previous estimate of curvature.
*/
// getRoadState();   // TO DO

/*
Construct a 2D map of expected lane and road
*/
// make2DRoad();  // TO DO

/*
Do coordinate transformations using matrices to make the expected image of the road.
This makes use of vehicle position, camera position, road position, and camera projection.
*/
// make3DRoad();   // TO DO

/*
Apply QDED to the expected image and record the expected data in each circular tile.
*/
// expectedQDED();   // TO DO

/*
Acquire an image. For test, it will be a .PGM image, taken from the CARLA driving simulator.
PGM is a simple format with no compression that can be read without needing any library routines.
Processed images may be written for debugging, but they are not used for anything else.
For live robotics, the images will be frames from video.
The QDED algorithm tiles the image with small circles.
Within each circle, it finds the position of the dominant edge.

QDED works best on monochrome images. If image is color, convert to monochrome.
*/
// qded(image_name);
/* need to make changes to qded.
The tile positions should be aligned with the expected lane edges.
Newly found edglets should be combined and discrepancy from expected lane edge noted.
Update position in lane and transmit it over CAN bus.
*/

/*
Apply Kalman filter to update expected state of vehicle, road, and maybe camera
*/
// Kalman();   // TO BE modified.

/*
Send updated state over CAN bus.
*/
// putState();   // TO DO

int ConeDetect(char* image_name)
{
	int width, height;
	// getVehicleState();
	// getCameraState();
	// getConeLocation();
	// getColorImage();
	if (0 != ReadPGM(image_name))
		return 1;
	width = ImageWidth;  // get globals
	height = ImageHeight;
	/*
	Convert RGB image to (Hue, Saturation, Intensity) image
	Theta = acos( (R-(G+B)/2) / sqrt( (R-G)*(R-G) + (R-B)(G-B)));
	H = (B>G)? 2*PI - Theta : Theta;
	S = 1 - 3* min(R,G,B) / (R+G+B);
	I = (R+G+B)/3;
	Work with either the H or I image
	*/
	// makeMonochrome();
	RGB2Gray(Image, Intensity, height, width);
	WriteALL(width, height, Intensity, "Images\\Gray_cone.pgm", 5);
	WriteALL(width, height, Image, "Images\\RGB_cone.ppm", 6);
	/* July 24, 2020  Tyler Folsom
	   Turns out hue is not all that great for finding orange.
	   Some shades of brown have the same hue.
	   It would be possible to use the CIE transform into two color
	   dimensions and use that to filter orange pixels.
	   https://www.image-engineering.de/library/technotes/958-how-to-convert-between-srgb-and-ciexyz
	  https://www.researchgate.net/figure/Left-Approximate-color-regions-on-CIE-1931-x-y-chromaticity-diagram-Gage-et-al_fig4_280903173
	  However, the computation is probably too intense and not much better than just using a region of RGB color space.
	  A strong orange is R=255, G=102 or 103, B=0.
	 The region around R = 240-255, G = 88 to 157, B = 0-50 can be considered orange.
	*/
	RGB2Saturation(Image, Saturation, height, width);
	WriteALL(width, height, Saturation, "Images\\Sat_cone.pgm", 5);

	RGB2Hue(Image, Hue, height, width);
	// Find Orange
	HueFilter(0, 6, 22, 28, Hue, height, width);
	WriteALL(width, height, Hue, "Images\\Orange_cone.pgm", 5);

	/*
	Find three Peaks of vertical and horizontal histogram.
	Take their intersections as the location of an orange blob
	For Hue, sum the angle from orange and select the minimum.
	May filter this to prefer near-orange hues.
	Let the three peak hues be in order (X1, X2, X3) and (Y1, Y2, Y3)
	Then the strongest orange blob is at (X1, Y1). with 8 other candidates.
	*/
	// MakeHistograms();
	// expectedCONEqded(image_name);
	// qdedCone();
	// qdedImage();
	// Find best fit to expected image
	// Kalman();
	// putState();
	return 0;
}

int LaneDetect(char* image_name)
{
	// getVehicleState();
	// getCameraState();
	// getRoadState();
	// make2DRoad();
	// make3DRoad();
	// expectedQDED();
	qded(image_name);
	// Kalman();
	// putState();
	return 0;
}

void ObstacleDetect()
{
	// Use the lanes found as the region of interest.
	// Tile this region with QDED
	// Any line segments found are obstacles
	// Report them over CAN
}

int main(int argc, char* argv[])
// argv: prog_name  image_name.pgm

{
	/* The embedded vision software processes the scene and shares what it finds
	   with the rest of the system over a CAN bus serial connection.
	   The main output is the position in lane, given as the distance in meters from the right edge
	   and position in meters from the left edge.
	   Additional outputs are positions of orange cones and obstacles in the lane.
	   Inputs are estimates of vehicle position, configuration of the lane, and 
	   previously estimated positions of cones and obstacles.
	*/
	time_t time;  /* time in CLOCKS_PER_SEC to do processing */
	time_t startTime;
	float elapsed;
	time = startTime = clock();
	/* initial delay */
	//printf("waiting\n");
	while ((time - startTime) < DELAY * CLOCKS_PER_SEC)
	{
		time = clock();
	}
	//printf("moving\n");

	char* image_name;	
//	image_name = argc < 2 ? "Images\\Carla5.ppm" : argv[1];
	image_name = "Images\\IMG_0445crop.ppm";

	// Find Orange Cones
	ConeDetect(image_name);

	// Find Lane Edges
//	LaneDetect(image_name);

	// Find Obstacles
	ObstacleDetect();  // TO DO

	time = clock();
	elapsed = (float)(time - startTime) / CLOCKS_PER_SEC;
	printf("Elapsed time: %.3f seconds/n", elapsed);

}