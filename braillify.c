#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "braillify.h"

typedef struct braillify {
	// input image
	const char*    filename;
	unsigned char* image;
	double *data;

	// input image dimensions
	int w,  h;
	double threshhold;
	int inverted;
} braillify_t;

static
double* at(braillify_t* b, int x, int y)
{
	return &b->data[x + y * b->w];
}

static
int between(int low, int val, int max)
{
	return val >= low && val < max;
}

static
void floyd_dither(braillify_t* b)
{
	int h = b->h, w = b->w;
	for (int y = 0; y < h; ++y) {
		int dir = (y % 2) ? -1 : 1;
		int x = dir > 0 ? 0 : w-1;
		for (; between(0, x, w); x += dir) {
			double old_pixel = *at(b, x, y);
			double new_pixel = old_pixel > b->threshhold ? 1 : 0;
			*at(b, x, y) = new_pixel;
			double e = old_pixel - new_pixel;
			if (between(0, x+dir, w)) {
				*at(b, x+dir, y) += e * 7./16.;
			}
			if (y+1 >= h) {
				continue;
			}
			*at(b, x, y+1) += e * 5./16.;
			if (between(0, x+dir, w)) {
				*at(b, x+dir, y+1) += e * 3./16.;
			}
			if (between(0, x-dir, w)) {
				*at(b, x-dir, y+1) += e * 1./16.;
			}
		}
	}
}

// the bits of the index of each braille character corresponds to whether the 
// dot is set or not.
const char* braille =
"⠀⠁⠂⠃⠄⠅⠆⠇⡀⡁⡂⡃⡄⡅⡆⡇⠈⠉⠊⠋⠌⠍⠎⠏⡈⡉⡊⡋⡌⡍⡎⡏"
"⠐⠑⠒⠓⠔⠕⠖⠗⡐⡑⡒⡓⡔⡕⡖⡗⠘⠙⠚⠛⠜⠝⠞⠟⡘⡙⡚⡛⡜⡝⡞⡟"
"⠠⠡⠢⠣⠤⠥⠦⠧⡠⡡⡢⡣⡤⡥⡦⡧⠨⠩⠪⠫⠬⠭⠮⠯⡨⡩⡪⡫⡬⡭⡮⡯"
"⠰⠱⠲⠳⠴⠵⠶⠷⡰⡱⡲⡳⡴⡵⡶⡷⠸⠹⠺⠻⠼⠽⠾⠿⡸⡹⡺⡻⡼⡽⡾⡿"
"⢀⢁⢂⢃⢄⢅⢆⢇⣀⣁⣂⣃⣄⣅⣆⣇⢈⢉⢊⢋⢌⢍⢎⢏⣈⣉⣊⣋⣌⣍⣎⣏"
"⢐⢑⢒⢓⢔⢕⢖⢗⣐⣑⣒⣓⣔⣕⣖⣗⢘⢙⢚⢛⢜⢝⢞⢟⣘⣙⣚⣛⣜⣝⣞⣟"
"⢠⢡⢢⢣⢤⢥⢦⢧⣠⣡⣢⣣⣤⣥⣦⣧⢨⢩⢪⢫⢬⢭⢮⢯⣨⣩⣪⣫⣬⣭⣮⣯"
"⢰⢱⢲⢳⢴⢵⢶⢷⣰⣱⣲⣳⣴⣵⣶⣷⢸⢹⢺⢻⢼⢽⢾⢿⣸⣹⣺⣻⣼⣽⣾⣿";

const char* to_braille(braillify_t* b, int x, int y) {
	int h = b->h, w = b->w;
	unsigned int offset = 0;
	// Set first row of bits
	for (unsigned i = 0; i <= 3 && y + i < h; ++i) {
		if (*at(b, x, y+1) > 0.5f) {
			offset |= 1u << i;
		}
		if (x + 1 < w && *at(b, x+1, y+1) > 0.5f) {
			offset |= 1u << (i + 4);
		}
	}
	if (b->inverted) {
		offset ^= (1u << 8u) - 1;
	}
	return braille + offset*3;
}

char* braillify(const char* filename, double threshhold, int inverted) {
	char *result = NULL;
	braillify_t b = {
		.filename   = filename,
		.image      = stbi_load(filename, &b.w, &b.h, NULL, 1),
		.inverted   = inverted,
		.threshhold = threshhold
	};
	if (b.image == NULL || b.w == 0 || b.h == 0) {
		fprintf(stderr, "failed to allocate memory for processing\n");
		goto done;
	}

	// convert image to doubles
	b.data = malloc(b.w * b.h * sizeof *b.data);
	if (b.data == NULL) {
		fprintf(stderr, "failed to allocate memory for processing\n");
		goto cleanup_image;
	}

	for (int i = 0; i < b.w * b.h; ++i) {
		b.data[i] = b.image[i] / 256.0f;
	}

	floyd_dither(&b);

	int bw = b.w/2 + b.w%2;
	int bh = b.h/4 + (b.h%4 > 0);
	result = malloc(bw * bh * 3 + bh + 1);
	if (result == NULL) {
		fprintf(stderr, "failed to allocate memory for result string\n");
		goto cleanup_data;
	}

	char *it = result;
	for (int y = 0; y < b.h; y+=4) {
		for (int x = 0; x < b.w; x+=2) {
			strncpy(it, to_braille(&b, x, y), 3);
			it += 3;
		}
		*it++ = '\n';
	}
	*it = '\0';

cleanup_data:
	free(b.data);

cleanup_image:
	stbi_image_free(b.image);

done:
	return result;
}


