/*
 * main.c
 *
 *      Author: bhagya, sanjay
 *      GPU based hexagonal framework
 */
#include "gpu_template_match.h"
/**
 * FUCNTION DEFINITIONS
 */

PXL gpu_get_best_point (GPU_TM tm, GPU_IMAGE source, GPU_IMAGE template) {
	PXL hex, ret;
	/**
	 * TEMPLATE MATCH
	 */
	unsigned int result_ht, result_wd;
	result_ht = (source.hex_height - 2) - (template.hex_height - 2) + 1;
	result_wd = (source.hex_width - 2) - (template.hex_width - 2) + 1;
	unsigned char *result;
	gpu_match_template (tm.sys, tm.matchtemplate, source.edge, template.edge, source.hex_height - 2, source.hex_width - 2, template.hex_height - 2, template.hex_width - 2, (void **)&result, IN_GPU_PTR, RET_CPU_PTR);
	hex = get_max_loc (result, result_ht, result_wd);
	ret.val = hex.val;
	hex_to_rec(hex.j, hex.i, source.width, source.width, source.hex_width, source.hex_height, &ret.j, &ret.i);

	return ret;
}

GPU_TM gpu_template_match_init () {
	/**
	 * INITIALISE SYSTEM
	 */
	//get contexts and command queues
	GPU_TM tm;
	tm.sys = system_init();
	//JIT compilations
	tm.rgbtogray = build_program (tm.sys.context, "converttogray.cl");
	tm.rectohex = build_program (tm.sys.context, "converttohex.cl");
	tm.hexedgedetect = build_program (tm.sys.context, "edgedetect.cl");
	tm.matchtemplate = build_program (tm.sys.context, "matchtemplate.cl");
	return tm;
}

void prepare_image (GPU_TM tm, GPU_IMAGE *input) {
	//grayscale conversion
	cl_mem GPUTmpGray;
	gpu_convert_to_gray (tm.sys, tm.rgbtogray, input->r, input->g, input->b, input->height, input->width, (void **)&GPUTmpGray, IN_CPU_PTR, RET_GPU_PTR);
	//hexagonal conversion
	cl_mem GPUTmpHexGray;
	gpu_convert_to_hex (tm.sys, tm.rectohex, GPUTmpGray, input->height, input->width, (void **)&GPUTmpHexGray, &(input->hex_height), &(input->hex_width), IN_GPU_PTR, RET_GPU_PTR);
	//edge detection
	cl_mem GPUTmpHexEdge;
	gpu_hex_edge_detect (tm.sys, tm.hexedgedetect, GPUTmpHexGray, input->hex_height, input->hex_width, input->height / 2, THRESH, (void **)&GPUTmpHexEdge, IN_GPU_PTR, RET_GPU_PTR);
	input -> edge = GPUTmpHexEdge;
	/**
	 * CLEANING UP
	 */
	clReleaseMemObject(GPUTmpGray);
	clReleaseMemObject(GPUTmpHexGray);
}

SYSTEM system_init () {
	SYSTEM sys;
	cl_int ciErr1;

	// Query platform ID
	cl_platform_id platform;
	cl_uint num_platforms;
	ciErr1 = clGetPlatformIDs (1, &platform, &num_platforms);
	if (ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clGetPlatformIDs\n");
		printCLErr(ciErr1);
	}

	// Setup context properties
	cl_context_properties props[3];
	props[0] = (cl_context_properties)CL_CONTEXT_PLATFORM;
	props[1] = (cl_context_properties)platform;
	props[2] = (cl_context_properties)0;

	// Create a context to run OpenCL on our CUDA-enabled NVIDIA GPU
	cl_context GPUContext = clCreateContextFromType(props, CL_DEVICE_TYPE_GPU, NULL, NULL, &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateContextFromType\n");
		printCLErr(ciErr1);
	}

	// Get the list of GPU devices associated with this context
	size_t ParmDataBytes;
	ciErr1 = clGetContextInfo(GPUContext, CL_CONTEXT_DEVICES, 0, NULL, &ParmDataBytes);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clGetContextInfo\n");
	}
	cl_device_id* GPUDevices = (cl_device_id*)malloc(ParmDataBytes);
	ciErr1 = clGetContextInfo(GPUContext, CL_CONTEXT_DEVICES, ParmDataBytes, GPUDevices, NULL);
	if (ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clGetContextInfo\n");
	}

	// Create a command-queue on the first GPU device
	cl_command_queue GPUCommandQueue = clCreateCommandQueue(GPUContext, GPUDevices[0], 0, &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateCommandQueue\n");
	}

	sys.context = GPUContext;
	sys.queue = GPUCommandQueue;
	return sys;
}

