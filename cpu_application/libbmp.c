#include "libbmp.h"
#include <malloc.h>

/**
 * readBMP - reads a 24-bit bmp image into three channels of R, G and B
 *
 * fp : file pointer to the bitmap file
 *
 * red, green, blue : point to memory of red,
 * green & blue buffers after the call
 *
 * height, width : contain height and width after the call
 */
void
readBMP (FILE *fp,
		unsigned char **red,
		unsigned char **green,
		unsigned char **blue,
		unsigned int *height,
		unsigned int *width)
{
	BITMAP_FILE_HEADER file_header;
	BITMAP_INFO_HEADER info_header;
	PIXEL pix;
	int i, j;
	int padding_bytes;
	unsigned char pad_byte;
	unsigned char *img_red, *img_green, *img_blue;

	//read file_header
	fread (&file_header.type, sizeof (short), 1, fp);
	fread (&file_header.size, sizeof (long), 1, fp);
	fread (&file_header.reserved1, sizeof (short), 1, fp);
	fread (&file_header.reserved2, sizeof (short), 1, fp);
	fread (&file_header.offsetbits, sizeof (long), 1, fp);

	//read info_header
	fread (&info_header.size, sizeof (long), 1, fp);
	fread (&info_header.width, sizeof (long), 1, fp);
	fread (&info_header.height, sizeof (long), 1, fp);
	fread (&info_header.planes, sizeof (short), 1, fp);
	fread (&info_header.bitcount, sizeof (short), 1, fp);
	fread (&info_header.compression, sizeof (long), 1, fp);
	fread (&info_header.sizeimage, sizeof (long), 1, fp);
	fread (&info_header.xpelspermeter, sizeof (long), 1, fp);
	fread (&info_header.ypelspermeter, sizeof (long), 1, fp);
	fread (&info_header.colorsused, sizeof (long), 1, fp);
	fread (&info_header.colorsimportant, sizeof (long), 1, fp);

	//assign height and width
	*height = info_header.height;
	*width = info_header.width;

	//allocate memory to red, green and blue channels
	img_red = malloc (sizeof(char) * info_header.height * info_header.width);
	img_green = malloc (sizeof(char) * info_header.height * info_header.width);
	img_blue = malloc (sizeof(char) * info_header.height * info_header.width);

	//calculate padding after each row
	padding_bytes = ((info_header.width * 3) % 4) ? (4 - (info_header.width * 3) % 4) : 0;

	//read bitmap data and assign to red, green and blue arrays
	for (i = 0; i < info_header.height; i++) {
		//read row data
		for (j = 0; j < info_header.width; j++) {
			//reading pixel
			fread (&pix.blue, sizeof (char), 1, fp);
			fread (&pix.green, sizeof (char), 1, fp);
			fread (&pix.red, sizeof (char), 1, fp);
			//writing to array
			*(img_red + (info_header.height - i - 1) * info_header.width + j) = pix.red;
			*(img_green + (info_header.height - i - 1) * info_header.width + j) = pix.green;
			*(img_blue + (info_header.height - i - 1) * info_header.width + j) = pix.blue;
		}
		//read padding bytes
		for (j=0; j < padding_bytes; j++) {
			fread (&pad_byte, sizeof (char), 1, fp);
		}
	}

	//pointers given by the user are pointed to the buffers
	*red = img_red;
	*green = img_green;
	*blue = img_blue;
}

/**
 * writeBMP - writes 3 channels of R, G and B in a 24-bit bmp image
 *
 * out_name : file name of bitmap file
 *
 * red, green, blue : point to memory of red, green & blue buffers
 *
 * height, width : height and width of the bmp image
 */
void
writeBMP (char *out_name,
		unsigned char *red,
		unsigned char *green,
		unsigned char *blue,
		unsigned int height,
		unsigned int width)
{
	BITMAP_FILE_HEADER file_header;
	BITMAP_INFO_HEADER info_header;
	PIXEL pix;
	FILE *fp;
	int i, j;
	int padding_bytes;
	unsigned char pad_byte;

	fp = fopen(out_name, "wb");

	//write file_header
	file_header.type = 0x4D42;
	fwrite (&file_header.type, sizeof (short), 1, fp);

	padding_bytes = ((width * 3) % 4) ? (4 - (width * 3) % 4) : 0;
	file_header.size = (3 * width + padding_bytes) * height + sizeof (BITMAP_FILE_HEADER) + sizeof (BITMAP_INFO_HEADER);
	fwrite (&file_header.size, sizeof (long), 1, fp);

	fwrite (&file_header.reserved1, sizeof (short), 1, fp);
	fwrite (&file_header.reserved2, sizeof (short), 1, fp);

	file_header.offsetbits = 0x00000036;
	fwrite (&file_header.offsetbits, sizeof (long), 1, fp);

	//read info_header
	info_header.size = 0x00000028;
	fwrite (&info_header.size, sizeof (long), 1, fp);

	info_header.width = width;
	fwrite (&info_header.width, sizeof (long), 1, fp);

	info_header.height = height;
	fwrite (&info_header.height, sizeof (long), 1, fp);

	info_header.planes = 0x0001;
	fwrite (&info_header.planes, sizeof (short), 1, fp);

	info_header.bitcount = 0x0018;
	fwrite (&info_header.bitcount, sizeof (short), 1, fp);

	info_header.compression = 0;
	fwrite (&info_header.compression, sizeof (long), 1, fp);

	info_header.sizeimage = file_header.size - 0x36;
	fwrite (&info_header.sizeimage, sizeof (long), 1, fp);

	info_header.xpelspermeter = 0x00000B13;
	fwrite (&info_header.xpelspermeter, sizeof (long), 1, fp);

	info_header.ypelspermeter = 0x00000B13;
	fwrite (&info_header.ypelspermeter, sizeof (long), 1, fp);

	info_header.colorsused = 0x00;
	fwrite (&info_header.colorsused, sizeof (long), 1, fp);

	info_header.colorsimportant = 0x00;
	fwrite (&info_header.colorsimportant, sizeof (long), 1, fp);

	pad_byte = 0x00;

	//write bitmap data from red, green and blue arrays
	for (i = 0; i < height; i++) {
		//read row data
		for (j = 0; j < width; j++) {
			//reading from array
			pix.red = *(red + (height - i - 1) * width + j);
			pix.green = *(green + (height - i - 1) * width + j);
			pix.blue = *(blue + (height - i - 1) * width + j);
			//writing pixel
			fwrite (&pix.blue, sizeof (char), 1, fp);
			fwrite (&pix.green, sizeof (char), 1, fp);
			fwrite (&pix.red, sizeof (char), 1, fp);
		}
		//write padding bytes
		for (j=0; j < padding_bytes; j++) {
			fwrite (&pad_byte, sizeof (char), 1, fp);
		}
	}

	fclose (fp);
}

