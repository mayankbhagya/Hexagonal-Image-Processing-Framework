__kernel void GrayscaleConversion(__global unsigned char* gray, __global unsigned char* red, __global unsigned char* green, __global unsigned char* blue) {
	unsigned int GId = get_global_id(0);

	gray[GId] = (float)(0.3 * red[GId]) + (float)(0.59 * green[GId]) + (float)(0.11 * blue[GId]);
}
