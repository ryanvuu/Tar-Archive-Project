#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "header.h"
#include "utilities.h"
#include "mytar.h"

/* Gets the of a file given its corresponding header */
char *getPerms(Header *header, char *perms) {
	long int h_mode = strtol(header->mode, NULL, OCTAL_BASE);

	/* Check type of file */
	if (*header->typeflag == REG_FLAG) {
		perms[TYPE_INDEX] = '-';
	}
	else if (*header->typeflag == DIR_FLAG) {
		perms[TYPE_INDEX] = 'd';
	}
	else if (*header->typeflag == SYM_FLAG) {
		perms[TYPE_INDEX] = 'l';
	}

	/* User permissions */
	if (h_mode & S_IRUSR) {
		perms[USR_R] = 'r';
	}
	else {
		perms[USR_R] = '-';
	}
	if (h_mode & S_IWUSR) {
		perms[USR_W] = 'w';
	}
	else {
		perms[USR_W] = '-';
	}
	if (h_mode & S_IXUSR) {
		perms[USR_X] = 'x';
	}
	else {
		perms[USR_X] = '-';
	}

	/* Group permissions */
	if (h_mode & S_IRGRP) {
		perms[GRP_R] = 'r';
	}
	else {
		perms[GRP_R] = '-';
	}
	if (h_mode & S_IWGRP) {
		perms[GRP_W] = 'w';
	}
	else {
		perms[GRP_W] = '-';
	}
	if (h_mode & S_IXGRP) {
		perms[GRP_X] = 'x';
	}
	else {
		perms[GRP_X] = '-';
	}

	/* Other permissions */
	if (h_mode & S_IROTH) {
		perms[OTH_R] = 'r';
	}
	else {
		perms[OTH_R] = '-';
	}
	if (h_mode & S_IWOTH) {
		perms[OTH_W] = 'w';
	}
	else {
		perms[OTH_W] = '-';
	}
	if (h_mode & S_IXOTH) {
		perms[OTH_X] = 'x';
	}
	else {
		perms[OTH_X] = '-';
	}
	perms[PERMS_WIDTH] = '\0';
	return perms;
}

/* Prints out permissions, owner/group, size, mtime,
 * and name fields of a file if verbose was set. */
void printVerbose(Header *header, char *name) {
	char perms[PERMS_WIDTH + 1];
	char *owngrp;
	long int size = 0;
	char mtime[MTIME_WIDTH + 1];
	time_t header_mtime = 0;
	struct tm *time;
	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int minute = 0;
	/* The length of the uname, gname, a slash character, and 
	 * null-terminating character. */
	size_t owngrp_size = strnlen(header->uname, UNAME_SIZE) + 
		strnlen(header->gname, GNAME_SIZE) +
		1 + 1;

	time = malloc(sizeof(struct tm) * 1);
	if (!time) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	owngrp = calloc(owngrp_size, sizeof(char));
	if (!owngrp) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	/* Sets the permissions field */
	getPerms(header, perms);
	/* Copies over at most, 18 bytes (the last byte being a null
	 * character. First, as much of the uname will be copied over, 
	 * then if there is still space, a slash is added, then if 
	 * there is still space, as much of the gname */
	snprintf(owngrp, owngrp_size, 
					"%s/%s", 
					header->uname, 
					header->gname);
	/* Get the size of the file */
	size = strtol(header->size, NULL, OCTAL_BASE);

	/* Convert the header->mtime to a long int via strtol, then type 
	 * cast that to time_t */
	header_mtime = (time_t)strtol(header->mtime, NULL, OCTAL_BASE);
	time = localtime(&header_mtime);
	year = REL_YEAR + time->tm_year;
	month = JANUARY + time->tm_mon;
	day = time->tm_mday;
	hour = time->tm_hour;
	minute = time->tm_min;
	snprintf(mtime, MTIME_WIDTH + 1, "%04d-%02d-%02d %02d:%02d", 
			year, month, day, hour, minute);
	/* Print out every field */
	printf("%10s %-17s %8ld %16s %s\n", perms, owngrp, size, mtime, name);
	return;
}


/* Lists the contents of an archive. 
 * All contents are listed if no path(s) are given, otherwise only
 * those paths are listed. */
