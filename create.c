#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "header.h"
#include "utilities.h"
#include "mytar.h"

void writeFile(char *, int, int, int);
void writeHeader(char *, int, int, int);

/* Creates and sets the prefix in a header. Also sets the name. */
void setPrefix(char *src, Header *header) {
	/* If we're in this function, then the name is at least 101 
	 * characters long. */
	/* Get the length of the source file name */
	size_t src_len = strlen(src);
	/* This takes us 101 characters from the end */
	int i = src_len - 101;
	/* The remaining number of characters for the name field */
	int remaining;

	/* Keep incrementing i until it is the index of a 
	 * '/' character */
	while (src[i] != '/') {
		i++;
	}
	/* i now represents the number of characters that need to be
	 * copied over into the prefix field */

	if (i > PREFIX_SIZE) {
		fprintf(stderr, "path too long\n");
		exit(EXIT_FAILURE);
	}

	/* The remaining number of characters is the length of the 
	 * src - i subtracting one more because the '/' won't be
	 * copied */
	remaining = src_len - i - 1;
	/* After this while loop, i will represent the index at where
	 * the '/' character is */
	memmove(header->prefix, src, i);
	/* src +i + 1 means to start copying over at the character 
	 * right after the '/' */
	memmove(header->name, src + i + 1, remaining);
	return;
}

/* Writes a directory along with any files/directories/links that may
 * be inside of it */
void writeDirectory(char *src, int fdout, int strict, int verbose) {
	size_t dir_len = 0;
	DIR *dir;
	struct dirent *entry;
	char *path;
	struct stat *entry_info = malloc(sizeof(struct stat) * 1);
	if (!entry_info) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	/* A character array of 256 characters + 1 null terminating byte. 
	 * This represents the path of files in the directory. */
	path = calloc(PATH_LIMIT + 1, sizeof(char));
	if (!path) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	/* First copy over the directory name */
	strcpy(path, src);
	/* If the directory doesn't already end with a '/',
	 * then append it */
	if (strstr(path, "/") == NULL) {
		strcat(path, "/");
	}

	dir_len = strlen(path);
	/* Write the directory header to the archive */
	writeHeader(path, fdout, strict, verbose);

	/* Open the directory stream */
	if ((dir = opendir(path)) == NULL) {
		perror("opendir");
		return;
	}
	/* Loop through every entry in the directory */
	while ((entry = readdir(dir)) != NULL) {
		/* error checking for readdir goes here */
		/* Skip over current directory (.) and parent directory
		 * (..) */
		if ((strcmp(entry->d_name, ".") == 0) || 
				(strcmp(entry->d_name, "..") == 0)) {
			continue;
		}
		/* Check if the path would be too long */
		if (strlen(path) + strlen(entry->d_name) > PATH_LIMIT) {
			fprintf(stderr, "Path is too long\n");
			continue;
		}
		/* Append the file */
		strcat(path, entry->d_name);
		if (lstat(path, entry_info) == -1) {
			perror(path);
			continue;
		}

		/* Check if the entry is a file */
		if (S_ISREG(entry_info->st_mode)) {
			/* If it is, treat it as such and call writeFile */
			/* Write the file's header */
			writeHeader(path, fdout, strict, verbose);
			/* Write the file's data */
			writeFile(path, fdout, strict, verbose);
			/* Clear only the appended part for the next entry */
			memset(path + dir_len, 0, PATH_LIMIT + 1 - dir_len);
			continue;
		}
		/* Check if the entry is a directory */
		else if (S_ISDIR(entry_info->st_mode)) {
			strcat(path, "/");
			/* Need to recurse */
			writeDirectory(path, fdout, strict, verbose);
			/* Clear only the appended part for the next entry */
			memset(path + dir_len, 0, PATH_LIMIT + 1 - dir_len);
			continue;
		}
		/* Check if the entry is a sym link */
		else if (S_ISLNK(entry_info->st_mode)) {
			writeHeader(path, fdout, strict, verbose);
			/* Clear only the appended part for the next entry */
			memset(path + dir_len, 0, PATH_LIMIT + 1 - dir_len);
			continue;
		}

	}
	closedir(dir);
	free(entry_info);
	free(path);
	return;
}