cl_program build_program (cl_context GPUContext, const char * filename) {
	cl_int ciErr1;

	// Create OpenCL program with source code
	char *OpenCLSource = NULL;
	unsigned int SourceLength;
	OpenCLSource = oclLoadProgSource(filename, "", &SourceLength);
	cl_program OpenCLProgram = clCreateProgramWithSource(GPUContext, 1, (const char **) &OpenCLSource, &SourceLength, &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateProgramWithSource\n");
	}

	// Build the program (OpenCL JIT compilation)
	ciErr1 = clBuildProgram(OpenCLProgram, 0, NULL, NULL, NULL, NULL);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clBuildProgram\n");
		printCLErr(ciErr1);
	}

	return OpenCLProgram;
}

void gpu_convert_to_gray (SYSTEM sys, cl_program OpenCLProgram, const void *red, const void *green, const void *blue, const unsigned int height, const unsigned int width, void **gray, enum IN_MODE inmode, enum RET_MODE retmode) {
	cl_int ciErr1, ciErr2;

	cl_context GPUContext = sys.context;
	cl_command_queue GPUCommandQueue = sys.queue;

	unsigned char *local_gray;

	cl_mem GPURed, GPUGreen, GPUBlue;

	if (inmode == IN_CPU_PTR) {
		// Allocate GPU memory for source vectors AND initialize from CPU memory
		GPURed = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, height * width, (unsigned char *)red, &ciErr1);
		ciErr2 = ciErr1;
		GPUGreen = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, height * width, (unsigned char *)green, &ciErr1);
		ciErr2 |= ciErr1;
		GPUBlue = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, height * width, (unsigned char *)blue, &ciErr1);
		ciErr2 |= ciErr1;
		if(ciErr2 != CL_SUCCESS) {
			printf("OpenCL Error: clCreateBuffer\n");
		}
	}
	else {
		GPURed = (cl_mem) red;
		GPUGreen = (cl_mem) green;
		GPUBlue = (cl_mem) blue;
	}


	// Allocate output memory on GPU
	cl_mem GPUGray = clCreateBuffer(GPUContext, CL_MEM_READ_WRITE, height * width, NULL, &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateBuffer\n");
	}

	// Create a handle to the compiled OpenCL function (Kernel)
	cl_kernel Conversion = clCreateKernel(OpenCLProgram, "GrayscaleConversion", &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateKernel\n");
	}


	// In the next step we associate the GPU memory with the Kernel arguments
	ciErr1 = clSetKernelArg(Conversion, 0, sizeof(cl_mem), (void*)&GPUGray);
	ciErr1 |= clSetKernelArg(Conversion, 1, sizeof(cl_mem), (void*)&GPURed);
	ciErr1 |= clSetKernelArg(Conversion, 2, sizeof(cl_mem), (void*)&GPUGreen);
	ciErr1 |= clSetKernelArg(Conversion, 3, sizeof(cl_mem), (void*)&GPUBlue);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clSetKernelArg\n");
	}


	// Launch the Kernel on the GPU
	size_t WorkSize[1] = {(height * width)};
	ciErr1 = clEnqueueNDRangeKernel(GPUCommandQueue, Conversion, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clEnQNDRangeKernel\n");
	}

	// Copy the output back to CPU memory, if required
	if (retmode == RET_CPU_PTR) {
		local_gray = (unsigned char *) malloc (height * width * sizeof (char));
		ciErr1 |= clEnqueueReadBuffer(GPUCommandQueue, GPUGray, CL_TRUE, 0, height * width, (unsigned char *) local_gray, 0, NULL, NULL);
		if(ciErr1 != CL_SUCCESS) {
			printf("OpenCL Error: clEnQReadBuffer\n");
		}
		*gray = local_gray;
		clReleaseMemObject (GPUGray);
	}
	else {
		*gray = GPUGray;
	}

	//Cleaning up
	clReleaseKernel (Conversion);
	if (inmode == IN_CPU_PTR) {
		clReleaseMemObject (GPURed);
		clReleaseMemObject (GPUGreen);
		clReleaseMemObject (GPUBlue);
	}
}

