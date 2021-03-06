/**************************************************************************************************
TODO

* Configuration files for simulation and caches

*************************************************************************************************/

#include "cache.h"
#include "traceread.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>


int check_policy(cache * cobj, request * req)
{
//returns policy decision. 
return 1;
}

/**************************************************************************************************
TODO: Can implement tail based insertion. Maybe for a later commit

duplicate_request(...)

Used to duplicate a request object. 

IMP: Need to update this for all the data variables in the request packet. 

Arguments:

1) to - The request object to be copied to
Type - Request *

2) from - The request object to copy from
Type - Request *

Return:

The destination copied request object

**************************************************************************************************/


request * duplicate_request(request * to, request * from)
{
	int i;
	//request * req_init(int choice, unsigned long long int addr, int ldst, int mode)
	to = req_init(1, from->addr, from->ldst, from->mode);
	
	for(i=0; i<=from->count; i++) //TODO for multiple cache heirarchy
		to->cache_queue[i]=from->cache_queue[i];
	
	to->next = from->next;

        to->count = from->count;
	to->stage = from->stage;
        to->dispatch_time = from->dispatch_time;
        to->completion_time = from->completion_time;

return to;
}


/**************************************************************************************************

queue_add(...)

Adding an element to the queue

Arguments:

1) cobj - Cache structure object
Type - Cache *

2) ptr - The head of the queue to which the element ahs to eb added
Type - Request *

3) reqin - "Reqest IN" is the request element to be added to the queue

Return:

Type - Integer
The number of requests already in the queue for the cache block address accessed by reqin argument
Usually used when inserting to the in-flight queue
Or for error checking in other queues

**************************************************************************************************/

int queue_add(cache * cobj, request* ptr, request * req) //TODO: Error checking in case there is a request "object" already present in the queue
{
	request * temp;
	temp = ptr->next;
	
	int req_present=0; 
	int set_bits = log(cobj->block_size)/log(2);
	unsigned long long int ones = 0xFFFFFFFFFFFFFFFF;
	
	if(debug>=4) printf("\n%llu: %s - Queue element added with dispatch_time: %llu", global_time, cobj->name, req->dispatch_time);

	if(temp == NULL) //First element is being added. i.e No element in the queue
	{
		req->next = NULL;
		ptr->next = req;
		if(debug>=4) printf("\n%llu: %s - Element added and number of duplicates returned: %d", global_time, cobj->name, req_present);
		return req_present; //successfully added
	}

	while(temp->next!=NULL)
	{
		if(((temp->addr)&(ones<<set_bits)) == ((req->addr)&(ones<<set_bits))) //need to do that block bot truncation thing here
		{
			req_present++; // Count indicating if any request for current address was already preset
		}
		temp=temp->next; //moving pointer to last queue block
	}
	req->next= NULL;
	temp->next=req;

return req_present; //return 0 if no duplicate requests present... Return the number of duplicates otherwise
}

/**************************************************************************************************

queue_stats(...)

Prints the status of all the queues in the cache 

Arguments: 

cobj - Cache structure object pointer
Type - Cache *

Returns:

Nil
**************************************************************************************************/


int queue_stats(cache * cobj)
{
	request * ptr;
	char * qname;
 	int i, count, flag;	
	printf("\n%s Cache queue status (Num elements) -\n", cobj->name);	

	for(i =0; i<4; i++)
	{
		count = 0;
		flag = 0;
		if(i==0) { ptr = cobj->forward; qname = "Forward queue"; }
		else if(i==1) { ptr = cobj->input; qname = "Input queue"; }
		else if(i==2) { ptr = cobj->response; qname = "Response queue"; } 
		else if(i==3) { ptr = cobj->ret; qname = "Return queue"; }

		printf("%s - %d, ", qname, num_queue_elements(ptr));
	}
	printf("\nNumber of requests input till now: %llu", qcount);
	printf("\nNumber of misses: %d", cobj->miss);

return 0;
}



/**************************************************************************************************

queue_remove_timestep(...)

To remove or just return the address of the first element in the queue which matches the timestep.
The input queue needs to maintain duplicates of the requests. Removing them during dispatch would
be incorrect. So, the option argument can be used just to get the pointers reference and duplicated
by the caller. There is an added functionality of lookup in this queue. If the option is 2, the 
function returns the pointer to the request which looks up the address. 

Arguments:

1) ptr - Request queu start pointer
Type - Request *

2) timestep - Current timestep of operation
Type - unsigned long long int

3) option - This is used to say if the referenced request needs to be removed from the queue or
only the address is required. 
0 -> Remove and return
1 -> Persist and return 

Return:

Type - Request *
Pointer to the removed queue element

**************************************************************************************************/

request * queue_remove_timestep(request *ptr, unsigned long long int timestep, int option)
{
	request * temp = NULL;
        request * cur; 
        cur = ptr->next;
        if(cur==NULL) { printf("\nNothing to remove"); return NULL; } //Nothing to remove. Mostly this would be called out of indexing error

        request * prev; // = malloc(sizeof(request));
        prev = ptr;

        while(cur!=NULL)
                {
				if(cur->dispatch_time==timestep) //If address is the same or the request itself is the same, we remove that request from the queue            
				{
					if(option==1) return cur; //The current pointer is returned to just get the address and not to dequeue yet. 
					temp = cur;
					cur=cur->next;
					prev->next= cur;
					break; // Stop when the first request with the current timestep to go is found
				}
				else
				{
					prev  = cur;
					cur = cur->next;
				}
			
                }

return temp;
}


/**************************************************************************************************

TODO See if this can be merged with queue_mremove()

queue_lookup(...)

There is a functionality of lookup in queues. If the option is 2, the function returns the pointer 
to the request which looks up the address.

Arguments:
1) cobj - Cache structure object
Type - Cache *

2) ptr - Pointer of head of queue
Type - Request *

3) req - The request whose address is to be used if multiple blocks are to be removed
Type - Request * req
Pass NULL if only the head is to be removed

Return:

Pointer of queue block that was a hit. NULL if a miss in the queue

**************************************************************************************************/


