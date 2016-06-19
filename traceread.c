#include <stdio.h>
#include <stdlib.h>
#include "traceread.h"
#include "cache.h"
#include <assert.h>

unsigned long long int temp_time;
unsigned long long int count=0;
int init=0;
int allow;
unsigned long long int qm;


int traceread_init(char * filen, unsigned long long int nq)
{
	fp = fopen(filen, "r+");
	if(fp==NULL) 
	{
		printf("\nCouldn't open file.....");
		assert(0);
	}
	
	printf("\nTrace reader initialized and opened: %s with max queries: %llu", filen, nq);

	qm=nq;
	init=1;
	temp_time=0;
	count=0;
	allow=1;
	
	req = malloc(sizeof(request));
        q = malloc(sizeof(query));
        free(req);
        free(q);
	
}

int traceread_end(void)
{
	printf("\nClosing trace file");
	fclose(fp);
	init=0;
	return 0;
}

query * traceread(unsigned long long int global_time, int check)
{

	if(init==0) //To verify that the init was called before traceread
	{
		printf("\nInit for traceread not called first");
		assert(0);
	}

	if(count==qm) 
	{
		printf("\nMax number of queries ended");
		exit(0);
		return NULL; //Keep returning NULL if maximum number of queries has been reached
	}
	
	if(allow==1)
	{
		allow=0;
		if(fgets(line, sizeof(line), fp)==NULL)
		{
			printf("\nEOF reached.. Exiting");
			exit(0);
		}

		else
		 if (sscanf(line,"%d %c", &delta, &opertype) > 0) {
                  if (opertype == 'R') {
                    if (sscanf(line,"%d %c %Lx %Lx", &delta, &opertype, &addr, &dummy) < 1) {
                      printf("Panic.  Poor trace format.\n");
                    }
                  }
                  else {
                    if (opertype == 'W') {
                      if (sscanf(line,"%d %c %Lx", &delta, &opertype, &addr) < 1) {
                        printf("Panic.  Poor trace format.\n");
                      }
                    }
                    else {
                      printf("Panic.  Poor trace format.\n");
                    }
                  }
                }
                else {
                  printf("Panic.  Poor trace format.\n");
                }
		
		temp_time += delta; 
	}
        

	if(temp_time == global_time)
	{
		if(check==1)
		{
			allow=1;
			count++;
			if(debug>=5) printf("\nNew request %llu from trace_read: %llu, %c, %llu", count, temp_time, opertype, addr);
		}
		req = req_init(1, addr, opertype=='R'?0:1, 0); //new populated choice, address, ldst, mode
		q = query_init(req, 1, 0, temp_time); //query * query_init(request * req, int valid, int ack, unsigned long long int iptime
		return q;
	}
	else
	{
		return NULL;
	}
return 0;
}
