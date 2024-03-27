#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "header.h"
#include "mytar.h"

uint32_t extract_special_int(char *where, int len) {
	int32_t val = -1;
	if( (len >= sizeof(val)) && (where[0] & 0x80)) {
		/* the top bit is set and we have space
		 * extract the last four bytes */
		val = *(int32_t *)(where+len-sizeof(val));
		val = ntohl(val);       /* convert to host byte order */
	}
	return val;
}

int insert_special_int(char *where, size_t size, int32_t val) {
	int err = 0;

	if (val < 0 || (size < sizeof(val))) {
		/* if it's negative, bit 31 is set and we can't use the flag
		 * if len is too small, we can't write it. Either way, we're
		 * done.
		 */
		err++;
	} else {
		/* game on.... */
		memset(where, 0, size);  /* Clear out the buffer */
		/* place the int */
		*(int32_t *)(where+size-sizeof(val)) = htonl(val);
		*where |= 0x80;          /* set that high-order bit */
	}

	return err;
}

/* Calculates and sets the check sum of a header */
void setChksum(Header *header) {
	int i;
	unsigned int chksum = 0;
	/* Loop through the entire header, skipping over the chksum field
	 * to calculate the chksum */
	for (i = 0; i < BLOCK_SIZE; i++) {
		/* These are the bounds of the chksum field */
		if (i >= CHKSUM_OFFSET && i <= CHKSUM_OFFSET + CHKSUM_SIZE -1) {
			continue;
		}
		/* Type cast header to an unsigned char pointer otherwise 
		 * incompatible types and can't add */
		chksum += ((unsigned char *)header)[i];
	}
	/* Add in 8 spaces which represent the initial checksum */ 
	chksum += CHKSUM_SIZE * ' ';
	sprintf(header->chksum, "%07o", chksum);
	return;
}

/* Same logic as setChksum, only we return the checksum instead */
unsigned int getChksum(Header *header) {
	int i;
	unsigned int chksum = 0;
	/* Loop through the entire header, skipping over the chksum field
	 * to calculate the chksum */
	for (i = 0; i < BLOCK_SIZE; i++) {
		/* These are the bounds of the chksum field */
		if (i >= CHKSUM_OFFSET && i <= CHKSUM_OFFSET + CHKSUM_SIZE -1) {
			continue;
		}
		/* Type cast header to an unsigned char pointer otherwise 
		 * incompatible types and can't add */
		chksum += ((unsigned char *)header)[i];
	}
	/* Add in 8 spaces which represent the initial checksum */ 
	chksum += CHKSUM_SIZE * ' ';
	return chksum;
}

/* Checks if a header's magic and version fields are "ustar" null-
 * terminated and "00" respectively. */
void strictCheck(Header *header) {
	uint32_t num = 0;
	/* Checks if the header->magic field is "ustar" null-terminated */
	if (strncmp(header->magic, "ustar", MAGIC_SIZE) != 0) {
		fprintf(stderr, "Header magic field not valid\n");
		free(header);
		exit(EXIT_FAILURE);
	}
	/* Checks if the header->version field is "00" */
	if (strncmp(header->version, "00", VERSION_SIZE) != 0) {
		fprintf(stderr, "Header version field not valid\n");
		free(header);
		exit(EXIT_FAILURE);
	}
	/* Checks if the header->uid field is too long */
	num = extract_special_int(header->uid, UID_SIZE);
	if (num > MAX_UID_SIZE) {
		fprintf(stderr, "Header uid field invalid\n");
		free(header);
		exit(EXIT_FAILURE);
	}
	/* Checks if the header->gid field is too long */
	num = extract_special_int(header->gid, GID_SIZE);
	if (num > MAX_GID_SIZE) {
		fprintf(stderr, "Header gid field invalid\n");
		free(header);
		exit(EXIT_FAILURE);
	}
	return;
}

int isValid(char *name, char *path, int t_flag) {
	int res;
	char *name_tok;
	char *path_tok;
	char *path_copy;
	char *name_copy;
	char *ncptr;
	char *pcptr;
	res = 0;

	if (t_flag) {
		if (strlen(path) > strlen(name)) {
			return res;
		}
	}

	name_copy = calloc(PATH_LIMIT, sizeof(char));
	if (!name_copy) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
	path_copy = calloc(PATH_LIMIT, sizeof(char));
	if (!path_copy) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	strcpy(name_copy, name);		    
	strcpy(path_copy, path);
	
	ncptr = name_copy;
	pcptr = path_copy;

	while (1) {
		if ((name_tok = strtok_r(ncptr, "/", &ncptr)) == NULL) {
			break;
		}
		if ((path_tok = strtok_r(pcptr, "/", &pcptr)) == NULL) {
			break;
		}
		res = (strcmp(name_tok, path_tok) == 0);
	}
	free(name_copy);
	free(path_copy);
	return res;
}