request * queue_lookup(cache * cobj, request * ptr, request * req)        
{
        request * cur; // = malloc(sizeof(request));
        cur = ptr->next;
	request * temp = NULL;
        if(cur==NULL) { if(debug>=4) printf("\n%llu: %s - Queue_lookup: Nothing to lookup", global_time, cobj->name); return temp; } //Nothing to remove. Mostly this would be called out of indexing error

        request * prev; // = malloc(sizeof(request));
        prev = ptr;
        unsigned long long int ones = 0xFFFFFFFFFFFFFFFF, val1, val2;
        int set_bits = log(cobj->block_size)/log(2); //cobj was passed just for this 

        while(cur!=NULL)
                {
			val1 = ((cur->addr)&(ones<<set_bits));
			val2 = ((req->addr)&(ones<<set_bits));
                        if((val1==val2)) //If address is the same and its a writeback request
                        {
                                return cur;
                                cur=cur->next;
                                prev->next= cur;
                              
                        }
                        else
                        {
                                prev  = cur;
                                cur = cur->next;
                        }

                }

return temp;
}



/**************************************************************************************************

queue_mremove(...)

To remove a request pointed to by the starting pointer argument (ptr). This function can be used to 
remove ALL requests which are for the same cache block.

Arguments:

1) cobj - Cache structure object
Type - Cache *

2) ptr - Pointer of head of queue
Type - Request *

3) req - The request whose address is to be used if multiple blocks are to be removed
Type - Request * req
Pass NULL if only the head is to be removed

Return:

Type - Integer
Number of elements removed is returned

**************************************************************************************************/

int queue_mremove(cache * cobj, request * ptr, request * req) 
{
	//Freeing memory should be done
	int count=0;
	request * cur; // = malloc(sizeof(request));
	cur = ptr->next;
	if(cur==NULL) { printf("\nNothing to remove"); return -1; } //Nothing to remove. Mostly this would be called out of indexing error

	request * prev; // = malloc(sizeof(request));
	prev = ptr; 
	unsigned long long int ones = 0xFFFFFFFFFFFFFFFF;
	int set_bits = log(cobj->block_size)/log(2); //cobj was passed just for this 

	while(cur!=NULL)
		{
			if(((cur->addr)&(ones<<set_bits))==((req->addr)&(ones<<set_bits))) //If address is the same		
			{
				cur=cur->next;
				prev->next= cur;
				count++;
			}
			else
			{
				prev  = cur;
				cur = cur->next;
			}

		}
	
return count;
}

/**************************************************************************************************

check_dispatch_ready(...)

To see if any of the requests are ready to go at the current timestep

Arguments:

1) req - The request starting pointer to be checked for
Type - Request *

2) timestep - The global timestep the iteration is compared against
Type - unsigned long long integer

Return:

Type - Integer
The number of requests that are ready to go for the current timestep 

**************************************************************************************************/

int check_dispatch_ready(request * req, unsigned long long int timestep)
{
	request * ptr;
	ptr = req;
	if(ptr->next == NULL) return 0;
	while(ptr->next!=NULL)
	{
		ptr = ptr->next;
		if(debug>=4) printf("\nDispatch time of current traversal element is: %llu with stage: %d", ptr->dispatch_time, ptr->stage);
		if(ptr->dispatch_time==timestep) 
		{
			if(debug>=4) printf("\n%llu: check_dispatch_ready - Queue element ready for dispatch found", global_time);
			return 1;
		}
	}

return 0;
}


/**************************************************************************************************

num_queue_elements(...)

To find the number of elements in the queue currently 

Arguments:

req - The pointer of the request head
Type - Request *

Return:
Type - int
The number of elements. Duh!

**************************************************************************************************/


int num_queue_elements(request * req) //To check the number of elements in the queue
{
	request * ptr;
	ptr = req;
	
	int num = 0;
	if(ptr->next==NULL) return 0;

	while(ptr->next!=NULL)
	{
		num++;
		ptr = ptr->next;
	}

return num;
}

/**************************************************************************************************

parse(...)

Function to extract set and tag from a request-address associated with a cache configuration. 

Arguments:

1) cobj - Cache structure object
Type - Cache *

2) req - Request where address is provided
Type - Request *

Return:

TL;DR - Return available at nexta->set and nexta->tag

This ones a bit sneaky. The expected return values are a set and a tag associated with the address
for the cache. The dual return is managed by returning an "naddress" structure with a global
object named "nexta" which has data variables "set" and "tag". Therefore, every time parse is 
called, the global "nexta" object's data variables are accessed and the result can be extracted
from there. 

**************************************************************************************************/

int parse(cache * cobj, request * req) 
{
	nexta->set = 0;
	nexta->tag = 0;
	int set_bit_st = log(cobj->block_size)/log(2);
	int set_bit_end = set_bit_st - 1 + (log(cobj->sets)/log(2));
	int tag_bit_st = set_bit_st+1; //recheck this
	int tag_bit_end = pa_size-1;
	
	int i, k;
	for(i=set_bit_st, k=0; i<=set_bit_end; i++, k++)
	{
		nexta->set = (((req->addr>>(i)) & 0x01)<<k)^nexta->set; 			
	}

	for(i=tag_bit_st, k=0; i<=tag_bit_end; i++, k++)
	{
		nexta->tag = (((req->addr>>(i)) & 0x01)<<k)^nexta->tag;			
	}
	

return 0;
}

/**************************************************************************************************

cache_init(...)

To inititalize the cache object 

Arguments:

TODO Finalize the arguments and declare all data variables

Timing things as well.

Return 
Type - Cache * 
The Cache object pointer

**************************************************************************************************/

cache * cache_init(char * name, long block_size, long ways, long sets)
{
	long i;
	cache * cobj = malloc(sizeof(cache));
        cobj->block_size=block_size;
        cobj->ways = ways;
        cobj->sets = sets;
	cobj->name = name;
	cobj->tag_block = malloc(sets * sizeof(blk*));

	cobj->read_latency = 2;
	cobj->write_latency = 5;
	cobj->lookup_latency = 2;

        for(i=0; i<sets; i++)
        {
		cobj->tag_block[i] = malloc(ways * sizeof(blk));
        }

	cobj->input = req_init(0, 0, 0, 0);	
	cobj->forward = req_init(0, 0, 0, 0);
	cobj->ret = req_init(0, 0, 0, 0);
	cobj->response = req_init(0, 0, 0, 0);

	cobj->max_input = 256;
	cobj->max_forward = 256;
	cobj->max_ret = 256;
	cobj->max_response = 256;
	
	cobj->pending_input = 0;
	cobj->pending_forward = 0;
	cobj->pending_ret = 0;
	cobj->pending_response = 0;
	
	cobj->next = NULL;

	cobj->tag_next=0;
	cobj->data_next=0;

	cobj->miss = 0;

	nexta = malloc(sizeof(naddress));


	cobj->lookuptime = 1;
	cobj->loadtime = 2;
	cobj->storetime = 3;
	cobj->victimtime = 1;
	
return cobj;
}

