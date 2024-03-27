#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <errno.h>

#include "header.h"
#include "utilities.h"
#include "mytar.h"

/* Restores the mtime of a file while leaving the access time 
 * unmodified.
 * This function is only used for regular files */
void restoreTimes(Header *header, char *name, int file) {
	/* The file's mtime */
	time_t mtime;
	struct utimbuf *file_times;
	/* This will just be used to get the access time */
	struct stat *file_info;
	
	/* Get the file's original mtime */
	mtime = (time_t)strtol(header->mtime, NULL, OCTAL_BASE);
	/* Get the times of the file that we just created */
	file_info = malloc(sizeof(struct stat) * 1);
	if (!file_info) {
		perror("malloc");
		close(file);
		exit(EXIT_FAILURE);
	}

	if (lstat(name, file_info) == -1) {
		perror("lstat");
		return;
	}

	file_times = malloc(sizeof(struct utimbuf) * 1);
	if (!file_times) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	/* Access time stays the same */
	file_times->actime = file_info->st_atime;
	/* Restore modification time */
	file_times->modtime = mtime;

	/* Change the file time */
	if (utime(name, file_times) == -1) {
		perror("utime");
		exit(EXIT_FAILURE);
	}
	free(file_info);
	free(file_times);
	return;
}

/* Checks if a given directory exists. */
int checkDirectory(char *name) {
	struct stat dir_info;
	int res;
	/* Don't need to error check because if stat returns
	 * -1, then the directory doesn't exist */
	res = stat(name, &dir_info);

	/* Directory exists */
	if (res == 0 && S_ISDIR(dir_info.st_mode)) {
		return 1;
	}
	return 0;
}

/* Makes a directory with the original name and perms. */
void extractDirectory(Header *header, char *name) {
	int dir_exists;
	/* The directory's permissions */
	mode_t modes;
	
	/* Check if the directory already exists */
	dir_exists = checkDirectory(name);

	/* If the directory doesn't exist, make it. */
	if (!dir_exists) {
		/* Get the directory's file protection modes */
		modes = (mode_t)strtol(header->mode, NULL, OCTAL_BASE);
		if (mkdir(name, modes) == -1) {
			perror("mkdir");
			exit(EXIT_FAILURE);
		}
	}
	return;
}

/* Creates a symlink unless it already exists. */
void extractSymlink(Header *header, char *name) {
	/* Most commonly will occur if file already exists */
	if (symlink(header->linkname, name) == -1) {
		perror(name);
		return;
	}
	return;
}

/* Creates a file with the original name and writes all of its contents. 
 * The parameter size is passed by value so the size in extractArchive can 
 * still be used when deciding if we need to lseek. */
void extractFile(int fdarchive, Header *header, char *name, long int size) {
	int fdout;
	/* The file's permissions */
	mode_t modes;

	/* Initialize a completely empty buffer to be read into */
	char *data = calloc(BLOCK_SIZE, sizeof(char));
	if (!data) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	/* Get the file protection modes that correspond to this file */
	modes = (mode_t)strtol(header->mode, NULL, OCTAL_BASE);
	/* Create a file with the same name and perms. as was archived. */
	fdout = open(name, O_WRONLY | O_CREAT | O_TRUNC, modes);
	if (fdout == -1) {
		perror(name);
		free(data);
		return;
	}
	
	/* Open error check goes here */
	while (1) {
		/* We have finished writing/creating the file once size
		 * is less than or equal to zero, indicating all bytes
		 * have been written */
		if (size <= 0) {
			break;
		}
		
		/* Read a block */
		if (read(fdarchive, data, BLOCK_SIZE) ==  -1) {
			fprintf(stderr, "error reading archive\n");
			exit(EXIT_FAILURE);
		}

		/* If size is greater than 512, we can write an entire block */
		if (size > BLOCK_SIZE) {
			if (write(fdout, data, BLOCK_SIZE) == -1) {
				perror(name);
				exit(EXIT_FAILURE);
			}
			size -= BLOCK_SIZE;
		}
		/* Otherwise, it is less than 512 so we only want to write 
		 * however many bytes are left, size. */
		else {
			if(write(fdout, data, size) == -1) {
				perror(name);
				exit(EXIT_FAILURE);
			}
			size -= size;
		}

		/* This ensures that the entire block is emptied before
		 * the next read. Otherwise, if a read of less than 512 
		 * bytes occurs, write might write junk data. */
		memset(data, 0, BLOCK_SIZE);
	}
	
	restoreTimes(header, name, fdout);
	free(data);
	close(fdout);
	return;
}