void gpu_convert_to_hex (SYSTEM sys, cl_program OpenCLProgram, const void *gray, const unsigned int height, const unsigned int width, void **hex_gray, unsigned int *hex_height, unsigned int *hex_width, enum IN_MODE inmode, enum RET_MODE retmode) {
	cl_int ciErr1, ciErr2;
	//calculate range of two dimensional hexagonal coordinates

	cl_context GPUContext = sys.context;
	cl_command_queue GPUCommandQueue = sys.queue;

	const float SQRT_3 = sqrt(3);
	int p_min, p_max, q_min, q_max;
	p_min = -(width / (2 * SQRT_3));
	p_max = (width / (2 * SQRT_3));
	q_min = -(width / (4 * SQRT_3) + (height / 4));
	q_max = (width / (4 * SQRT_3) + (height / 4));

	//allocate memory for storing colour values of hex pixels, back on host
	*hex_height = q_max - q_min;
	*hex_width = p_max - p_min;

	cl_mem GPUGray;

	//Taking image from CPU or GPU
	if (inmode == IN_CPU_PTR) {
		GPUGray = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, height * width, (void *)gray, &ciErr1);
		ciErr2 = ciErr1;
		if(ciErr2 != CL_SUCCESS) {
			printf("OpenCL Error: clCreateBuffer\n");
		}
	}
	else {
		GPUGray = (cl_mem) gray;
	}

	//Creating mandatory kernel arguments on GPU
	cl_mem GPUHeight = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&height, &ciErr1);
	ciErr2 = ciErr1;
	cl_mem GPUWidth = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&width, &ciErr1);
	ciErr2 |= ciErr1;
	if(ciErr2 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateBuffer");
	}

	// Allocate output memory on GPU
	cl_mem GPUHexHeight = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), hex_height, &ciErr1);
	ciErr2 = ciErr1;
	cl_mem GPUHexWidth = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), hex_width, &ciErr1);
	ciErr2 |= ciErr1;
	cl_mem GPUHexGray = clCreateBuffer(GPUContext, CL_MEM_WRITE_ONLY, *hex_height * *hex_width, NULL, &ciErr1);
	ciErr2 = ciErr1;
	if(ciErr2 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateBuffer\n");
		printCLErr (ciErr2);
	}

	// Create a handle to the compiled OpenCL function (Kernel)
	cl_kernel Transform = clCreateKernel(OpenCLProgram, "Transformation", &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateKernel\n");
	}

	// In the next step we associate the GPU memory with the Kernel arguments
	ciErr1 = clSetKernelArg(Transform, 0, sizeof(cl_mem), (void*)&GPUHexGray);
	ciErr1 |= clSetKernelArg(Transform, 1, sizeof(cl_mem), (void*)&GPUGray);
	ciErr1 |= clSetKernelArg(Transform, 2, sizeof(cl_mem), (void*)&GPUHeight);
	ciErr1 |= clSetKernelArg(Transform, 3, sizeof(cl_mem), (void*)&GPUWidth);
	ciErr1 |= clSetKernelArg(Transform, 4, sizeof(cl_mem), (void*)&GPUHexHeight);
	ciErr1 |= clSetKernelArg(Transform, 5, sizeof(cl_mem), (void*)&GPUHexWidth);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clSetKernelArgsex\n");
	}

	// Launch the Kernel on the GPU
	size_t WorkSize[1] = {(*hex_height * *hex_width)}; // one dimensional Range
	ciErr1 = clEnqueueNDRangeKernel(GPUCommandQueue, Transform, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clEnQNDRangeKernel\n");
	}

	// Copy the output back to CPU memory, if required
	unsigned char *local_gray;
	if (retmode == RET_CPU_PTR) {
		local_gray = (unsigned char *) malloc ((*hex_height) * (*hex_width) * sizeof (char));

		ciErr1 = clEnqueueReadBuffer(GPUCommandQueue, GPUHexGray, CL_TRUE, 0, *hex_height * *hex_width, local_gray, 0, NULL, NULL);

		if(ciErr1 != CL_SUCCESS) {
			printf("OpenCL Error: clEnQReadBuffer\n");
		}

		*hex_gray = local_gray;
		clReleaseMemObject (GPUHexGray);
	}
	else {
		*hex_gray = GPUHexGray;
	}

	//Cleaning up
	clReleaseKernel (Transform);
	clReleaseMemObject (GPUHeight);
	clReleaseMemObject (GPUWidth);
	if (inmode == IN_CPU_PTR) {
		clReleaseMemObject (GPUGray);
	}
	clReleaseMemObject (GPUHexHeight);
	clReleaseMemObject (GPUHexWidth);


}