/**************************************************************************************************

query_init(...)

This is to declare the interface between the CPU and the cache. The request is rapped in this 
query object with indication of validity in input so that the cache latched it in. The ack is 
generated to tell the CPU if the request was accepted. 

1) req - the request packt for which the interface is made
Type - Request *

2) valid - To indicate if the query is valid in the current cycle
Type - integer

3) ack - Acknowledge by the cache - used by the CPU
Type - int

Returns:
Type - Query *
Query object

**************************************************************************************************/

query * query_init(request * req, int valid, int ack, unsigned long long int iptime)
{
	query * qobj = malloc(sizeof(query));
	qobj->req = malloc(sizeof(request));
	qobj->req = req;
	qobj->valid=valid;
	qobj->ack;
	qobj->iptime = iptime;

return qobj;
}


/**************************************************************************************************

dump_request_data(...)

To dump all parameters of a request to terminal.

Arguments:
req - The request object
Type - Request *

Returns:
Nil

**************************************************************************************************/


int dump_request_data(request * req)
{
	if(req==NULL)
	printf("\n----------\nRequest Data Dump\n----------\n");
	printf("\nID: %llu", (unsigned long long int) req );
//	printf("\nAddress: ");
//	print_bits(req->addr);
	printf("\nCount: %d", req->count);
	printf("\nLDST: %d", req->ldst);
	printf("\nMode: %d", req->mode);
	printf("\nStage: %d", req->stage);
	printf("\nDispatch time: %llu", req->dispatch_time);
	printf("\nCompletion time: %llu", req->completion_time);
	

return 0;
}


/**************************************************************************************************

req_init(...)

This is the "constructor" alternative for initialising request objects.

IMP: Please update this if adding more data variables to Request structure

Arguments:

1) option - Type of initialisation required
1 -> Full init by considering input arguments
0 -> Dummy init with just mem alloc

2) addr - Address this request corresponds to
Type - unsigned long long int

3) ldst - If the operation is a load or a store
Type - Integer
0 -> LOAD
1 -> STORE

4) mode - Can be used to say what the type of the request is
Type - Integer
0 -> Normal LD/ST (original or forwarded, doesn't matter)
1 -> WB (Write back - Due to Dirty)
2 -> Eviction
//TODO TODO can mode be replaced completely by stage?

Return:
Type - Request *
Oven-baked fresh return object

//TODO 
1) Changing ldst and mode to an ENUM
2) Finalising timing stuff
3) Add the data init for forwarded too - Forwarded = 0 means original REQ. 1 means forwarded. 
need this indication for the update function. 

**************************************************************************************************/


request * req_init(int option, unsigned long long int addr, int ldst, int mode)
{
	int i;
	request * temp;
	switch(option)
	{
	case 1:
		temp = malloc(sizeof(request));
		temp->addr= addr;
		temp->count = -1;
		temp->ldst = ldst;
		temp->mode = mode;
		temp->cache_queue  = malloc(MAX_LEVELS * sizeof(cache*)); //TODO: plus one becasue planning to add CPU object as well
		for(i=0; i<MAX_LEVELS; i++)
			temp->cache_queue[i] = malloc(sizeof(cache));
		temp->next = NULL;
		
		temp->dispatch_time = 0;
		temp->completion_time = 0;

	break;

	case 0:
	        temp = malloc(sizeof(request));
	        temp->next = NULL;
	break;
	}

return temp;
}

/**************************************************************************************************

snapshot(...)

To print the current valid and dirty status of every block in the cache on the console. 

Arguments:

cobj - Cache structure pointer
Type - Cache *

Returns:
Zero. 

TODO 
1) Exporting to a file 
2) Including queue statuses too

**************************************************************************************************/


int snapshot(cache * cobj)
{
	int i, j;
	printf("\n------------------------------\nCache snapshot of %s \n------------------------------\n", cobj->name);

	for(i=0;i<cobj->sets;i++)
	{
		printf("\nSET %d ",i);
		for(j=0;j<cobj->ways;j++)
			printf("|v=%d, d=%d, %lu| ", cobj->tag_block[i][j].valid, cobj->tag_block[i][j].dirty, cobj->tag_block[i][j].counter);
	}

	queue_stats(cobj);
	printf("\nNext Tag Available: %llu, Next Data Available: %llu", cobj->tag_next, cobj->data_next);

	printf("\n-----------------------------\n");
return 0;
}

/**************************************************************************************************

find_victim(...)

Looks for the LRU block and returns its way number as the victim.

Arguments:

1) cobj - Cache structure object pointer
Type - Cache *

2) req - Request structure object pointer
Type - Request *


Returns:
Type - Long
Way number

-1 if some error in finding max. Should not happen ideally

**************************************************************************************************/


long find_victim(cache * cobj, request * req)
{
        if(debug>=5) printf("\n%s: Inside find_victim", cobj->name);
        //print_bits(req->addr);

        //TODO: Recheck this too. Return with best bet
        parse(cobj, req);
        int set = nexta->set;
        unsigned long max = 0;
        long i, way_id=-1;
        if(set>=cobj->sets) { printf("\n%s: find_victim() Out of bounds SET accessed... Exiting", cobj->name); return -2; } //exit on incorrect argument passed
        
        for(i=0; i<cobj->ways; i++)
        {
                if(cobj->tag_block[set][i].valid==0) { return i; } //if any invalid block present, it becomes the victim        

                if(max<=cobj->tag_block[set][i].counter) //find the max counter in the objects
                {
                        max=cobj->tag_block[set][i].counter;
                        way_id=i;
                }
        }
        return way_id;  
}


