#ifndef TRACEREAD_H_
#define TRACEREAD_H_

#include <stdio.h>
#include "cache.h"

FILE *fp;

int delta;
char opertype;
unsigned long long int addr;
unsigned long long int dummy;
request * req;
query * q;

char * trace_file_name;
char line[256];

query * traceread(unsigned long long int, int);
int traceread_init(char * , unsigned long long int);
int traceread_end(void);


#endif
