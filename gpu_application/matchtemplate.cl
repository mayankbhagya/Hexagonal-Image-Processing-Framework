__kernel void Match(__global unsigned char* image, __global unsigned char* tmplate, __global unsigned int* im_width, __global unsigned int* tm_height, __global unsigned int* tm_width, __global unsigned char *result) {
	unsigned int image_width = *im_width;
	unsigned int template_width = *tm_width;
	unsigned int template_height = *tm_height;
	
	unsigned int GId = get_global_id(0);

	unsigned int result_image_row = GId / (image_width - template_width + 1);
	unsigned int result_image_column = GId % (image_width - template_width + 1);
	
	int i,j;
	unsigned int im_index, tm_index;
	unsigned int tm_white = 0, im_white = 0;
	
	for (i = 0; i < template_height; i++) {
		for (j = 0; j < template_width; j++) {
			im_index = (result_image_row + i) * image_width + (result_image_column + j);
			tm_index = i * template_width + j;
			if (tmplate [tm_index] == 255) {
				++tm_white;
				if (image [im_index] == 255) {
					++im_white;
				}
			}
		}
	}
	
	result [result_image_row * (image_width - template_width + 1) + result_image_column] = (int)(255 * ((float) im_white / tm_white));
		
}