/**************************************************************************************************

lookup(...)

To perform a lookup in a cache for a particular request. 

Arguments:

1) cobj - Cache structure pointer
Type - Cache *

2) req - Request pointer which has the address to lookup
Type - Request *

Return:
Type - long

On HIT -> Way number 
On MISS -> -1 
Incorrect Argument -> -2

**************************************************************************************************/

long lookup(cache * cobj, request * req)
{
        //printf("\n%s: Inside lookup", cobj->name);
        //print_bits(req->addr);
        unsigned long long int ones = 0xFFFFFFFFFFFFFFFF;
        long i;
        parse(cobj, req);
        if(nexta->set>=cobj->sets) return -2; 
        int block_bits = log(cobj->block_size)/log(2);

        if(debug>=5) printf("\n%s: SET lookup is - %llu", cobj->name, nexta->set );  
        for(i=0; i<cobj->ways; i++)
                if(( (cobj->tag_block[nexta->set][i].addr) & (ones<<block_bits)) == ((req->addr) & (ones<<block_bits)) && (cobj->tag_block[nexta->set][i].valid==1)) return i; //return the WAY where it was found

return -1; //None found. Return negative one
}


/**************************************************************************************************
TODO Under construction
Can lookup be merged into this?

update(...)

MAJOR responsibility here. This function looks at the request passed, checking its mode to decide
what is to be done. 

Arguments:

1) cobj - Cache structure object pointer
Type - Cache *

2) req - Request pointer
Type - Request *

3) way - The way top update with the current request. The way is found using lookup or find_victim
Type - Request *

Return:

//TODO

**************************************************************************************************/


int update(cache * cobj, request * req, long way) //to update the SET counters when a cache block in a particular WAY is accessed for LRU
{
	//MAJOR responsibility here. 
	// ppp Add cases of changinf valid/dirty bits here

	if(debug>=6) printf("\n%llu: %s - Inside update()", global_time, cobj->name);
	long victim =0;
	parse(cobj, req);
	int set = nexta->set;
	if(set>=cobj->sets) { printf("\n%s: update() Out of bounds SET accessed... Exiting", cobj->name); return -2; } //exit on incorrect argument passed

	switch(req->stage)
	{
		case 11: //Being returned FROM cache to higher level cache (@ return queue)
			if(req->ldst==1) //ST operation and it was a HIT
				cobj->tag_block[set][way].dirty=1;
			cobj->tag_block[set][way].valid = 1;
			cobj->tag_block[set][way].addr = req->addr;
		break;

		case 23: //Request being returned TO cache from lower level cache (@ response queue)
			if(req->ldst==1)
				cobj->tag_block[set][way].dirty=1;
			cobj->tag_block[set][way].valid = 1;	
			cobj->tag_block[set][way].addr = req->addr;
		break;

		default:
			printf("\nInvalid mode selected for request...Exiting");
			exit(0);
		break;
	}

	long i;
	
//	print_count_values(cobj, set);

	for(i=0; i<cobj->ways; i++)
	{
		if(i==way) cobj->tag_block[set][i].counter=0;
		else cobj->tag_block[set][i].counter++;
	}
	
//	print_count_values(cobj,set);

return 1;
}

/**************************************************************************************************

TODO see if this can be replaced with just a variable storing max val

find_max_dispatch(...)

Function to find the max dispatch time in a particular request queueu

Arguments:

req - The head pointer of the request queue
Type - Request *

Return:
Type - unsigned long long int
The max value of the dispatch in that queue


**************************************************************************************************/



int find_max_dispatch(request * req) //To check the number of elements in the queue
{
        request * ptr;
        ptr = req;
	
        int num = 0;
        if(ptr->next==NULL) return 0;

	unsigned long long int max = 0;
        while(ptr->next!=NULL)
        {
                if(ptr->dispatch_time>max)
			max = ptr->dispatch_time;
                ptr = ptr->next;
        }

return max;
}


unsigned long long int max_cmp(unsigned long long int a, unsigned long long int b)
{
	return a>=b?a:b;
}


long optime(cache * cobj, request * req)
{
	//returns time required to do this function in the current cache block
	
	switch(req->stage)
	{
	//Time for:

	case 0: //Tag lookup from input queue

		return cobj->lookuptime;
	break;


	case 1: //Data access (LOAD)

		return cobj->loadtime;
	break;

	case 6: //Data access (STORE)

		return cobj->storetime;
	break;

	case 21: //Looking up and finding a victim

		return cobj->victimtime;	
	break;

	case 22: //Data eviction (Fetch and write)

		return cobj->loadtime + cobj->storetime;
	break;
	
	default:
		assert(0);
	break;

	}

	return 3;
}

/**************************************************************************************************

schedule_queue(...)

Schedule queue is an important function which uses the arguments to decide where and 
when the request passed is to be scheduled. The queue_select argument hints on which output queue
needs to be considered while deciding scheduling.  

Arguments:

1) cobj - Cache structure object pointer
Type - Cache *

2) req - The request under consideration
Type - Request *

3) queue_select - just an identifier to show what queue is being worked on
Type - Integer

0 -> Forward queue
1 -> Input queue
2 -> Response queue
3 -> Return queue

Return:

TODO
1) queue_select can be ENUM

**************************************************************************************************/

