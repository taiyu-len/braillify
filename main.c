#include "braillify.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	const char* filename = NULL;
	int opt;
	double threshhold = .5;
	int    invert = 0;
	while ((opt = getopt(argc, argv, "t:i")) != -1) {
		switch (opt) {
		case 't': {
			threshhold = strtod(optarg, NULL);
			break;
		}
		case 'i': {
			invert = 1;
			break;
		}
		}
	}
	if (optind < argc) {
		filename = argv[optind];
	} else {
		fprintf(stderr, "pass filename in\n");
		return 0;
	}
	char *result = braillify(filename, threshhold, invert);
	if (result) {
		printf("%s", result);
	} else {
		fprintf(stderr, "invalid filename\n");
	}
	free(result);
	return 0;
}