void gpu_hex_edge_detect (SYSTEM sys, cl_program OpenCLProgram, const void *hex_gray, const unsigned int hex_height, const unsigned int hex_width,  const unsigned int half_org_height, const unsigned int threshold, void **hex_edge, enum IN_MODE inmode, enum RET_MODE retmode) {
	cl_int ciErr1, ciErr2;
	unsigned char *local_edge;
	cl_mem GPUHexGray;

	cl_context GPUContext = sys.context;
	cl_command_queue GPUCommandQueue = sys.queue;

	//Taking image from CPU or GPU
	if (inmode == IN_CPU_PTR) {
		GPUHexGray = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, hex_height * hex_width, (unsigned char *)hex_gray, &ciErr1);
		if(ciErr1 != CL_SUCCESS) {
			printf("OpenCL Error: clCreateBuffer\n");
			printCLErr (ciErr1);
		}
	}
	else {
		GPUHexGray = (cl_mem) hex_gray;
	}

	//Creating mandatory kernel arguments on GPU
	cl_mem GPUThreshold = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&threshold, &ciErr1);
	ciErr2 = ciErr1;
	cl_mem GPUHexWidth = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&hex_width, &ciErr1);
	ciErr2 |= ciErr1;
	cl_mem GPUHexHeight = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&hex_height, &ciErr1);
	ciErr2 |= ciErr1;
	cl_mem GPUHalfOrgHeight = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&half_org_height, &ciErr1);
	ciErr2 |= ciErr1;
	if(ciErr2 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateBuffer");
		printCLErr (ciErr2);
	}

	// Allocate output memory on GPU
	cl_mem GPUHexEdge = clCreateBuffer(GPUContext, CL_MEM_WRITE_ONLY, (hex_height - 2) * (hex_width - 2), NULL, &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateBuffer\n");
		printCLErr (ciErr1);
	}

	// Create a handle to the compiled OpenCL function (Kernel)
	cl_kernel EdgeDetect = clCreateKernel(OpenCLProgram, "EdgeDetection", &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateKernel");
	}

	ciErr1 = clSetKernelArg(EdgeDetect, 0, sizeof(cl_mem), (void*)&GPUThreshold);
	ciErr1 |= clSetKernelArg(EdgeDetect, 1, sizeof(cl_mem), (void*)&GPUHexWidth);
	ciErr1 |= clSetKernelArg(EdgeDetect, 2, sizeof(cl_mem), (void*)&GPUHexGray);
	ciErr1 |= clSetKernelArg(EdgeDetect, 3, sizeof(cl_mem), (void*)&GPUHexEdge);
	ciErr1 |= clSetKernelArg(EdgeDetect, 4, sizeof(cl_mem), (void*)&GPUHexHeight);
	ciErr1 |= clSetKernelArg(EdgeDetect, 5, sizeof(cl_mem), (void*)&GPUHalfOrgHeight);

	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clSetKernelArg");
	}

	// Launch the Kernel on the GPU
	size_t EdgeImageSize[1] = {(hex_height - 2) * (hex_width - 2)}; // one dimensional Range
	ciErr1 = clEnqueueNDRangeKernel(GPUCommandQueue, EdgeDetect, 1, NULL, EdgeImageSize, NULL, 0, NULL, NULL);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clEnQNDRangeKernel: ");
		printCLErr (ciErr1);
	}

	// Copy the output back to CPU memory, if required
	if (retmode == RET_CPU_PTR) {
		local_edge = (unsigned char *) malloc ((hex_height - 2) * (hex_width - 2) * sizeof (char));
		ciErr1 = clEnqueueReadBuffer(GPUCommandQueue, GPUHexEdge, CL_TRUE, 0, (hex_height - 2) * (hex_width - 2), local_edge, 0, NULL, NULL);
		if(ciErr1 != CL_SUCCESS) {
			printf("OpenCL Error: clEnQReadBuffer: ");
			printCLErr (ciErr1);
		}
		*hex_edge = local_edge;
		clReleaseMemObject (GPUHexEdge);
	}
	else {
		*hex_edge = GPUHexEdge;
	}
	//Cleaning up
	clReleaseKernel (EdgeDetect);
	if (inmode == IN_CPU_PTR) {
		clReleaseMemObject (GPUHexGray);
	}
	clReleaseMemObject (GPUThreshold);
	clReleaseMemObject (GPUHexWidth);
}

