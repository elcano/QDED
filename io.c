/* IO.c
  April 26, 2005   Tyler Folsom
  Routines to communicate with camera and DSP

  Copyright (c) 2005  Tyler Folsom.  All rights reserved.
*/

#include "features.h"
#include <stdio.h>  /* for FILE, fopen, fputc */
#include <ctype.h>  /* for isspace */


PIXEL Image[3 * BOUNDS_RIGHT * BOUNDS_BOTTOM];
int ImageWidth;
int ImageHeight;
int ReadPGM(char *image_name);
extern void WritePPM(int Width, int Height, PIXEL* image, char* filename);
extern void WritePGM( int Width, int Height, PIXEL *image, char *filename, 
			   int featureIndex, struct FILTER *pKern);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int ReadPGM(char *image_name)
{
//	struct FILTER *null = 0;
	FILE *fp;
    char c;
	int i, j, k, jj;
	int chunks, remainder;
	unsigned char image_line[LINE_BUFFER];
	char magicNum[4];
	int width, height, bits;
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
	if (magicNum[0] != 'P' || (magicNum[1] != '2' && magicNum[1] != '5') )
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
	// assert 3 numbers, right size, bits == 8
	/* clear out image */
	for (i = 0; i < BOUNDS_RIGHT * BOUNDS_BOTTOM; i++)
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
	else
	{	// read binary
	/* ignore the one byte of white space after 255 */
		i = fgetc(fp);
		if (width < LINE_BUFFER)
		{
 			for(i=0; i < height; i++)
			{
				k = i * BOUNDS_RIGHT;
				charsRead = fread(&image_line, sizeof(unsigned char), width, fp);
				if (charsRead < width)
				{
					printf("%d characters read; expected %d", charsRead, width);
				}
				for (j=0; j < width; j++)
					Image[k+j] = image_line[j];
			}
		}
		else
		{
			chunks = width / LINE_BUFFER;
			remainder = width - chunks * LINE_BUFFER;
 			for(i=0; i < height; i++)
			{
				k = i * BOUNDS_RIGHT;
				for (j = 0; j < chunks; j++)
				{
					charsRead = fread(&image_line, sizeof(unsigned char), LINE_BUFFER, fp);
					if (charsRead < LINE_BUFFER)
					{
						printf("%d characters read; expected %d", charsRead, LINE_BUFFER);
					}
					for (jj = 0; jj < LINE_BUFFER; jj++)
						Image[k+j*LINE_BUFFER+jj] = image_line[jj];
				}
				if (remainder > 0)
					charsRead = fread(&image_line, sizeof(unsigned char), remainder, fp);
					if (charsRead < remainder)
					{
						printf("%d characters read; expected %d", charsRead, remainder);
					}
					for (jj = 0; jj < remainder; jj++)
						Image[k+j*LINE_BUFFER+jj] = image_line[jj];
			}
		}
	}
	fclose(fp);
	ImageWidth = width;  // set globals
	ImageHeight = height;
	/* DEBUG */
	WriteALL(width, height, Image, "Images\\carl5.PGM");
//	WritePGM( width, height, &Image[0], "Karla5.PGM", UNUSED, NULL);

	return 0;   // success 
}
/*---------------------------------------------------------------------------*/

