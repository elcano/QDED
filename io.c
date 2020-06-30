/* IO.c
  April 26, 2005   Tyler Folsom
  Routines to communicate with camera and DSP

  Copyright (c) 2005, 2020  Tyler Folsom.  All rights reserved.
*/

#include "features.h"
#include <stdio.h>  /* for FILE, fopen, fputc */
#include <ctype.h>  /* for isspace */


PIXEL Image[3 * BOUNDS_RIGHT * BOUNDS_BOTTOM];
int ImageWidth;
int ImageHeight;
int ReadPGM(char *image_name);
extern void WriteALL(int Width, int Height, PIXEL* image, char* filename, int Magic);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int ReadPGM(char *image_name)
{
//	struct FILTER *null = 0;
	FILE *fp;
    char c;
	int i, j, k;
	PIXEL* pImage;
	char magicNum[4];
	int width, height, bits;
	int byte_size, pixel_bytes;
	int width_pixels;
	size_t charsRead;
	/* read a PCX file; default is image.pgm */
	fopen_s(&fp, image_name, "rb");
    if (fp == NULL) return 1;
	magicNum[0] = (char) getc(fp);
	magicNum[1] = (char) getc(fp);
	/* magic numbers
	Format                   Magic number 	Bits	For
	Portable Bit Map (PBM) ASCII	P1	1	Monochrome (B&W)
	Portable Gray Map (PGM) ASCII	P2	any	Gray scale
	Portable Pixel Map (PPM) ASCII	P3	any	Color (RGB)
	Portable Bit Map (PBM) binary	P4	1	Monochrome (B&W)
	Portable Gray Map (PGM) binary	P5	1 to 8	Gray scale
	Portable Pixel Map (PPM) binary	P6	1 to 24	Color (RGB)
	*/
	if (magicNum[0] != 'P' || (magicNum[1] != '2' && magicNum[1] != '5' && magicNum[1] != '6') )
		return (1);  // unsupported format
	do {
		c = fgetc(fp);
	} while (isspace(c));
    if ( c == '#' )
    {
        /* optional comment line is present */
        while ( c != '\n' ) c=fgetc(fp);
    }
    else
    {
        ungetc(c,fp); /* push char back so we can scan the line */
    }
	fscanf_s(fp, "%i%i%i", &width, &height, &bits);
	/* clear out image */
	pImage = Image;
	for (i = 0; i < 3* BOUNDS_RIGHT * BOUNDS_BOTTOM; i++)
		Image[i] = 0;
	if (magicNum[1] == '2')
	{  // readASCII
		for(i=0; i < height; i++)
		{
			k = i * BOUNDS_RIGHT;
			for(j=0; j<width; j++)
			{
				fscanf_s(fp, "%c", &Image[k++], 1);
			}
		}
	}
	else // (magicNum[1] == '5' or '6')
	{	// read binary
	/* ignore the one byte of white space after 255 */
		i = fgetc(fp);
		pixel_bytes = (magicNum[1] == '5') ? 1 : 3;
		width_pixels = width * pixel_bytes;
		byte_size =  sizeof(unsigned char);
		{
 			for(i=0; i < height; i++)
			{
				charsRead = fread(pImage, byte_size, width_pixels, fp);
				if (charsRead < width_pixels)
				{
					printf("%d characters read; expected %d", charsRead, width_pixels);
				}
				pImage += pixel_bytes*BOUNDS_RIGHT;
			}
		}
	}

	fclose(fp);
	ImageWidth = width;  // set globals
	ImageHeight = height;
	/* DEBUG */
//	WriteALL(width, height, Image, "Images\\carl5.PGM", 5);  // gray image
	WriteALL(width, height, Image, "Images\\carl5.PPM", 6);  // color image
	//	WritePGM( width, height, &Image[0], "Karla5.PGM", UNUSED, NULL);

	return 0;   // success 
}
/*---------------------------------------------------------------------------*/