int schedule_queue(cache * cobj, request * req, int queue_select)
{
	request * insr;
	long removed = 0;
	switch(queue_select)
	{
		case 0: //Forward queue
			if(debug>=3) printf("\n%llu: %s - Inserting to forward queue", global_time, cobj->name);	
			insr = duplicate_request(insr, req);	
			insr->dispatch_time = max_cmp(global_time, max_cmp(insr->completion_time, find_max_dispatch(cobj->forward)))+1; 
			queue_add(cobj, cobj->forward, insr);
			cobj->pending_forward++;
		break;

		case 1: //Input queue

			//If request is writeback, check policy!=exclusive and add to forward queue directly. Otherwise, schedule for writebactime(cobj)

			if(check_policy(cobj,req)==0) //0 means not in policy 
			{
				if(debug>=4) printf("\n%llu: %s - Input queue insert - Queueing to forward.", global_time,cobj->name);
				req->completion_time = 0;
				schedule_queue(cobj, req, 0); //Pushing to forward queue if not in policy
				break;
			}
			
			insr = duplicate_request(insr, req);
			insr->dispatch_time = 0;	 //making this default zero to not allow duplicates
			insr->stage = 0;
			
			if(queue_lookup(cobj, cobj->input, req)==NULL) //Do all this only if the request is original 
			{
				if(debug>=4) printf("\n%llu: %s - ORIGINAL request", global_time,cobj->name);
				insr->dispatch_time = max_cmp(global_time, cobj->tag_next) + 1; // (+wire_latency?) 
				insr->completion_time = insr->dispatch_time + optime(cobj, insr); //optime for tag lookup
				cobj->tag_next = insr->completion_time;
				cobj->pending_input++;
			}

			else
			{

				if(debug>=4) printf("\n%llu: %s - DUPLICATE request", global_time,cobj->name);
				cobj->miss++;
				insr->stage=5; //Duplicate
			}			

					
	
			if(debug>=3) printf("\n%llu: %s - Inserting to Input queue", global_time, cobj->name);	
			if(debug>=4) printf("\nDispatch alloted is: %llu. New tag avail is %llu.", insr->dispatch_time, cobj->tag_next);
			queue_add(cobj, cobj->input, insr);
//			free(req); //TODO check later if this indeed is this function's headache
		break;

		case 2: //Response queue
			
			if(debug>=3) printf("\n%llu: %s - Inserting to Response queue", global_time, cobj->name);	
			if(check_policy(cobj, req)==0)
			{
				schedule_queue(cobj, req, 3); //Pushing to return if not in policy 
				break;
			}

			insr = duplicate_request(insr, req);
			insr->dispatch_time = max_cmp(max_cmp(global_time,req->dispatch_time), find_max_dispatch(cobj->response))+1; //Queue traversal could be avoided
//			insr->completion_time = insr->dispatch_time + optime(cobj,req); //Doesn't matter actually
			//cobj->data_next = insr->completion_time;
			insr->stage=21;
			queue_add(cobj, cobj->response, insr);
			cobj->pending_response++;
			free(req);
		break;

		case 3: //Return queue
			
			if(debug>=3) printf("\n%llu: %s - Inserting to Return queue", global_time, cobj->name);	
			// dispatch = max(completion_time+1,max(return_queue)+1)
			insr = duplicate_request(insr,req);
			insr->dispatch_time = max_cmp(global_time, max_cmp(req->completion_time, find_max_dispatch(cobj->ret)))+1;
                        queue_add(cobj, cobj->ret, insr);
                        cobj->pending_ret++;
			//free(req);			

		break;

		default:
			//TODO Make the error enums
		break;
			
	}

return 0;
}



/**************************************************************************************************

dispatch_update(...)

TO update the dispatch_times of the elements in a queue due to a premature removal (ex. from
output queue during lookup) or hit/miss scenarios (ex in the input queue)

Arguments:

1) ptr - Start pointer of queue
Type - Request *

2) start - The change is for requests whose dispatch_times are greater than start
Type - unsigned long long int

3) delta - the change in dispatch_times in for these requests
Type - long

Return:

TODO

**************************************************************************************************/

int dispatch_update(request * ptr, unsigned long long int start, long delta)
{
	request * cur; // = malloc(sizeof(request));
	cur = ptr->next;
	request * temp = NULL;
	if(cur==NULL) { printf("\nDispatch update: Nothing to lookup"); return 0; } //Nothing to remove. Mostly this would be called out of indexing error

	request * prev; // = malloc(sizeof(request));
	prev = ptr;
	
	while(cur!=NULL)
		{
			if(cur->dispatch_time>=start) //If address is the same             
			{
				cur->dispatch_time += delta;
				cur->completion_time += delta;
			}
				prev  = cur;
				cur = cur->next;
		}
return 0;
}


/**************************************************************************************************

make_packet_update(...)

Will make a request packet from a memory location for purposes of eviction etc. This will also 
update the victim location with the input request.

Arguments:

1) cobj - Cache structure object pointer
Type - cache *

2) ireq - Input request against which eviction is decided
Type - Request *

3) mode - The type of packet to be made
Type - Integer

TODO Third argument could be "stage" instead

Returns:
Type - Request *
The pointer to the newly generated request

**************************************************************************************************/

request * make_packet_update(cache * cobj, request * ireq, int mode)
{
	long victim;
	request * req;
	victim = find_victim(cobj, ireq);
	parse(cobj, ireq);
	if(debug>=5) printf("\nCalling update from make_packet_update");
	if ((cobj->tag_block[nexta->set][victim].dirty==0) || (cobj->tag_block[nexta->set][victim].valid==0)) //Dropping cache entry if invalid or clean during eviction
	{
		//update(cobj, ireq, victim);
		return NULL;
	}
	req = req_init(1, cobj->tag_block[nexta->set][victim].addr, ireq->ldst, mode); //Third argument is redundant actually. This would completely rely on the stage indicating eviction
	//update(cobj, ireq, victim);

return req;
}


/**************************************************************************************************

dispatch_queues(...)

This function, as opposed to a consolidated one to process all of the queues is to have a 
flexibility to reorder the priority of execution. 

IMP: This is assuming that the DRAM controller has no stalling for input requests. i.e The Cache
controller does not have to ask for a permission before sending a request to DRAM. 

Arguments:

1) cobj - Cache object 
Type - Cache * 

2) timestep - Global timestep for progressing
Type - Unsigned long long int

3) queue_select - Variable to select which queue to work on 
Type - Integer
0 -> Forward queue
1 -> Input queue
2 -> Response queue
3 -> Return queue

Return:

//TODO


**************************************************************************************************/

