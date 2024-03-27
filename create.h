#ifndef CREATEH
#define CREATEH

#include "header.h"

void setPrefix(char *, Header *);
void writeDirectory(char *, int, int, int);
void writeFile(char *, int, int, int);
void writeHeader(char *, int, int, int);
void createArchive(int, char *[], int, int);

#endif
