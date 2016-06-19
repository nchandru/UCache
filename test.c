//To check if the cache miss is propagated to DRAM/Next level of cache


#include "cache.h"
#include "traceread.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

int main( int argc, char * argv[])
{
	if(argc!=3) 
	{
		printf("\nWrong number of arguments provided");
		return 0;
	}

	printf("\n**************************** BEGIN TEST ******************************\n");
        unsigned long long int addr;
        int ldst, mode, req_q, evict_q, valid, dirty;
        long blk_size, way, set;
        char * name;

	qmax = 300000;
	
        int set_num, tag_num, way_num, i;

	//file_name = "/home/chandru/benchmark/benchmark_bt";	
	
	res_file_name=argv[2];
	trace_file_name=argv[1];

	printf("\nOpening result file: %s", res_file_name);

	traceread_init(trace_file_name, qmax);
	
	//simulation setup
	debug = 0;
	breakpoint = 0;
	end_time = 3000;
	
	//TODO Make this part of a simulation setup thing
        cache *l1; 

        //Cache declaration parameters
	//32 KB Cache, 4 Way, 16 sets, 64 blocks per cache line
        l1 = cache_init("L1", 64, 4, 512); //l1 = init(name, blk_size, way, set);
	l1->next = NULL;        
        printf("\nResting state");
        
	snapshot(l1);

	simulate(l1);

	traceread_end();

return 0;
}

