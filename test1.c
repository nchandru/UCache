#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

int main()
{

	printf("\n****************************************************************************************\n");
        unsigned long long int addr;
        int ldst, mode, req_q, evict_q, valid, dirty;
        long blk_size, way, set;
        char * name;

        int set_num, tag_num, way_num, i;

	//simulation setup
	debug = 1;
	breakpoint = 1;
	end_time = 10;
	qmax = 1;

        cache *l1; 

        //Cache declaration parameters
        l1 = cache_init("L1", 64, 4, 16); //l1 = init(name, blk_size, way, set); //name, block size, way, set, request queue, evict queue
	l1->next = NULL;        
        printf("\nResting state");
        snapshot(l1); 
       
        manual_set(l1, 2, 2, 5, 1, 0); //int manual_set(cache * cobj, long set_num, long way_num, long tag_num, int valid, int dirty)
        snapshot(l1);

	addr = gen_addr(l1, 2, 2, 5); //Cache, block, set, tag
	request * req = req_init(1, addr, 0, 0); //new populated choice, address, ldst, mode 
	query * q1 = query_init(req, 1, 0, 3); //query * query_init(request * req, int valid, int ack, unsigned long long int iptime

	

	//TODO Make this part of a simulation setup thing
	qlist = malloc(qmax * sizeof(query*));
        for(i=0; i<qmax; i++)
                qlist[i] = malloc(sizeof(query));

	printf("\nQuery array initialised");
	
	qlist[0] = q1;

	simulate(l1);


return 0;
}

