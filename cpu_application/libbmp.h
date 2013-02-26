/*
 * libbmp.h
 *
 * Bitmap library to read and write standard
 * 24-bit uncompressed windows bitmap files
 *
 *  Created on: 24-Jun-2010
 *      Author: bhagya
 */

#ifndef LIBBMP_H_
#define LIBBMP_H_

#include <stdio.h>

typedef struct {
   unsigned short type;
   unsigned long size;
   unsigned short reserved1;
   unsigned short reserved2;
   unsigned long offsetbits;
} BITMAP_FILE_HEADER;

typedef struct {
   unsigned long size;
   unsigned long width;
   unsigned long height;
   unsigned short planes;
   unsigned short bitcount;
   unsigned long compression;
   unsigned long sizeimage;
   long xpelspermeter;
   long ypelspermeter;
   unsigned long colorsused;
   unsigned long colorsimportant;
} BITMAP_INFO_HEADER;

//Data structure to hold 1 pixel:
typedef struct {
   unsigned char blue;
   unsigned char green;
   unsigned char red;
} PIXEL;


void
readBMP (FILE *fp,
		unsigned char **red,
		unsigned char **green,
		unsigned char **blue,
		unsigned int *height,
		unsigned int *width);

void
writeBMP (char *out_name,
		unsigned char *red,
		unsigned char *green,
		unsigned char *blue,
		unsigned int height,
		unsigned int width);

#endif /* LIBBMP_H_ */