query * dispatch_queues(cache * cobj, unsigned long long int timestep, int queue_select)
{
	//look for types
	request * req;
	request * lookupreq;
	request * insr;
	request * dummy; //For the return queue
	long lookup_way, victim, type;
	
	switch(queue_select)
	{
		case 0: //Forward queue
			
			if(cobj->pending_forward<=0) break; //Nothing to forward			
			if(debug>=3) printf("\n%llu: %s - Dispatching Forward queue", global_time, cobj->name);	
			assert(cobj->pending_forward == num_queue_elements(cobj->forward));
	
			if(debug>=4) printf("\n%llu: %s - Checking dispatch ready from forward queue", global_time, cobj->name);

			while(check_dispatch_ready(cobj->forward, timestep)!=0)	//To flush out all of the requests ready to go at current time step
			{
				if(cobj->pending_forward<=0) break; // Redundant actually

				if(cobj->next == NULL)
				{
					if(debug>=3) printf("\nDRAM access");
					dram_access(queue_remove_timestep(cobj->forward, timestep, 0));
				}
				
				else
				{
					if(access_perm(cobj->next)!=0)  //If the next level is busy and the permission is denied, Update the counters and exit
					{
						dispatch_update(cobj->forward, timestep, 1);	
						assert(cobj->pending_forward == num_queue_elements(cobj->forward));
						break;
					}

					schedule_queue(cobj->next, queue_remove_timestep(cobj->forward, timestep, 0), 1); //Push to return queue of next level
				}
				cobj->pending_forward--;
			}

			if(debug>=1) assert(cobj->pending_forward == num_queue_elements(cobj->forward));
		

		break;

		case 1: // Input queue
//			if(cobj->pending_input<=0) break; //Nothing in the input queue
			if(debug>=3) printf("\n%llu: %s - Dispatching Input queue", global_time, cobj->name);	
			if(debug>=10) printf("\n%llu: %s - Checking dispatch ready from input queue", global_time, cobj->name);

			while(check_dispatch_ready(cobj->input, timestep)!=0)
			{
				if(debug>=2) printf("\n%llu: %s - Someone is ready in the input dispatch queue", global_time, cobj->name);
				
				if(debug>=3) printf("\n%llu: %s - Going perform lookup now", global_time, cobj->name);
				lookup_way = -1;
				req = queue_remove_timestep(cobj->input, timestep, 1); //Retain and return from input queue
				
				if(req->stage==0) //This means its a tag lookup
				{
					if(debug>=3) printf("\n%llu : %s - Its a tag lookup", global_time, cobj->name);
					lookup_way = lookup(cobj,req);
					if(lookup_way == -2) { printf("\n%llu: %s - Error in value passed as address.... Exiting", timestep, cobj->name); exit(0);}
					
					else if(lookup_way>=0) //Cache hit
					{
						if(debug>=3) printf("\n%llu: %s - Its a tag hit!", global_time, cobj->name);
						req->dispatch_time = max_cmp(req->completion_time, max_cmp(cobj->data_next, global_time)) + 1; 
						if(req->ldst==0) req->stage=1; //LD operation
						else if(req->ldst==1) req->stage = 6; //ST operation
						req->completion_time = req->dispatch_time + optime(cobj, req); //optime for data lookup
						cobj->data_next = req->completion_time;
						if(debug>=4) printf("\nDispatch alloted is: %llu. New data avail is %llu.", req->dispatch_time, cobj->data_next);
					}

					else //Looking into forward queue now
					{
						lookupreq = queue_lookup(cobj, cobj->forward, req);
                                                if(lookupreq!=NULL)  //Forward queue look up was a hit
						{
							if(debug>=4) printf("\n%llu: %s - FORWARD QUEUE HIT", global_time, cobj->name); //TODO Verbose
							req->dispatch_time = max_cmp(req->completion_time, max_cmp(cobj->data_next, global_time)) +1;
							req->stage=2; // Using to say that the lookup happened on the forward queue
							req->completion_time = req->dispatch_time + optime(cobj, req);
							assert(0);
						}

						else //Miss
						{
							if(debug>=3) printf("\n%llu: %s - Its a tag MISS", global_time, cobj->name);
							if(req->stage==0) //TODO Was mode here earlier 
							{
								cobj->miss++;
								req->count++;
								req->cache_queue[req->count]=cobj;
								req->stage=4;
								if(debug>=5) printf("\nAdding cache ID to request history as: %s", req->cache_queue[req->count]->name);
								schedule_queue(cobj, req, 0); //Send to forward queue if ld/st lookup failed
								req->dispatch_time = 0; //Making the dispatch at input queue as zero
							}
						}

					}
				}
		
				else if((req->stage==1) || (req->stage==6)) //Bank hit - Data handling 
				{	//Normal LD/ST
	 				if(debug>=3) printf("\n%llu: %s - Data handling under progress", global_time, cobj->name);

					if(debug>=4) 
					{
						printf("\nIt was a LD/ST HIT for request with address:");
						print_bits(cobj, req->addr); 
					}

					//hit_hook(cobj)
					req->dispatch_time=0;
					req->stage=11; //To indicate that the request is a hit and being returned
					schedule_queue(cobj, req, 3); //Pushing to return queue

				}
		
				else if(req->stage==2) //Forward queue hit
				{	//Lookup was hit in the forward queue
					lookupreq = queue_lookup(cobj, cobj->forward, req);
					if(lookupreq==NULL) //Packet left before reading it. So it is queued back
					{
						printf("\n%llu: %s - Packet left from forward queue before you could read it...Oops!", timestep, cobj->name);
						req->stage=0;
						req->dispatch_time = cobj->tag_next + 1; // (+wire_latency?) 
						req->completion_time = insr->dispatch_time + optime(cobj, insr);
						cobj->tag_next = insr->completion_time;
					}
					else
					{
						//Increment hit counter here
						insr = make_packet_update(cobj, lookupreq, type); 
						queue_mremove(cobj, cobj->forward, lookupreq);

						//change property and mode of insr
						
						schedule_queue(cobj, insr, 0); //Schedule victim to forward queue
						
						if(req->ldst==0) //If load request, return the packet
						{
							//update properties of req
	                                                schedule_queue(cobj, req, 3); //Pushing to return queue
						}

					}
				
				}
				
				else if(req->stage==3) //Writeback operation
				{
                                                insr = make_packet_update(cobj, req,type); 
						if(debug>=5) printf("\nUdate being called from input queues writeback operation");
                                                update(cobj, req, victim);
						schedule_queue(cobj, req, 0); //To send the original request forward as well as evicted one					
                                                schedule_queue(cobj, insr, 0); //Schedule to forward queue
				}
				

			}
		break;

		case 2: //Response queue

			if(cobj->pending_response<=0) break;
			if(debug>=3) printf("\n%llu: %s - Dispatching Response queue", global_time, cobj->name);	
			if(debug>=10) printf("\n%llu: %s - Checking dispatch ready from response queue", global_time, cobj->name);

			while(check_dispatch_ready(cobj->response, timestep)!=0)
			{

				lookup_way = -1;
				req = queue_remove_timestep(cobj->response, timestep, 1); //Retain and return from input queue

				if(req->stage==21) //This means the request has been temporarily returned. tag lookup scheduling yet to be done
				{
					req->dispatch_time = max_cmp(max_cmp(cobj->tag_next, req->dispatch_time), global_time) + 1;
					cobj->tag_next = req->dispatch_time + optime(cobj, req); //tag lookup and victim finding time
					req->completion_time = cobj->tag_next;
					req->stage=22;
					continue;
				}
				
				else if(req->stage==22) 
				{
					//req->dispatch_time += optime(cobj,req); /*use optime for stage 22*/ 
					req->dispatch_time = max_cmp(max_cmp(cobj->data_next, req->completion_time), global_time) + 1;
					//find_victim here
					cobj->data_next = req->dispatch_time + optime(cobj,req);
					req->completion_time = cobj->data_next;
					req->stage=23;
					continue;
				}
				
				else //Just data things for eviction and replacement
				{
					insr = make_packet_update(cobj, req, 24); //The last one is stage. But passed into mode for now. TODO TODO 
					cobj->pending_response--;
					if(insr==NULL) //TODO need to revisit this policy as well. For now, dropping packet is it was invalid or clean
					{
						if(debug>=4) printf("\nVictim was invalid or clean so dropping the packet....");
						schedule_queue(cobj, req, 3); //Pushing to return queue
						queue_remove_timestep(cobj->response, timestep, 0); //TODO. Need to revisit how the policy was managed
						continue;
					}	

					if(debug>=4) printf("\nVictim was valid or dirty, so perfrming WB");
					insr->stage=24;
					insr->count++;
	                                insr->cache_queue[req->count]=cobj;

					schedule_queue(cobj, insr, 0); //Pushing to forward queue
					schedule_queue(cobj, req, 3);
					queue_remove_timestep(cobj->response, timestep, 0); //TODO. Need to revisit how the policy was managed
				}

			}
		break;

		case 3: //Return queue

			if(cobj->pending_ret<=0) 
			{
				output->valid = 0;
				break; //Nothing to return 
			}

			if(debug>=3) printf("\n%llu: %s - Dispatching Return queue", global_time, cobj->name);			
			
			while(check_dispatch_ready(cobj->ret, timestep)!=0)
			{
				//TODO Look to see if the policy can be included here rather than during input
				req = queue_remove_timestep(cobj->ret, timestep, 0);		
		
				lookup_way = lookup(cobj, req);
				if(debug>=5) printf("\nUpdate being called from Return queue");
				
				if(req->stage==23) lookup_way = find_victim(cobj, req);

				update(cobj, req, lookup_way); //Update sees ldst, mode and stage to update validity, dirty, counters for LRU
					
				cobj->pending_input-=queue_mremove(cobj, cobj->input, req); //TODO see if this has to be mode specific
				
				if(debug>=4) printf("\nReturn time for a few packets has arrived. Updating and removing them from input queue.");
				
				//if(req->cache_queue[req->count]!=NULL)
				//	schedule_queue(req->cache_queue[req->count], req, 2);		
				//req->count--;  //TODO make this for multiple cache heirarchies

				output->req = req;
				output->valid = 1;
				req->dispatch_time = 0;
				cobj->pending_ret--;

				if(req->ldst==1) //ST operation
				{
					output->valid=0;
			//		free(req);
				}
			}

		break;

		default:
			//TODO Define global error ENUMS
			printf("\n%llu: %s - Error in cache dispatch", timestep, cobj->name);
		break; 

	}


}

