#ifndef UTILITIESH
#define UTILITIESH

uint32_t extract_special_int(char *, int);
int insert_special_int(char *, size_t, int32_t);
void setChksum(Header *);
unsigned int getChksum(Header *); 
void strictCheck(Header *);
int isValid(char *, char *, int);

#endif