/* Writes the data of a regular file to an archive */
void writeFile(char *src, int fdout, int strict, int verbose) {
	int fdin;
	int status = 0;
	/* Initialize a completely empty buffer to be read into */
	char *data = calloc(BLOCK_SIZE, sizeof(char));
	if (!data) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	fdin = open(src, O_RDONLY);
	if (fdin == -1) {
		perror("open");
		return;
	}
	
	/* Keep reading the source file, 512 bytes at a time, until
	 * end of file is returned (0) */
	while ((status = read(fdin, data, BLOCK_SIZE)) != 0) {
		if (status == -1) {
			perror(src);
			exit(EXIT_FAILURE);
		}
		if (write(fdout, data, BLOCK_SIZE) == -1) {
			fprintf(stderr, "can't write to archive\n");
			return;
		}
		/* This ensures that the entire block is emptied before
		 * the next read. Otherwise, if a read of less than 512 
		 * bytes occurs, write might write junk data. */
		memset(data, 0, BLOCK_SIZE);
	}
	free(data);
	close(fdin);
	return;
}

/* Given a file/symlink/directory, this function will write a
 * header to the archive, printing out the names as they are 
 * added if verbose is set. */
void writeHeader(char *src, int fdout, int strict, int verbose) {
	Header *header;
	struct stat *src_info;
	struct passwd *pswrd;
	struct group *grp;
	
	/* Print file name if verbose is set */
	if (verbose) {
		printf("%s\n", src);
	}

	src_info = malloc(sizeof(struct stat) * 1);
	if (!src_info) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	/* Stat the source file */
	if (lstat(src, src_info) == -1) {
		perror("lstat");
		return;
	}
	
	header = calloc(1, sizeof(Header));
	if (!header) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	/* Write the name of the source file to the header if it's below
	 * 100 characters */
	if (strlen(src) <= NAME_SIZE) {
		strcpy(header->name, src);
	}
	/* Otherwise the name is too long so need to create a prefix */
	else {
		setPrefix(src, header);	
	}
	
	/* Set the mode. AND the mode with 07777 octal because only 
	 * want to extract the permissions part of the mode */
	sprintf(header->mode, "%07o", src_info->st_mode & PERMS_MASK);
	
	/* Set the uid */
	/* First check if the uid exceeds the max uid size allowed by 
	 * 7 octal digits */
	if (src_info->st_uid < MAX_UID_SIZE) {
		sprintf(header->uid, "%07o", src_info->st_uid);
	}
	/* uid too big */
	else {
		/* Check if in strict mode */
		if (strict) {
			fprintf(stderr, 
					"%s: uid too big.  Skipping.\n",
					src);
			return;
		}
		insert_special_int(header->uid, UID_SIZE, src_info->st_uid);
	}
	/* Set the gid */
	if (src_info->st_gid < MAX_GID_SIZE) {
		sprintf(header->gid, "%07o", src_info->st_gid);
	}
	/* gid too big */
	else {
		/* Check if in strict mode */
		if (strict) {
			fprintf(stderr, 
					"%s: gid too big.  Skipping.\n",
					src);
			return;
		}
		insert_special_int(header->gid, GID_SIZE, src_info->st_gid);
	}	

	/* Set the size */
	/* Check the file type */
	/* File type is regular */
	if (S_ISREG(src_info->st_mode)) {
		/* Size too big */
		if (src_info->st_size > MAX_SIZE_SIZE) {
			if (strict) {
				fprintf(stderr, 
						"%s: size too big.   "
						"Skipping.\n",
						src);
				return;
			}
			insert_special_int(header->size, SIZE_SIZE, 
					src_info->st_size);
		}
		else {
			sprintf(header->size, "%011o", (int)src_info->st_size);
		}
		*(header->typeflag) = REG_FLAG; 
	}
	/* File type is directory */
	else if (S_ISDIR(src_info->st_mode)) {
		sprintf(header->size, "%011o", 0);
		*(header->typeflag) = DIR_FLAG;
	}
	/* File type is symbolic link */
	else if (S_ISLNK(src_info->st_mode)) {
		sprintf(header->size, "%011o", 0);
		*(header->typeflag) = SYM_FLAG;
	}

	/* Set the mtime */
	if (src_info->st_mtime < MAX_MTIME_SIZE) {
		sprintf(header->mtime, "%011o", (int)src_info->st_mtime);
	}
	/* mtime too big */
	else {
		if (strict) {
			fprintf(stderr, 
					"%s: size too big.  Skipping.\n",
					src);
			return;
		}
		insert_special_int(header->mtime, MTIME_SIZE, 
				src_info->st_mtime);
	}
	/* Set the link name (if file is a symlink) */
	if (S_ISLNK(src_info->st_mode)) {
		if (readlink(src, header->linkname, LINKNAME_SIZE) == -1) {
			perror(src);
			return;
		}
	}

	/* Set the magic number */
	strcpy(header->magic, MAGIC_NUM);
	
	/* Set the version number */
	strcpy(header->version, VERSION_NUM);

	/* Initialize password struct to get the user name */
	pswrd = getpwuid(src_info->st_uid);
	strcpy(header->uname, pswrd->pw_name);

	/* Initialize group struct to get the group name */
	grp = getgrgid(src_info->st_gid);
	strcpy(header->gname, grp->gr_name);

	/* Major and minor device numbers aren't relevant for
	 * this assignment. The header fields for these are 
	 * already zero since we calloc'd. */

	/* Set the checksum */
	setChksum(header);

	/* Write the header */
	if (write(fdout, header, BLOCK_SIZE) == -1) {
		perror("write");
		return;
	}
	
	free(src_info);
	free(header);
	return;
}