void gpu_match_template (SYSTEM sys, cl_program OpenCLProgram, const void *image, const void *template, const unsigned int image_height, const unsigned int image_width, const unsigned int template_height, const unsigned int template_width, void **result, enum IN_MODE inmode, enum RET_MODE retmode) {
	cl_int ciErr1, ciErr2;
	unsigned char *local_res;

	cl_context GPUContext = sys.context;
	cl_command_queue GPUCommandQueue = sys.queue;

	cl_mem GPUImage, GPUTemplate;
	//Taking image from CPU or GPU
	if (inmode == IN_CPU_PTR) {
		GPUImage = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image_height * image_width, (unsigned char *)image, &ciErr1);
		ciErr2 = ciErr1;
		GPUTemplate = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, template_height * template_width, (unsigned char *)template, &ciErr1);
		ciErr2 |= ciErr1;
		if(ciErr2 != CL_SUCCESS) {
			printf("OpenCL Error: clCreateBuffer\n");
		}
	}
	else {
		GPUImage = (cl_mem) image;
		GPUTemplate = (cl_mem) template;
	}

	//Creating mandatory kernel arguments on GPU
	cl_mem GPUImageWidth = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&image_width, &ciErr1);
	ciErr2 = ciErr1;
	cl_mem GPUTemplateWidth = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&template_width, &ciErr1);
	ciErr2 |= ciErr1;
	cl_mem GPUTemplateHeight = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof (int), (void *)&template_height, &ciErr1);
	ciErr2 |= ciErr1;
	if(ciErr2 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateBuffer");
	}

	// Allocate output memory on GPU
	cl_mem GPUResult = clCreateBuffer(GPUContext, CL_MEM_WRITE_ONLY, (image_height - template_height + 1) * (image_width - template_width + 1), NULL, &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateBuffer\n");
	}

	// Create a handle to the compiled OpenCL function (Kernel)
	cl_kernel MatchTemplate = clCreateKernel(OpenCLProgram, "Match", &ciErr1);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clCreateKernel");
	}

	ciErr1 = clSetKernelArg(MatchTemplate, 0, sizeof(cl_mem), (void*)&GPUImage);
	ciErr1 |= clSetKernelArg(MatchTemplate, 1, sizeof(cl_mem), (void*)&GPUTemplate);
	ciErr1 |= clSetKernelArg(MatchTemplate, 2, sizeof(cl_mem), (void*)&GPUImageWidth);
	ciErr1 |= clSetKernelArg(MatchTemplate, 3, sizeof(cl_mem), (void*)&GPUTemplateHeight);
	ciErr1 |= clSetKernelArg(MatchTemplate, 4, sizeof(cl_mem), (void*)&GPUTemplateWidth);
	ciErr1 |= clSetKernelArg(MatchTemplate, 5, sizeof(cl_mem), (void*)&GPUResult);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clSetKernelArg");
	}

	// Launch the Kernel on the GPU
	size_t Workgroup[1] = {(image_height - template_height + 1) * (image_width - template_width + 1)}; // one dimensional Range
	ciErr1 = clEnqueueNDRangeKernel(GPUCommandQueue, MatchTemplate, 1, NULL, Workgroup, NULL, 0, NULL, NULL);
	if(ciErr1 != CL_SUCCESS) {
		printf("OpenCL Error: clEnQNDRangeKernel: ");
		printCLErr (ciErr1);
	}

	// Copy the output back to CPU memory, if required
	if (retmode == RET_CPU_PTR) {
		local_res = (unsigned char *) malloc ((image_height - template_height + 1) * (image_width - template_width + 1) * sizeof (char));
		ciErr1 = clEnqueueReadBuffer(GPUCommandQueue, GPUResult, CL_TRUE, 0, (image_height - template_height + 1) * (image_width - template_width + 1), local_res, 0, NULL, NULL);
		if(ciErr1 != CL_SUCCESS) {
			printf("OpenCL Error: clEnQReadBuffer: ");
			printCLErr (ciErr1);
		}
		*result = local_res;
		clReleaseMemObject (GPUResult);
	}
	else {
		*result = GPUResult;
	}
	//Cleaning up
	if (inmode == IN_CPU_PTR) {
		clReleaseKernel (MatchTemplate);
		clReleaseMemObject (GPUImage);
		clReleaseMemObject (GPUTemplate);
	}
	clReleaseMemObject (GPUImageWidth);
	clReleaseMemObject (GPUTemplateWidth);
	clReleaseMemObject (GPUTemplateHeight);

}

