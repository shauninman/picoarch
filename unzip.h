#ifndef __UNZIP_H_
#define __UNZIP_H_

#include <stdio.h>

int unzip_tmp(FILE *zip, const char **extensions, char *filename, size_t len);
int unzip(FILE *zip, const char **extensions, FILE *dest);

#endif
