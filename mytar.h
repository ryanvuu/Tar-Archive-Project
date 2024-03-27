/* This header file just defines macros that aren't necessarily
 * related to the header structure. */

#define OCTAL_BASE 8
#define PATH_LIMIT 256
/* The first possible index of argv that represents paths passed
 * as arguments. */
#define ARG_START 3

/* The index of the options */
#define OPTS_INDEX 1
#define NUM_REQ_OPTS 1
#define MAX_OPTS 4
#define MIN_OPTS 2
/* The index in argv of the tar file */
#define TAR_INDEX 2

/* These are used for verbose listing */
#define PERMS_WIDTH 10

/* Index for type of file */
#define TYPE_INDEX 0
/* Indices for user, group, and other permissions */
#define USR_R 1
#define USR_W 2
#define USR_X 3
#define GRP_R 4
#define GRP_W 5
#define GRP_X 6
#define OTH_R 7
#define OTH_W 8
#define OTH_X 9

#define OWNGRP_WIDTH 17
#define SIZE_WIDTH 8
#define MTIME_WIDTH 16

/* These are for converting the tm struct members into actual numbers.
 * The ones that I am using are tm_mday, tm_mon, and tm_year. The 
 * latter two give an integer relative to January and relative to 
 * 1900, respectively, according to the man page. */
#define JANUARY 1
#define REL_YEAR 1900