/**************************************************************************************************

cpuio(...)

Is for the interface between CPU and the cache (usually L1) defined in simulate(...). It looks 
for output acknowledge and valid to decide if something is available on the bus or if the input 
was queued in the cache. The values are update in the global input and output queues. 

Arguments:
timestep
Type - unsigned long long int

Return
The query if any at the current timestep

TODO File handling integration

**************************************************************************************************/

query * cpuio(unsigned long long int timestep, int input)
{
	int i, flag=0;

	if(input==1) //Output
	{
		if(output->ack==0) { printf("\nStall"); } //TODO Need to perform stall operation. May need file handling here	
		if(output->valid==1 && output->highest==1) 
		{
			//This area is when a particular request is available in the queue
			if(debug>=4) printf("\nGot something back:");
			if(debug>=4) dump_request_data(output->req);	
			output->valid = 0;
		}

	}

	else if(input==0) //Input
	{
		if(traceread(timestep, 0)!=NULL) 
		{
			if(debug>=3) printf("\nNew Query from CPU");
			return traceread(timestep, 1);
		}
	
		else 
		{
			return NULL;
		}
	}

}


/**************************************************************************************************

simulate(...)

1) cobj - Cache structure pointer 
Type - Cache *

TODO
1) DRAM dispatch
2) Return from return queue of L1

**************************************************************************************************/

int simulate(cache * cobj)
{
	input = malloc(sizeof(query));
	output = malloc(sizeof(query));
	qcount = 0;
	long temp_counter=0;

	int temp=1;
	cache * prop = malloc(sizeof(cache));
	prop = cobj;

	while(1)
	{
		if(debug>=0) printf("\nTimestep: %llu, Number of instructions: %llu", global_time, qcount);
	
		output->ack = 1;
		input = cpuio(global_time, 0);
		while(prop!=NULL) 
		{
			while(input!=NULL)
			if((prop==cobj) && (input->valid==1) && (input->iptime==global_time)) //CHECK equality in the last condition. Should it be ==?
			{
				if(access_perm(cobj)!=0) 
				{
					temp=0;					
				}
				else
				{
					temp_counter++;
					qcount++;
					if(debug>=2) printf("\nAccess granted");
					temp=1;
					if(debug>=2) 
					{
						printf("\nRequest arrived with address: ");
						print_bits(cobj, input->req->addr);
					}
					schedule_queue(cobj, input->req, 1); //Inserting to input queue of cache if request was valid at its dispatch time
					input->valid=0;
				}
				input=cpuio(global_time, 0); //To accomodate for multiple inputs
			}

			if(debug>=2) printf("\n**Dispatching forward queue**\n");
			dispatch_queues(prop, global_time, 0); //Forward
			if(debug>=2) printf("\n**Dispatching input queue**\n");
			dispatch_queues(prop, global_time, 1); //Input
			if(debug>=2) printf("\n**Dispatching response queue**\n");
			dispatch_queues(prop, global_time, 2); //Response
			if(debug>=2) printf("\n**Dispatching return queue**\n");

			if(prop==cobj) //If the working object is highest level of cache, May need to consider the 
			{
				output->highest = 1;
				dispatch_queues(prop, global_time, 3); //Return //TODO Need to return here. ALso this works only for L1
				output->ack = temp;
			}
			else 
			{	
				output->highest = 0;
				dispatch_queues(prop, global_time, 3);
			}

			//To calculate the MPKI
			if(temp_counter>=1000)
			{
				RES_FP = fopen(res_file_name, "a+");
				if(RES_FP==NULL) 
				{
					printf("\nUnable to open result file...");
					return 0;
				}

				fprintf(RES_FP, "\n%d", prop->miss);
				if(debug>=0) printf("\nMiss: %d", prop->miss);
				temp_counter=0;
				prop->miss=0;
				fclose(RES_FP);
			}
	
			if(debug>=1) snapshot(prop);
			if(breakpoint>=1) getchar();
			prop = prop->next;

		}	
		cpuio(global_time, 1);
		prop = cobj;
		global_time++;

		if(breakpoint>=2) if(global_time>=end_time) return 0;
	}
}

