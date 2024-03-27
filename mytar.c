#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "create.h"
#include "list.h"
#include "extract.h"
#include "mytar.h"

int main(int argc, char *argv[]) {
	int i;
	size_t num_options = 0;
	int c_flag = 0;
	int t_flag = 0;
	int x_flag = 0;
	int v_flag = 0;
	int f_flag = 0;
	int s_flag = 0;
	/* Represents, uniquely, how many of the c, t, and x 
	 * flags are set */
	int req_flags = 0;
	int unique_flags = 0;

	if (argc < 3) {
		fprintf(stderr, "usage: %s [ctxvS]f tarfile "
				"[ path [ ... ] ]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	num_options = strlen(argv[OPTS_INDEX]);
	for (i = 0; i < num_options; i++) {
		if (argv[OPTS_INDEX][i] == 'c') {
			if (c_flag == 0) {
				req_flags += 1;
				unique_flags += 1;
			}
			c_flag += 1;
		}
		else if (argv[OPTS_INDEX][i] == 't') {
			if (t_flag == 0) {
				req_flags += 1;
				unique_flags += 1;
			}
			t_flag += 1;
		}
		else if (argv[OPTS_INDEX][i] == 'x') {
			if (x_flag == 0) {
				req_flags += 1;
				unique_flags += 1;
			}
			x_flag += 1;
		}
		else if (argv[OPTS_INDEX][i] == 'v') {
			if (v_flag == 0) {
				unique_flags += 1;
			}
			v_flag += 1;
		}
		else if (argv[OPTS_INDEX][i] == 'f') {
			if (f_flag == 0) {
				unique_flags += 1;
			}
			f_flag += 1;
		}
		else if (argv[OPTS_INDEX][i] == 'S') {
			if (s_flag == 0) {
				unique_flags += 1;
			}
			s_flag += 1;
		}
		else {
			fprintf(stderr, "usage: %s [ctxvS]f tarfile "
					"[ path [ ... ] ]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
		/* c, t, and x options can only be set once */
		if (c_flag > 1 || t_flag > 1 || x_flag > 1) {
			fprintf(stderr, "%s: you must choose " 
					"one of the 'ctx' options.\n"
					"usage: %s [ctxvS]f tarfile "
					"[ path [ ... ] ]\n", 
					argv[0], argv[0]);
			exit(EXIT_FAILURE);
		}	
	}

	/* Only one of c, t, or x options can be chosen */
	if (req_flags != NUM_REQ_OPTS) {
		fprintf(stderr, "%s: you must choose " 
				"one of the 'ctx' options.\n"
				"usage: %s [ctxvS]f tarfile "
				"[ path [ ... ] ]\n", 
				argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Can't have more than 4 options or less than 2 options */
	if (unique_flags > MAX_OPTS || 
		unique_flags < MIN_OPTS) {
		fprintf(stderr, "%s: you must choose " 
				"one of the 'ctx' options.\n"
				"usage: %s [ctxvS]f tarfile "
				"[ path [ ... ] ]\n", 
				argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

	/* f option wasn't given */
	if (f_flag == 0) {
		fprintf(stderr, "%s: you must choose " 
				"one of the 'ctx' options.\n"
				"usage: %s [ctxvS]f tarfile "
				"[ path [ ... ] ]\n", 
				argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

	/* After all that checking, if we make it this far, then
	 * all flags are valid  */
	if (c_flag) {
		createArchive(argc, argv, s_flag, v_flag);
	}
	else if (t_flag) {
		listArchive(argc, argv, s_flag, v_flag);
	}
	else if (x_flag) {
		extractArchive(argc, argv, s_flag, v_flag);
	}

	return 0;
}