void listArchive(int numPaths, char *paths[], int strict, int verbose) {
	int i, j;
	/* The number of data blocks to possibly skip over */
	int num_dblocks = 0;
	/* This is what the checksum of a given header should be */
	unsigned int csum;
	/* This is what the header's checksum is */
	unsigned int hcsum;
	/* This is a counter to check end of archive */
	int eoa = 0;
	/* The name of a path can be at most 256 chars. */
	char name[PATH_LIMIT];
	long int size;
	int fdarchive;
	/* Don't need to calloc since header data will get overwritten with
	 * archive headers. */
	Header *header;
	/* Flag to signify that we are listing, not extracting since both
	 * list and extract use the same function, isValid, to see what is
	 * valid to list/extract. */
	int t_flag = 1;

	/* Check if a valid archive was given which is paths[2] */

	header = malloc(sizeof(Header) * 1);
	if (!header) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	/* Open the archive for reading */
	fdarchive = open(paths[2], O_RDONLY);
	if (fdarchive == -1) {
		perror(paths[2]);
		free(header);
		exit(EXIT_FAILURE);
	}

	while (1) {
		/* Read a header */
		if (read(fdarchive, header, BLOCK_SIZE) == -1) {
			perror(paths[2]);
			exit(EXIT_FAILURE);
		}
		/* End of archive reached, return */
		if (eoa == 2) {
			return;
		}

		/* Check if the header's checksum is the same as what the
		 * checksum should actually be */
		csum = getChksum(header);
		hcsum = strtol(header->chksum, NULL, OCTAL_BASE);
		/* Checksums don't match */
		if (csum != hcsum) {
			/* Potentially end of archive */
			if (csum == 256) {
				for (j = 0; j < BLOCK_SIZE; j++) {
					if (((unsigned char *)header)[j] != 
							'\0') {
						fprintf(stderr, 
								"Incorrect "
								"header "
								"checksum\n");
						free(header);
						exit(EXIT_FAILURE);
					}
				}
				/* Increment the eoa counter */
				eoa += 1;
				continue;
			}
			else {
				fprintf(stderr, "Incorrect header checksum\n");
				free(header);
				exit(EXIT_FAILURE);
			}
		}
		else {
			/* Reset the eoa counter */
			eoa = 0;
		}

		/* Strict is set so check magic and version */
		if (strict) {
			strictCheck(header);	
		}
		/* Not strict so just check if magic field is ustar */
		else {
			if (strncmp(header->magic, "ustar", 
						MAGIC_SIZE - 1) != 0) {
				fprintf(stderr, "Header magic field "
						"not valid\n");
				free(header);
				exit(EXIT_FAILURE);
			}
		}

		/* Get the size of the file */
		size = strtol(header->size, NULL, OCTAL_BASE);

		/* Using strnlen, strncpy, and strncat because we can't
		 * assume that the name or prefix is null-terminated. */
		/* Check if path would be too long */
		if (strnlen(header->name, NAME_SIZE) + 1 +
				strnlen(header->prefix, PREFIX_SIZE) > 
				PATH_LIMIT) {
			fprintf(stderr, "path too long\n");
			free(header);
			continue;
		}
		/* Construct name */
		/* No prefix, so entire name is just in name field */
		if (header->prefix[0] == '\0') {	
			strncpy(name, header->name, NAME_SIZE);
		}
		/* Prefix exists */
		else {
			/* Copy over the prefix to name */
			strncpy(name, header->prefix, PREFIX_SIZE);
			/* Append a '/' */
			strcat(name, "/");
			/* Append the name field to name */
			strncat(name, header->name, NAME_SIZE);
		}

		/* No paths were given, so print the entire archive. */
		if (numPaths < 4) {
			/* Verbose set */
			if (verbose) {
				printVerbose(header, name);
			}
			/* Verbose not set */
			else {
				printf("%s\n", name);
			}
		}	
		/* Paths were given */
		else {
			/* From the mytar demo, entries are listed in the 
			 * order they appear in the archive, not the order 
			 * in which they were passed as arguments. */
			for (i = ARG_START; i < numPaths; i++) {
				/* Check if one of the given path is 
				 * the same as the current header. */
				if (isValid(name, paths[i], t_flag)) {
					/* Verbose set */
					if (verbose) {
						printVerbose(header, name);
						break;
					}
					/* Verbose not set */
					else {
						printf("%s\n", name);
						break;
					}
				}
			}
		}
		/* Checks if the file has contents */
		if (size > 0) {
			/* Gets the ceiling of dividing the size with 512 
			 * which results in the number of blocks we need to 
			 * skip over to reach the next header */
			num_dblocks = size/BLOCK_SIZE + 
				(size % BLOCK_SIZE != 0);
			/* Skips over the contents */
			if (lseek(fdarchive, 
					  BLOCK_SIZE * num_dblocks, 
					  SEEK_CUR) 
					  == -1) {
				perror("lseek");
				/* if lseek fails, then the next header read 
				 * will fail anyway */
			}
		}
	}
	free(header);
	close(fdarchive);
	return;
}

