
#include "cpu_template_match.h"
/**
 * FUCNTION DEFINITIONS
 */

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

void cpu_convert_to_gray (const unsigned char *red, const unsigned char *green, const unsigned char *blue, const unsigned int height, const unsigned int width, unsigned char **gray) {
	register int i, j, index;
	unsigned int buf;
	unsigned char *buffer;
	buffer = malloc (height * width);
	for (i = 0; i < height; i++) {
		for ( j = 0; j < width; j++) {
			index = i * width + j;
			buf = (float)(0.3 * red [index]) + (float)(0.59 * green [index]) + (float)(0.11 * blue [index]);
			buffer [index] = buf;
		}
	}
	*gray = buffer;
}

void cpu_convert_to_hex (const unsigned char *img_gray, const unsigned int height, const unsigned int width, unsigned char **ptr_hex_gray, unsigned int *ptr_hex_height, unsigned int *ptr_hex_width) {
	const float SQRT_3 = 1.732050808;

	//calculate range of two dimensional hexagonal coordinates
	int p_min, p_max, q_min, q_max;
	p_min = -(width / (2 * SQRT_3));
	p_max = (width / (2 * SQRT_3));
	q_min = -(width / (4 * SQRT_3) + (height / 4));
	q_max = (width / (4 * SQRT_3) + (height / 4));

	int hex_height = q_max - q_min;
	int hex_width = p_max - p_min;
	*ptr_hex_height = hex_height;
	*ptr_hex_width = hex_width;

	//allocate memory for storing colour values of hex pixels
	unsigned char *hex_gray = malloc (hex_height * hex_width);

	*ptr_hex_gray = hex_gray;

	//store colour values for hex pixels
	register int p,q;
	int i,j,k;
	double x_center, y_center;
	int int_x_center, int_y_center;
	float bitmap_x_center, bitmap_y_center;
	unsigned char final_gray;
	unsigned char gray_buff[4];
	double diff;

	for (q = - hex_height / 2; q < hex_height / 2; q++) {
		for (p = - hex_width / 2; p < hex_width / 2; p++) {

			//calculate i,j,k
			i = p + 2 * q;
			j = 2 * p + q + 1;
			k = p - q + 1;

			//calc rectangular coordinates of center
			y_center = i;
			x_center = ((j + k) / SQRT_3);

			//converting them to bitmap coordinates
			bitmap_x_center = (width / 2.0) + x_center;
			bitmap_y_center = ((height / 2.0) - 1) - y_center;

			//if the pixel is a valid bitmap pixel
			if ( bitmap_x_center > 0 && bitmap_y_center > 1 && bitmap_x_center < width && bitmap_y_center < height) {
				int_x_center = bitmap_x_center;
				int_y_center = bitmap_y_center;
				diff = bitmap_x_center - (double)int_x_center;

				//interpolate according to position of center
				if ( diff > (float)0.5 ) {
					//calculate colour values of 4 pixels
					gray_buff[0] = img_gray [INDEX(bitmap_x_center, bitmap_y_center, width)];
					gray_buff[1] = img_gray [INDEX(bitmap_x_center, bitmap_y_center - 1, width)];
					gray_buff[2] = img_gray [INDEX(bitmap_x_center + 1, bitmap_y_center, width)];
					gray_buff[3] = img_gray [INDEX(bitmap_x_center + 1, bitmap_y_center - 1, width)];

					//merge 4 pixels to 2 pixels
					gray_buff[0] = (gray_buff[0]>>1) + (gray_buff[1]>>1);
					gray_buff[1] = (gray_buff[2]>>1) + (gray_buff[3]>>1);

					//merge 2 pixels to one pixel by linear interpolation
					final_gray = (diff / 2) * gray_buff[1] + ((2 - diff) / 2) * gray_buff[0];
				}
				else {
					//calculate colour values of 4 pixels
					gray_buff[0] = img_gray [INDEX(bitmap_x_center, bitmap_y_center, width)];
					gray_buff[1] = img_gray [INDEX(bitmap_x_center, bitmap_y_center - 1, width)];
					gray_buff[2] = img_gray [INDEX(bitmap_x_center - 1, bitmap_y_center, width)];
					gray_buff[3] = img_gray [INDEX(bitmap_x_center - 1, bitmap_y_center - 1, width)];

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
			hex_gray [INDEX(p + hex_width / 2, ((hex_height / 2) - 1) - q , hex_width)] = final_gray;
		}
	}

}

void cpu_hex_edge_detect (const unsigned char *hex_gray, const unsigned int hex_height, const unsigned int hex_width, const unsigned int half_org_height, const unsigned int threshold, unsigned char **hex_edge) {
	register int p, q;
	unsigned char * neighbors;
	unsigned char * img_edge = (unsigned char *) malloc (sizeof(unsigned char) * (hex_height - 2) * (hex_width - 2));
	int h1, h2, h3, r;

	char k1[] = {0, -1, +1, -1, +1, 0};
	char k2[] = {+1, +1, 0, 0, -1, -1};
	for (q = 1; q < hex_height - 1; q++) {
		for (p = 1; p < hex_width - 1; p++) {
			neighbors = get_neighbors (hex_gray, p, q, hex_width);
			h1 = hex_convolute (k1, neighbors) / 2;
			h2 = hex_convolute (k2, neighbors) / 2;
			h3 = h1 + h2;
			r = h1*h1 + h2*h2 + h3*h3 - h2*h3 + 1.732050808 * h1 * (h2 - h3);
			if (r < threshold) {
				img_edge [INDEX (p - 1, q - 1, hex_width - 2)] = 0;
			}
			else {
				img_edge [INDEX (p - 1, q - 1, hex_width - 2)] = 255;
			}
			if((q >= (signed int)(half_org_height - 2) && p <= (signed int)((hex_width*(q - half_org_height + 2) / (hex_height - half_org_height )) + 8)) || (q <= (signed int)(hex_height - half_org_height ) && p >= (signed int)((hex_width*q/(hex_height - half_org_height)) - 4) )) {
						img_edge [INDEX (p - 1, q - 1, hex_width - 2)] = 0;
					}
		}
	}

	*hex_edge = img_edge;
}

void cpu_temp_match (IMAGE *imge, IMAGE *tmplt, PXL *pp){
	register int p, q, i, j;
	IMAGE img = *imge, temp = *tmplt;
	PXL pix = *pp;
	unsigned int height = img.hex_height - 2, width = img.hex_width - 2, temp_height = temp.hex_height - 2, temp_width = temp.hex_width - 2;
	unsigned char * img_result = (unsigned char *) malloc (sizeof(unsigned char) * (height - temp_height + 1) * (width - temp_width +1));
	int r, m;

		for (q = 0; q < height - temp_height + 1; q++) {
			for (p = 0; p <  width - temp_width + 1; p++) {
				r = 0;
				m = 0;
				for (i = 0; i < temp_height; i++){
					for(j = 0; j < temp_width; j++){
						if(temp.edge [INDEX (j, i, temp_width)] == 255){
							m++;
							if(img.edge [INDEX (j + p, i + q, width)] == 255) {
									r++;
							}
						}

					}
				}

				r = (int)(255 * ((double)r / m));
				img_result [INDEX (p, q, width - temp_width + 1)] = r;

			}
		}
		unsigned int result_ht = height - temp_height + 1;
		unsigned int result_wd = width - temp_width + 1;
//		writeBMP (RESULT, img_result, img_result, img_result, result_ht, result_wd);
		pix = get_max_loc(img_result, result_ht, result_wd);
		*imge = img;
		*tmplt = temp;

		PXL result_cod;
		result_cord(pix.j, pix.i, img.width,  img.height, img.hex_width,  img.hex_height, &result_cod.j, &result_cod.i);
		result_cod.val = pix.val;
		*pp = result_cod;
		free (img_result);
}

unsigned char * get_neighbors (const unsigned char * img_gray, int p, int q, int hex_width) {
	unsigned char * neighbors;

	//allocating memory for six neighbors
	neighbors = (unsigned char *) malloc (sizeof (unsigned char) * 6);

	neighbors[0] = img_gray [INDEX (p, q + 1, hex_width)];
	neighbors[1] = img_gray [INDEX (p - 1, q + 1, hex_width)];
	neighbors[2] = img_gray [INDEX (p + 1, q, hex_width)];
	neighbors[3] = img_gray [INDEX (p - 1, q, hex_width)];
	neighbors[4] = img_gray [INDEX (p + 1, q - 1, hex_width)];
	neighbors[5] = img_gray [INDEX (p, q - 1, hex_width)];

	return neighbors;
}

int hex_convolute (char * operator, unsigned char * neighbors) {
	int i, ans = 0;
	for (i = 0; i < 6; i++) {
		ans = ans + (operator[i] * neighbors[i]);
	}
	return abs (ans);
}

void result_cord(int p, int q, const unsigned int width, const unsigned int height, const unsigned int hex_width, const unsigned int hex_height,unsigned int *point_x_center, unsigned int *point_y_center){

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

void prepare_image(IMAGE *img){
	IMAGE im = *img;

		//grayscale conversion
		unsigned char *gray;
		cpu_convert_to_gray (im.r, im.g, im.b, im.height, im.width, &gray);
		//hexagonal conversion
		unsigned char *hex_gray;
		cpu_convert_to_hex (gray, im.height, im.width, &hex_gray, &im.hex_height, &im.hex_width);
		//edge detection
		cpu_hex_edge_detect (hex_gray, im.hex_height, im.hex_width, im.height / 2, THRESH, &im.edge);

		*img = im;

		free (gray);
		free (hex_gray);
}