/* Creates an archive with files specified by the user. If one of the 
 * paths given is a directory, all the directories contents will also
 * be added. */
void createArchive(int numPaths, char *paths[], int strict, int verbose) {
	int i;
	char *data;	
	struct stat *src_info;
	int fdout;
	fdout = open(paths[TAR_INDEX], 
			O_WRONLY | O_CREAT | O_TRUNC, 
			S_IRUSR | S_IWUSR);
	if (fdout == -1) {
		perror(paths[TAR_INDEX]);
	}
	src_info = malloc(sizeof(struct stat) * 1);
	if (!src_info) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (i = ARG_START; i < numPaths; i++) {
		/* Check if path is too long */
		if (strlen(paths[i]) > PATH_LIMIT) {
			fprintf(stderr, "%s: path too long\n", paths[i]);
			continue;
		}

		/* Use lstat so we can see if dealing with a directory 
		 * OR the file doesn't exist */
		if (lstat(paths[i], src_info) == -1) {
			perror(paths[i]);
			continue;
		}	

		/* File to be archived is a regular file */
		if (S_ISREG(src_info->st_mode)) {
			writeHeader(paths[i], fdout, strict, verbose);
			writeFile(paths[i], fdout, strict, verbose);
		}
		/* File to be archived is a directory */
		else if (S_ISDIR(src_info->st_mode)) {
			writeDirectory(paths[i], fdout, strict, verbose);
		}
		/* File to be archived is a symlink */
		else if (S_ISLNK(src_info->st_mode)) {
			writeHeader(paths[i], fdout, strict, verbose);
		}
	}
	
	data = calloc(BLOCK_SIZE, sizeof(char));

	/* Write the End of Archive marker which is two blocks of 
	 * zero bytes. */
	if (write(fdout, data, BLOCK_SIZE) == -1) {
		perror("write");
		exit(EXIT_FAILURE);
	}
	if (write(fdout, data, BLOCK_SIZE) == -1) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	free(data);
	free(src_info);
	close(fdout);
	return;
}

