/* main.c
 *
 *      Author: bhagya, sanjay
 *      CPU based hexagonal framework
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "libbmp.h"
#include <sys/time.h>

//PARAMETERS
#define IMG "i6.bmp"
#define TMP "t6.bmp"
#define THRESH 1000
#define RESULT "result_cpu.bmp"
#define IMG_HEX "img_hex.bmp"
#define IMG_GRAY "img_gray.bmp"
#define IMG_EDGE "img_edge.bmp"
#define TMP_HEX "tmp_hex.bmp"
#define TMP_GRAY "tmp_gray.bmp"
#define TMP_EDGE "tmp_edge.bmp"

#define INDEX(x,y,w) ((int)(y)*(int)(w) + (int)(x))
//DEBUGGING MACROS
#define PF(x) printf(#x": %f\n",x)
#define PI(x) printf(#x": %d\n",x)
//PROFILING MACROS
#define TIME_INIT					\
			struct timeval idthen, idnow
#define TIME_START					\
		gettimeofday (&idthen, NULL)
#define TIME_END					\
		gettimeofday (&idnow, NULL);\
		printf ("Time taken by id: %f\n", idnow.tv_sec - idthen.tv_sec + 1e-6 * (idnow.tv_usec - idthen.tv_usec))

//DATA STRUCTURES
typedef struct {
	unsigned int i;
	unsigned int j;
	unsigned char val;
} PXL;

typedef struct {
	unsigned char *r, *g, *b, *edge;
	unsigned int height;
	unsigned int width;
	unsigned int hex_height;
	unsigned int hex_width;
} IMAGE;

//DECLARATIONS
unsigned char * get_neighbors (const unsigned char * img_gray, int p, int q, int hex_width);
int hex_convolute (char * operator, unsigned char * neighbors);
void cpu_convert_to_gray (const unsigned char *red, const unsigned char *green, const unsigned char *blue, const unsigned int height, const unsigned int width, unsigned char **gray);
void cpu_convert_to_hex (const unsigned char *img_gray, const unsigned int height, const unsigned int width, unsigned char **hex_gray, unsigned int *hex_height, unsigned int *hex_width);
void cpu_hex_edge_detect (const unsigned char *hex_gray, const unsigned int hex_height, const unsigned int hex_width, const unsigned int height, const unsigned int threshold, unsigned char **hex_edge);
void cpu_temp_match (IMAGE *im, IMAGE *tmp, PXL *p);
PXL get_max_loc (const unsigned char *img, unsigned int height, unsigned int width);
void result_cord(int p, int q, const unsigned int width, const unsigned int height, const unsigned int hex_width, const unsigned int hex_height, unsigned int *point_x_center, unsigned int *point_y_center);
void prepare_image(IMAGE *im);