/* A lot of the logic used in here is reused from list since they both read
 * through an archive. */
void extractArchive(int numPaths, char *paths[], int strict, int verbose) {
	int i, j;
	/* Flag indicating if a file was extracted or not */
	int was_extracted;
	/* The number of data blocks to possibly skip over */
	int num_dblocks = 0;
	/* This is what the checksum of a given header should be */
	unsigned int csum;
	/* This is what the header's checksum is */
	unsigned int hcsum;
	/* This is a counter to check end of archive */
	int eoa = 0;
	/* The name of a path can be at most 256 chars. plus a null terminating
	 * char. */
	char name[PATH_LIMIT + 1];
	long int size;
	int fdarchive;
	/* Don't need to calloc since header data will get overwritten with
	 * archive headers. */
	Header *header;
	/* Flag to signify that we are extracting, not listing since both
	 * list and extract use the same function, isValid, to see what is
	 * valid to list/extract. */
	int t_flag = 0;
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
		/* Error checking for read goes here */
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
								"Incorrect" 
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
			if (strncmp(header->magic, 
						"ustar", 
						MAGIC_SIZE - 1) != 0) {
				fprintf(stderr, "Header magic field "
						"not valid\n");
				free(header);
				exit(EXIT_FAILURE);
			}
		}

		/* Get the size of the file */
		size = strtol(header->size, NULL, OCTAL_BASE);
		was_extracted = 0;
		
		/* Using strnlen, strncpy, and strncat because we can't
		 * assume that the name or prefix is null-terminated. */
		/* Check if path would be too long */
		if (strnlen(header->name, NAME_SIZE) + 1 +
				strnlen(header->prefix, PREFIX_SIZE) > 
				PATH_LIMIT) {
			fprintf(stderr, "path too long\n");
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

		/* No paths were given, so extract the entire archive. */
		if (numPaths < 4) {
			/* Extract regular file */
			if (*header->typeflag == REG_FLAG) {
				extractFile(fdarchive, header, name, size);
			}
			/* Extract directory */
			else if (*header->typeflag == DIR_FLAG) {
				extractDirectory(header, name);
			}
			/* Extract symbolic link */
			else if (*header->typeflag == SYM_FLAG) {
				extractSymlink(header, name);
			}
			/* Wait until after extraction to print name */
			if (verbose) {
				printf("%s\n", name);
			}
		}
		/* Paths were given */
		else {
			/* Start looping through each given path */
			for (i = ARG_START; i < numPaths; i++) {
				/* strcmp checks if the current header is a 
				 * path argument that was passed. The 
				 * arguments passed to strstr are flipped 
				 from list.c. This is to ensure that the 
				 directory is always created first if a path
				 is a file/directory inside a directory. */
				if (isValid(name, paths[i], t_flag)) {
					if (*header->typeflag == REG_FLAG) {
						extractFile(fdarchive, header, 
								name, size);
						if (verbose) {
							printf("%s\n", name);
						}
						was_extracted = 1;
						break;
					}
					else if (*header->typeflag==DIR_FLAG) {
						extractDirectory(header, name);
						if (verbose) {
							printf("%s\n", name);
						}

						was_extracted = 1;
						break;
					}
					else if(*header->typeflag == SYM_FLAG) {
						extractSymlink(header, name);
						if (verbose) {
							printf("%s\n", name);
						}
						was_extracted = 1;
						break;
					}
				}
			}
			/* Once out of the for loop one of two scenarios have
			 * occured:
			 * 1: The current header matched one of the paths and
			 * was successfully extracted.
			 * 2: The current header did not match any of the given
			 * paths. 
			 * If case 2 occurred, then may need to lseek. */
			if (!was_extracted) {
				/* If the current header is a regular 
				 * file, need to skip ahead to the next 
				 * header */
				if (*header->typeflag == REG_FLAG) {
					/* Gets the ceiling of dividing the 
					 * size with 512 which results in the 
					 * number of blocks we need to skip 
					 * over to reach the next header */
					num_dblocks = size/BLOCK_SIZE + 
						(size % BLOCK_SIZE != 0);
					/* Skips over the contents */
					if (lseek(fdarchive, 
					  BLOCK_SIZE * num_dblocks, 
					  SEEK_CUR) 
					  == -1) {
						perror("lseek");
					/* if lseek fails, then the next 
					 * header read will fail anyway */
					}
				}
			}

		}

	} /* This is the while loop */
	free(header);
	close(fdarchive);
	return;
}

