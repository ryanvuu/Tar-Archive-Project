#ifndef EXTRACTH
#define EXTRACTH

#include "header.h"

void restoreTimes(Header *, char *, int);
int checkDirectory(char *);
void extractDirectory(Header *, char *);
void extractSymlink(Header *, char *, int);
void extractFile(int, Header *, char *, long int);
void extractArchive(int, char **, int, int);

#endif