char* oclLoadProgSource(const char* cFilename, const char* cPreamble, size_t* szFinalLength) {
    // locals
    FILE* pFileStream = NULL;
    size_t szSourceLength;

    // open the OpenCL source code file

	pFileStream = fopen(cFilename, "rb");
	if(pFileStream == 0)
	{
		return NULL;
	}

    size_t szPreambleLength = strlen(cPreamble);

    // get the length of the source code
    fseek(pFileStream, 0, SEEK_END);
    szSourceLength = ftell(pFileStream);
    fseek(pFileStream, 0, SEEK_SET);

    // allocate a buffer for the source code string and read it in
    char* cSourceString = (char *)malloc(szSourceLength + szPreambleLength + 1);
    memcpy(cSourceString, cPreamble, szPreambleLength);
    if (fread((cSourceString) + szPreambleLength, szSourceLength, 1, pFileStream) != 1)
    {
        fclose(pFileStream);
        free(cSourceString);
        return 0;
    }

    // close the file and return the total length of the combined (preamble + source) string
    fclose(pFileStream);
    if(szFinalLength != 0)
    {
        *szFinalLength = szSourceLength + szPreambleLength;
    }
    cSourceString[szSourceLength + szPreambleLength] = '\0';

    return cSourceString;
}

void printCLErr(cl_int err)
{
        switch (err)
        {
                case CL_SUCCESS:
                        printf("CL_SUCCESS\n");
                        break;
                case CL_DEVICE_NOT_FOUND:
                        printf("CL_DEVICE_NOT_FOUND\n");
                        break;
                case CL_DEVICE_NOT_AVAILABLE:
                        printf("CL_DEVICE_NOT_AVAILABLE\n");
                        break;
                case CL_COMPILER_NOT_AVAILABLE:
                        printf("CL_COMPILER_NOT_AVAILABLE\n");
                        break;
                case CL_MEM_OBJECT_ALLOCATION_FAILURE:
                        printf("CL_MEM_OBJECT_ALLOCATION_FAILURE\n");
                        break;
                case CL_OUT_OF_RESOURCES:
                        printf("CL_OUT_OF_RESOURCES\n");
                        break;
                case CL_OUT_OF_HOST_MEMORY:
                        printf("CL_OUT_OF_HOST_MEMORY\n");
                        break;
                case CL_PROFILING_INFO_NOT_AVAILABLE:
                        printf("CL_PROFILING_INFO_NOT_AVAILABLE\n");
                        break;
                case CL_MEM_COPY_OVERLAP:
                        printf("CL_MEM_COPY_OVERLAP\n");
                        break;
                case CL_IMAGE_FORMAT_MISMATCH:
                        printf("CL_IMAGE_FORMAT_MISMATCH\n");
                        break;
                case CL_IMAGE_FORMAT_NOT_SUPPORTED:
                        printf("CL_IMAGE_FORMAT_NOT_SUPPORTED\n");
                        break;
                case CL_BUILD_PROGRAM_FAILURE:
                        printf("CL_BUILD_PROGRAM_FAILURE\n");
                        break;
                case CL_MAP_FAILURE:
                        printf("CL_MAP_FAILURE\n");
                        break;
                case CL_INVALID_VALUE:
                        printf("CL_INVALID_VALUE\n");
                        break;
                case CL_INVALID_DEVICE_TYPE:
                        printf("CL_INVALID_DEVICE_TYPE\n");
                        break;
                case CL_INVALID_PLATFORM:
                        printf("CL_INVALID_PLATFORM\n");
                        break;
                case CL_INVALID_DEVICE:
                        printf("CL_INVALID_DEVICE\n");
                        break;
                case CL_INVALID_CONTEXT:
                        printf("CL_INVALID_CONTEXT\n");
                        break;
                case CL_INVALID_QUEUE_PROPERTIES:
                        printf("CL_INVALID_QUEUE_PROPERTIES\n");

                        break;
                case CL_INVALID_COMMAND_QUEUE:
                        printf("CL_INVALID_COMMAND_QUEUE\n");
                        break;
                case CL_INVALID_HOST_PTR:
                        printf("CL_INVALID_HOST_PTR\n");
                        break;
                case CL_INVALID_MEM_OBJECT:
                        printf("CL_INVALID_MEM_OBJECT\n");
                        break;
                case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
                        printf("CL_INVALID_IMAGE_FORMAT_DESCRIPTOR\n");
                        break;
                case CL_INVALID_IMAGE_SIZE:
                        printf("CL_INVALID_IMAGE_SIZE\n");
                        break;
                case CL_INVALID_SAMPLER:
                        printf("CL_INVALID_SAMPLER\n");
                        break;
                case CL_INVALID_BINARY:
                        printf("CL_INVALID_BINARY\n");
                        break;
                case CL_INVALID_BUILD_OPTIONS:
                        printf("CL_INVALID_BUILD_OPTIONS\n");
                        break;
                case CL_INVALID_PROGRAM:
                        printf("CL_INVALID_PROGRAM\n");
                        break;
                case CL_INVALID_PROGRAM_EXECUTABLE:
                        printf("CL_INVALID_PROGRAM_EXECUTABLE\n");
                        break;
                case CL_INVALID_KERNEL_NAME:
                        printf("CL_INVALID_KERNEL_NAME\n");
                        break;
                case CL_INVALID_KERNEL_DEFINITION:
                        printf("CL_INVALID_KERNEL_DEFINITION\n");
                        break;
                case CL_INVALID_KERNEL:
                        printf("CL_INVALID_KERNEL\n");
                        break;
                case CL_INVALID_ARG_INDEX:
                        printf("CL_INVALID_ARG_INDEX\n");
                        break;
                case CL_INVALID_ARG_VALUE:
                        printf("CL_INVALID_ARG_VALUE\n");
                        break;
                case CL_INVALID_ARG_SIZE:
                        printf("CL_INVALID_ARG_SIZE\n");
                        break;
                case CL_INVALID_KERNEL_ARGS:
                        printf("CL_INVALID_KERNEL_ARGS\n");
                        break;
                case CL_INVALID_WORK_DIMENSION:
                        printf("CL_INVALID_WORK_DIMENSION\n");
                        break;
                case CL_INVALID_WORK_GROUP_SIZE:
                        printf("CL_INVALID_WORK_GROUP_SIZE\n");
                        break;
                case CL_INVALID_WORK_ITEM_SIZE:
                        printf("CL_INVALID_WORK_ITEM_SIZE\n");
                        break;
                case CL_INVALID_GLOBAL_OFFSET:
                        printf("CL_INVALID_GLOBAL_OFFSET\n");
                        break;
                case CL_INVALID_EVENT_WAIT_LIST:
                        printf("CL_INVALID_EVENT_WAIT_LIST\n");
                        break;
                case CL_INVALID_EVENT:
                        printf("CL_INVALID_EVENT\n");
                        break;
                case CL_INVALID_OPERATION:
                        printf("CL_INVALID_OPERATION\n");
                        break;
                case CL_INVALID_GL_OBJECT:
                        printf("CL_INVALID_GL_OBJECT\n");
                        break;
                case CL_INVALID_BUFFER_SIZE:
                        printf("CL_INVALID_BUFFER_SIZE\n");
                        break;
                case CL_INVALID_MIP_LEVEL:
                        printf("CL_INVALID_MIP_LEVEL\n");
                        break;
                case CL_INVALID_GLOBAL_WORK_SIZE:
                        printf("CL_INVALID_GLOBAL_WORK_SIZE\n");

                        break;
        }
}

