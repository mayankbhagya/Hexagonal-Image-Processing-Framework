__kernel void EdgeDetection(__global int* ptr_threshold, __global int* ptr_hex_width, __global char* hex_gray, __global char* hex_edge, __global int* ptr_hex_height,  __global int* ptr_half_org_height) {
	
	int threshold = *ptr_threshold, hex_width = *ptr_hex_width, hex_height = *ptr_hex_height, half_org_height = *ptr_half_org_height;
	int i, p, q;
	
	// Index of the elements to add
	unsigned int GId = get_global_id(0);
	

	p = (GId % (hex_width - 2)) + 1;
	q = (GId / (hex_width - 2)) + 1;
	
	//get neighbors
	unsigned char neighbors[6];
	neighbors[0] = hex_gray [p + (q + 1) * hex_width];
	neighbors[1] = hex_gray [p - 1 + (q + 1) * hex_width];
	neighbors[2] = hex_gray [p + 1 + q * hex_width];
	neighbors[3] = hex_gray [p - 1 + q * hex_width];
	neighbors[4] = hex_gray [p + 1 + (q - 1) * hex_width];
	neighbors[5] = hex_gray [p + (q - 1) * hex_width];

	//convolute
	int h1 = 0, h2 = 0, h3, r;
	char k1[] = {0, -1, +1, -1, +1, 0};
	char k2[] = {+1, +1, 0, 0, -1, -1};
	for (i = 0; i < 6; i++) {
		h1 = h1 + (k1[i] * neighbors[i]);
		h2 = h2 + (k2[i] * neighbors[i]);
	}
	h1 = (h1 < 0) ? (h1 * -1) : (h1);
	h2 = (h2 < 0) ? (h2 * -1) : (h2);
	h3 = h1 + h2;

	r = h1*h1 + h2*h2 + h3*h3 - h2*h3 + 1.732050808 * (h1 * (h2 - h3));
	if (r < threshold) {
		hex_edge [p - 1 + (q - 1) * (hex_width - 2)] = 0;
	}
	else {
		hex_edge [p - 1 + (q - 1) * (hex_width - 2)] = 255;
	}
	
	if((q > half_org_height && p < ((hex_width*(q - half_org_height) / (hex_height - half_org_height)) + 8)) || (q < (hex_height - half_org_height) && p > ((hex_width*q/(hex_height - half_org_height)) - 4) )) {
		hex_edge [p - 1 + (q - 1) * (hex_width - 2)] = 0;
	}

}
