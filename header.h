#ifndef HEADERH
#define HEADERH

#define BLOCK_SIZE 512
#define NAME_SIZE 100
#define MODE_SIZE 8
#define UID_SIZE 8
#define GID_SIZE 8
#define SIZE_SIZE 12
#define MTIME_SIZE 12
#define CHKSUM_SIZE 8
#define TYPEFLAG_SIZE 1
#define LINKNAME_SIZE 100
#define MAGIC_SIZE 6
#define VERSION_SIZE 2
#define UNAME_SIZE 32
#define GNAME_SIZE 32
#define DEVMAJOR_SIZE 8
#define DEVMINOR_SIZE 8
#define PREFIX_SIZE 155

#define PERMS_MASK 07777
#define MAX_UID_SIZE 07777777
#define MAX_GID_SIZE 07777777
#define MAX_SIZE_SIZE 077777777777
#define MAX_MTIME_SIZE 077777777777

#define MAGIC_NUM "ustar"
#define VERSION_NUM "00"

#define REG_FLAG '0'
#define SYM_FLAG '2'
#define DIR_FLAG '5'

#define CHKSUM_OFFSET 148

typedef struct header {
	char name[NAME_SIZE];
	char mode[MODE_SIZE];
	char uid[UID_SIZE];
	char gid[GID_SIZE];
	char size[SIZE_SIZE];
	char mtime[MTIME_SIZE];
	char chksum[CHKSUM_SIZE];
	char typeflag[TYPEFLAG_SIZE];
	char linkname[LINKNAME_SIZE];
	char magic[MAGIC_SIZE];
	char version[VERSION_SIZE];
	char uname[UNAME_SIZE];
	char gname[GNAME_SIZE];
	char devmajor[DEVMAJOR_SIZE];
	char devminor[DEVMINOR_SIZE];
	char prefix[PREFIX_SIZE];
	/* The header block is 512 bytes. These are the extra 12 bytes. */
	char extra[12];
} Header;

#endif