void hex_to_rec(int p, int q, const unsigned int width, const unsigned int height, const unsigned int hex_width, const unsigned int hex_height,unsigned int *point_x_center, unsigned int *point_y_center) {

	const float SQRT_3 = 1.732050808;
	int i,j,k;
	double x_center, y_center;
	int int_x_center, int_y_center;
	float bitmap_x_center, bitmap_y_center;

	p = p + 1 - hex_width / 2;
	q = hex_height / 2 - q;

	i = p + 2 * q;
	j = 2 * p + q + 1;
	k = p - q + 1;

	//calc rectangular coordinates of center
	y_center = i;
	x_center = ((j + k) / SQRT_3);

	//converting them to bitmap coordinates
	bitmap_x_center = (width / 2.0) + x_center;
	bitmap_y_center = ((height / 2.0) - 1) - y_center;
	int_x_center = bitmap_x_center;
	int_y_center = bitmap_y_center;

	*point_x_center =  int_x_center;
	*point_y_center =  int_y_center;
}

PXL get_max_loc (const unsigned char *img, unsigned int height, unsigned int width) {
	PXL result;
	unsigned register int i, j, idx;
	result.i = 0;
	result.j = 0;
	result.val = 0;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			idx = i * width + j;
			if (img [idx] > result.val ) {
				result.i = i;
				result.j = j;
				result.val = img [idx];
			}
		}
	}
	return result;
}