/**************************************************************************************************
dram_access(...)

TODO

**************************************************************************************************/

int dram_access(request * req)
{

	request * insr;
	/* Handling Writeback operation in this application*/
	if(req->stage==24)
	{
		free(req);
		return 0;
	}		

	cache * llc = malloc(sizeof(cache));
	llc = req->cache_queue[req->count];
	if(debug>=3) printf("\n%llu: %s - Inserting to DRAM queue", global_time, llc->name);
	insr = duplicate_request(insr, req);
	insr->dispatch_time = req->dispatch_time + DRAM_LATENCY;

	schedule_queue(llc, insr, 2); //Pushing to response queue of LLC if LD operation


	free(req);

return 0;
}


/**************************************************************************************************
access_perm(...)

To get permission from the cache if a block can be inserted. This is open to more type of policies. 
At current commit, it grants permission if there's atleast one free block in all four queues.

Arguments

cobj - Cache structure pointer object
Type - Cache *

Returns 
Type - Integer

0 -> Permission granted
1 -> Permission denied
**************************************************************************************************/

int access_perm(cache * cobj)
{
if(debug>=1)
{
	//assert(num_queue_elements(cobj->input)==cobj->pending_input);
	assert(num_queue_elements(cobj->forward)==cobj->pending_forward);
	assert(num_queue_elements(cobj->response)==cobj->pending_response);
	assert(num_queue_elements(cobj->ret)==cobj->pending_ret);
}

if((cobj->pending_input<cobj->max_input) && (cobj->pending_forward<cobj->max_forward) && (cobj->pending_response<cobj->max_response) &&(cobj->pending_ret<cobj->max_ret)) return 0; //Return zero if approved
else return 1; //Return 1 if denied		
}

/**************************************************************************************************

gen_addr(...)

This is a function used for debugginf purposes. The function returns an address for a particular
cache's set and tag. Maybe overkilling with the argumentt types. 

Arguments:
1) cobj - Cache structure pointer object
Type - Cache *

2) block - the block offset
Type - Unsigned long long int

3) set - the set number
Type - Unsigned long long int

4) tag - The tag value
Type - unisgned long long int

Returns
Type - Unisgned long long int
Address corresponding to the passed arguments

**************************************************************************************************/

unsigned long long int gen_addr(cache * cobj, unsigned long long int block, unsigned long long int set, unsigned long long int tag)
{
	if((set>cobj->sets-1) || (block>cobj->block_size-1)) { printf("\n%s: Addressing error. Out of bounds.\n", cobj->name); exit(0);}

	int tag_bit_shift = pa_size -((log(cobj->sets)/log(2))+(log(cobj->block_size)/log(2)));
	int set_bit_shift = log(cobj->block_size)/log(2);

	return ((tag<<(pa_size-tag_bit_shift))|(set<<set_bit_shift)|(block));
}


/**************************************************************************************************
print_bits(...) 

TODO printing in reverse order

To print the bit representation of a passed argument

Arguments:
add - Addres (any value) to be printed
Type - unsigned long long int

Returns 
Nil

**************************************************************************************************/


int print_bits(cache * cobj, unsigned long long int add)
{
	//Warning. This prints bits int he reverse order and truncates the "MSB".. Need to fix later
	printf("\n");
	unsigned long long int addr = add;
	if(addr==0) { printf("0"); return 0;}
	int cnt = pa_size;
	int block_bits = log(cobj->block_size)/log(2);
	int set_bits = block_bits + (log(cobj->sets)/log(2));

	while(1)
	{
		if(cnt==block_bits || cnt==set_bits) printf(" | ");
		if((addr>>(cnt-1)) & 0x01) printf("1");
		else printf("0");
		cnt=cnt-1;
		if(cnt==0) break;
	}

return 0;
}


/**************************************************************************************************

manual_set(...)

used for configuring a cache block manually. This also used for debugging.

Arguments:

1) cobj - Cache structure pointer object
Type - Cache *

2) set_num - The set number under consideration
Type - long

3) way_num - Way number under consideration
Type - long

4) tag_num - The tag number under consideration
Type - long

5) Valid - The valid bit to be set
Type - Int

6) dirty - The dirty bit to be set
Type - Int

Returns 
Nil

**************************************************************************************************/


int manual_set(cache * cobj, long set_num, long way_num, long tag_num, int valid, int dirty)
{
        if(set_num>cobj->sets-1) { printf("\n%s: Addressing error. Out of bounds.\n", cobj->name); exit(0);}
	if(debug>=1) printf("\n%s Manual set performed with address: ", cobj->name);
	cobj->tag_block[set_num][way_num].valid = valid;
	cobj->tag_block[set_num][way_num].dirty = dirty;
        cobj->tag_block[set_num][way_num].addr = gen_addr(cobj, 0,  set_num, tag_num);
	if(debug>=1) print_bits(cobj, gen_addr(cobj, 0, set_num, tag_num));
return 0;
}

