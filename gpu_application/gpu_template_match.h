/*
 * gpu_template_match.h
 *
 *  Created on: 19-Apr-2011
 *      Author: bhagya
 */

#ifndef GPU_TEMPLATE_MATCH_H_
#define GPU_TEMPLATE_MATCH_H_

#include <stdlib.h>
#include <CL/cl.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

//PROFILING MACROS
#define TIME_INIT				\
		struct timeval idthen, idnow
#define TIME_START					\
		gettimeofday (&idthen, NULL)
#define TIME_END					\
		gettimeofday (&idnow, NULL);\
		printf ("Time taken by id: %f\n", idnow.tv_sec - idthen.tv_sec + 1e-6 * (idnow.tv_usec - idthen.tv_usec))

//PARAMETERS
#define THRESH 5000

//DATA STRUCTURES
enum RET_MODE {RET_GPU_PTR, RET_CPU_PTR};
enum IN_MODE {IN_GPU_PTR, IN_CPU_PTR};

typedef struct  {
	cl_context context;
	cl_command_queue queue;
} SYSTEM;

typedef struct {
	SYSTEM sys;
	cl_program rgbtogray, rectohex, hexedgedetect, matchtemplate;
} GPU_TM;

typedef struct {
	unsigned int i;
	unsigned int j;
	unsigned char val;
} PXL;

typedef struct {
	unsigned char *r, *g, *b;
	cl_mem edge;
	unsigned int height, width;
	unsigned int hex_height, hex_width;
} GPU_IMAGE;

/**
 * FUNCTION DECLARATIONS
 */
SYSTEM system_init ();
char* oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t* szFinalLength);
cl_program build_program (cl_context OpenCLProgram, const char * filename);
GPU_TM gpu_template_match_init ();

void gpu_convert_to_gray (SYSTEM sys, cl_program OpenCLProgram, const void *red, const void *green, const void *blue, const unsigned int height, const unsigned int width, void **gray, enum IN_MODE inmode, enum RET_MODE retmode);
void gpu_convert_to_hex (SYSTEM sys, cl_program OpenCLProgram, const void *img_gray, const unsigned int height, const unsigned int width, void **hex_gray, unsigned int *hex_height, unsigned int *hex_width, enum IN_MODE inmode, enum RET_MODE retmode);
void gpu_hex_edge_detect (SYSTEM sys, cl_program OpenCLProgram, const void *hex_gray, const unsigned int hex_height, const unsigned int hex_width,  const unsigned int half_org_height, const unsigned int threshold, void **hex_edge, enum IN_MODE inmode, enum RET_MODE retmode);
void prepare_image (GPU_TM tm, GPU_IMAGE *input);

void gpu_match_template (SYSTEM sys, cl_program OpenCLProgram, const void *image, const void *template, const unsigned int image_height, const unsigned int image_width, const unsigned int template_height, const unsigned int template_width, void **result, enum IN_MODE inmode, enum RET_MODE retmode);
PXL get_max_loc (const unsigned char *img, unsigned int height, unsigned int width);
void hex_to_rec(int p, int q, const unsigned int width, const unsigned int height, const unsigned int hex_width, const unsigned int hex_height,unsigned int *point_x_center, unsigned int *point_y_center);
PXL gpu_get_best_point (GPU_TM tm, GPU_IMAGE source, GPU_IMAGE template);

void printCLErr(cl_int err);


#endif /* GPU_TEMPLATE_MATCH_H_ */
