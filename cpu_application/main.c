/*
 * main.c
 *
 *  Created on: 19-Apr-2011
 *      Author: bhagya
 */

#include <stdio.h>
#include <pthread.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <unistd.h>
#include "libbmp.h"
#include "cpu_template_match.h"

//Configuration
#define MATCH_PER 70
#define FRAME_PREFIX "animation/frames"
#define FRAME_START 120
#define FRAME_END 600
#define FPS 25
#define TEMPLATE "i8.bmp"

//Data structures
typedef struct {
	unsigned char *r, *g, *b;
} FRAME;

//Declarations
void createFrameFromCV (IplImage *opencv_frame, FRAME *rgb_frame);
unsigned int loadFrames (const char *name, unsigned int start, unsigned int end, IplImage **opencv_frames, FRAME *rgb_frames);
void * displayController (void *);
void * frameProcessor (void *);

//metadata for running video
int loadedFrames = -1;
int currentFrame = -1;
int frame_height, frame_width;
int drawRect = 0;
int rect_i = 0, rect_j = 0;
int rect_ht = 0, rect_wd = 0;

IMAGE template;
//BEGIN
int main () {

	pthread_t tid0, tid1;
	IplImage **opencv_frames;
	FRAME *rgb_frames;

	//Prepare templates
	printf ("Loading template...\n");
	FILE *template_fp = fopen (TEMPLATE, "r");
	readBMP (template_fp, &template.r, &template.g, &template.b, &template.height, &template.width);
	prepare_image (&template);

	//Loading frames
	opencv_frames = (IplImage **) malloc (sizeof (IplImage *) * (FRAME_END - FRAME_START + 1));
	rgb_frames = (FRAME *) malloc (sizeof (FRAME) * (FRAME_END - FRAME_START + 1));
	printf ("Loading frames into memory...\n");
	loadedFrames = loadFrames (FRAME_PREFIX, FRAME_START, FRAME_END, opencv_frames, rgb_frames);
	printf ("%d frames loaded!\n",loadedFrames);

	//Start processing video
	pthread_create(&tid0, NULL, displayController, (void *)opencv_frames);
	usleep(1000*1000);
	pthread_create(&tid1, NULL, frameProcessor, rgb_frames);

	pthread_join (tid0, NULL);
	pthread_join (tid1, NULL);

	return 0;
}

void createFrameFromCV (IplImage *opencv_frame, FRAME *rgb_frame) {
	rgb_frame->r = (unsigned char *) malloc (opencv_frame->height * opencv_frame->width);
	rgb_frame->g = (unsigned char *) malloc (opencv_frame->height * opencv_frame->width);
	rgb_frame->b = (unsigned char *) malloc (opencv_frame->height * opencv_frame->width);

	unsigned register int i, j;
	for (i = 0; i < opencv_frame->height; i++) {
		for (j = 0; j < opencv_frame->width; j++) {
			rgb_frame->b[i * opencv_frame->width + j] = opencv_frame->imageData[i * opencv_frame->widthStep + opencv_frame->nChannels * j];
			rgb_frame->g[i * opencv_frame->width + j] = opencv_frame->imageData[i * opencv_frame->widthStep + opencv_frame->nChannels * j + 1];
			rgb_frame->r[i * opencv_frame->width + j] = opencv_frame->imageData[i * opencv_frame->widthStep + opencv_frame->nChannels * j + 2];
		}
	}
}

unsigned int loadFrames (const char *name, unsigned int start, unsigned int end, IplImage **opencv_frames, FRAME *rgb_frames) {
	char file_name[256];
	unsigned int count;
	register unsigned int i;

	if (!(start <= end)) {
		return 0;
	}

	count = 0;
	for (i = start; i <= end; i++) {
		sprintf (file_name, "%s%d.jpg", name, i);
		opencv_frames[i - start] = cvLoadImage (file_name, CV_LOAD_IMAGE_COLOR);
		if (!opencv_frames[i - start]) {
			break;
		}
		if (!(i - start)) {
			frame_height = opencv_frames[0]->height;
			frame_width = opencv_frames[0]->width;
		}
		createFrameFromCV (opencv_frames[i - start], &rgb_frames[i - start]);
		++count;
	}
	return count;
}

void *displayController (void *opencv_frames_ptr) {
	cvNamedWindow("Video", CV_WINDOW_AUTOSIZE);
	IplImage **opencv_frames;
	opencv_frames = (IplImage **) opencv_frames_ptr;
	int i = 0;
	for (i = 0; i < loadedFrames; i ++) {
		currentFrame = i;
		if (drawRect) {
			cvRectangle (opencv_frames[i], cvPoint (rect_j, rect_i), cvPoint(rect_j + rect_wd - 1, rect_i + rect_ht - 1), cvScalar (0,255,0,0), 2, 8, 0);
		}
		cvShowImage("Video", opencv_frames[i]);
		cvWaitKey(1000 / FPS);
	}
	return NULL;
}

void *frameProcessor (void *rgb_frames_ptr) {
	FRAME *rgb_frames = (FRAME *)rgb_frames_ptr;
	unsigned int skip = 0, localCurrFrame;
	unsigned int prev_frame_id = 0, frame_id = 0;

	TIME_INIT;

	PXL result;
	IMAGE source;
	source.height = frame_height;
	source.width = frame_width;
	rect_ht = template.height;
	rect_wd = template.width;
	while ( (localCurrFrame = currentFrame)  < loadedFrames - 1) {
		TIME_START;
		skip += localCurrFrame - prev_frame_id;
		frame_id = localCurrFrame + skip;
		printf ("Current: %d Processing: %d Skip: %d\n", localCurrFrame, frame_id, skip);
		/*******************************/
		source.r = rgb_frames [frame_id].r;
		source.g = rgb_frames [frame_id].g;
		source.b = rgb_frames [frame_id].b;
		prepare_image(&source);
		cpu_temp_match (&source, &template, &result);
		if (result.val >= (MATCH_PER / 100.0) * 255) {
			rect_i = result.i;
			rect_j = result.j;
			drawRect = 1;
		}
		else {
			drawRect = 0;
		}
		free (source.edge);
		TIME_END;
		/*******************************/
		prev_frame_id = frame_id;
	}

	return NULL;
}

