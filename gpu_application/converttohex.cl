__kernel void Transformation(__global char* hex_gray, __global char* img_gray, __global int* ptr_height, __global int* ptr_width, __global int* ptr_hex_height, __global int* ptr_hex_width) {

	int height = *ptr_height, width = *ptr_width;
	int hex_height = *ptr_hex_height, hex_width = *ptr_hex_width;

	// Index of the elements to add
	unsigned int GId = get_global_id(0);

	//store colour values for hex pixels
	int p, q;
	int i,j,k;
	float x_center, y_center;
	int int_x_center, int_y_center;
	float bitmap_x_center, bitmap_y_center;
	unsigned char final_gray;
	unsigned char gray_buff[4];
	float diff;

	//calculate p,q
	p = (GId % hex_width) - hex_width / 2;
	q = (hex_height / 2) - 1 - (GId / hex_width);

	//calculate i,j,k
	i = p + 2 * q;
	j = 2 * p + q + 1;
	k = p - q + 1;

	//calc rectangular coordinates of center
	y_center = i;
	x_center = ((j + k) / 1.732050808);

	//converting them to bitmap coordinates
	bitmap_x_center = (width / 2.0) + x_center;
	bitmap_y_center = ((height / 2.0) - 1) - y_center;

	//if the pixel is a valid bitmap pixel
	if ( bitmap_x_center > 0 && bitmap_y_center > 1 && bitmap_x_center < width && bitmap_y_center < height) {
		int_x_center = bitmap_x_center;
		int_y_center = bitmap_y_center;
		diff = bitmap_x_center - int_x_center;

		//interpolate according to position of center
		if ( diff > (float)0.5 ) {
			//calculate colour values of 4 pixels
			gray_buff[0] = img_gray [int_y_center * width + int_x_center];
			gray_buff[1] = img_gray [(int_y_center - 1) * width + int_x_center];
			gray_buff[2] = img_gray [(int_y_center) * width + (int_x_center + 1)];
			gray_buff[3] = img_gray [(int_y_center - 1) * width + (int_x_center + 1)];

			//merge 4 pixels to 2 pixels
			gray_buff[0] = (gray_buff[0]>>1) + (gray_buff[1]>>1);
			gray_buff[1] = (gray_buff[2]>>1) + (gray_buff[3]>>1);

			//merge 2 pixels to one pixel by linear interpolation
			final_gray = (diff / 2) * gray_buff[1] + ((2 - diff) / 2) * gray_buff[0];
		}
		else {
			//calculate colour values of 4 pixels
			gray_buff[0] = img_gray [int_y_center * width + int_x_center];
			gray_buff[1] = img_gray [(int_y_center - 1) * width + int_x_center];
			gray_buff[2] = img_gray [(int_y_center) * width + (int_x_center - 1)];
			gray_buff[3] = img_gray [(int_y_center - 1) * width + (int_x_center - 1)];

			//merge 4 pixels to 2 pixels
			gray_buff[0] = (gray_buff[0]>>1) + (gray_buff[1]>>1);
			gray_buff[1] = (gray_buff[2]>>1) + (gray_buff[3]>>1);

			//merge 2 pixels to one pixel by linear interpolation
			final_gray = ((1 - diff) / 2) * gray_buff[1] + ((1 + diff) / 2) * gray_buff[0];
		}
	}
	//if pixel isn't a valid bitmap pixel
	else {
		final_gray = 0;
	}

	//assign calculated colour to hex grid
	hex_gray [((hex_height / 2) - 1 - q)*hex_width + (p + hex_width / 2)] = final_gray;
	
}
