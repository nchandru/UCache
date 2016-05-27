/**************************************************************************************************

Functionalities to remember:

TODO
1) While asking for permission to send to next cache, both input AND forward queues of the next 
cache is to be checked for a free spot. When I think of it now, I feel we shoudl check for all
queues. 

2) Generic queue removal and counter update etc is taken care by dispatcher and NOT by scheduler. 
The scheduler only takes care of adding and its counters

3) Configuration files for simulation and caches

DONE
1) When pushed to input queue. The queue looks at policy and automatically forwards to forward
queue if WB packet is not inclusive. The higher levels dont maintain the policies. Only the 
lower levels do. 

2) The update function takes care of eviction packet. 

//Check if a strcuture type of CPU can be added during cache init? Or somewhere
//Need to update input counter only if the request was for a different cache block
//Verbose modes

Some global variables:

1) nexta -> Parsed address stored here after calling parse(...)

2) global_time -> Clock timestep for simulator

3) breakpoint -> Defines where the code should break. Incremental
0 -> Default
1 -> Waits for keyboard input after every cycle

4) debug -> For printing stuff. Incremental
0 -> Default
1 -> Cache snapshots on every cycle 
2 -> Request data is dumped 

**************************************************************************************************/

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define pa_size 64
#define MAX_LEVELS 3



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
TODO Need to see if this can be done using memory Syscalls. Memset like?

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
	
	for(i=0; i<=from->count; i++)
		to->cache_queue[i]=from->cache_queue[i];
	
	to->mode = from->mode;
	to->next = NULL;

/*	//Timing aliasing

	to->dispatch_time=from->access_time;
	to->return_time=from->return_time;
*/
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
	
	if(temp == NULL) //First element is being added. i.e No element in the queue
	{
		printf("\nFirst element was added");
		req->next = NULL;
		ptr->next = req;
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
        if(cur==NULL) { printf("\nNothing to lookup"); return temp; } //Nothing to remove. Mostly this would be called out of indexing error

        request * prev; // = malloc(sizeof(request));
        prev = ptr;
        unsigned long long int ones = 0xFFFFFFFFFFFFFFFF;
        int set_bits = log(cobj->block_size)/log(2); //cobj was passed just for this 

        while(cur!=NULL)
                {
                        if(((cur->addr)&(ones<<set_bits))==((req->addr)&(ones<<set_bits))) //If address is the same             
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

	if(ptr->next == NULL) return -1;

	while(ptr->next!=NULL)
	{
		if(ptr->dispatch_time==timestep) return 1;
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

	cobj->max_input = 32;
	cobj->max_forward = 32;
	cobj->max_ret = 32;
	cobj->max_response = 32;
	
	cobj->pending_input = 0;
	cobj->pending_forward = 0;
	cobj->pending_ret = 0;
	cobj->pending_response = 0;
	
	cobj->next = NULL;

	cobj->tag_next=0;
	cobj->data_next=0;

	nexta = malloc(sizeof(naddress));

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
	qobj->req=req;
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
	printf("\nAddress: ");
	print_bits(req->addr);
	printf("\nCount: %d", req->count);
	printf("\nLDST: %d", req->ldst);
	printf("\nMode: %d", req->mode);
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
			printf("|v=%d, d=%d| ", cobj->tag_block[i][j].valid, cobj->tag_block[i][j].dirty);
	}

	queue_stats(cobj);

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

**************************************************************************************************/


long find_victim(cache * cobj, request * req)
{
        printf("\n%s: Inside find_victim", cobj->name);
        //print_bits(req->addr);

        //TODO: Recheck this too. Return with best bet
        parse(cobj, req);
        int set = nexta->set;
        unsigned long max = 0;
        long i,way_id=-1;
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

        printf("\n%s: SET lookup is - %llu", cobj->name, nexta->set );  
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

	printf("\n%s: Inside update()", cobj->name);

	parse(cobj, req);
	int set = nexta->set;
	if(set>=cobj->sets) { printf("\n%s: update() Out of bounds SET accessed... Exiting", cobj->name); return -2; } //exit on incorrect argument passed

	switch(req->mode)
	{
		case 0: //Normal LD/ST
			if(req->ldst==1) //ST operation
				cobj->tag_block[set][way].dirty=1;
			cobj->tag_block[set][way].valid = 1;
			cobj->tag_block[set][way].addr = req->addr;
		break;

		case 1: //Eviction

		break;

		case 2: //Write back

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
	
			insr = duplicate_request(insr, req);	
			insr->dispatch_time = max_cmp(req->completion_time, find_max_dispatch(cobj->forward))+1; 
			queue_add(cobj, cobj->forward, insr);
			cobj->pending_forward++;
		break;

		case 1: //Input queue

			//If request is writeback, check policy!=exclusive and add to forward queue directly. Otherwise, schedule for writebactime(cobj)

			if(check_policy(cobj,req)==0) //0 means not in policy 
			{	
				req->completion_time = 0;
				schedule_queue(cobj, req, 0); //Pushing to forward queue if not in policy
				break;
			}
			
			insr = duplicate_request(insr, req);
			insr->dispatch_time = -1;	 //making this default zero to not allow duplicates
			insr->stage = 0;

			if(queue_lookup(cobj, cobj->input, req)==NULL) //Do all this only if the request is original 
			{
				insr->dispatch_time = max_cmp(global_time, cobj->tag_next) + 1; // (+wire_latency?) 
				insr->completion_time = insr->dispatch_time + optime(cobj, insr);
				cobj->tag_next = insr->completion_time;
				cobj->pending_input++;
			}
			queue_add(cobj, cobj->input, insr);
			free(req);
		break;

		case 2: //Response queue

			if(check_policy(cobj, req)==0)
			{
				schedule_queue(cobj, req, 3); //Pushing to return if not in policy 
				break;
			}

			insr = duplicate_request(insr, req);
			insr->dispatch_time = cobj->data_next+1;
			insr->completion_time = insr->dispatch_time + optime(cobj,req);
			cobj->data_next = insr->completion_time;
			queue_add(cobj, cobj->response, insr);
			cobj->pending_response++;
			free(req);
		break;

		case 3: //Return queue

			// dispatch = max(completion_time+1,max(return_queue)+1)
			insr = duplicate_request(insr,req);
			insr->dispatch_time = max_cmp(req->completion_time, find_max_dispatch(cobj->ret))+1;
                        queue_add(cobj, cobj->ret, insr);
                        cobj->pending_ret++;
//			free(req);			

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
	if(cur==NULL) { printf("\nNothing to lookup"); return 0; } //Nothing to remove. Mostly this would be called out of indexing error

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
	req = req_init(1, cobj->tag_block[nexta->set][victim].addr, ireq->ldst, mode);
	update(cobj, ireq, victim);

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
	long lookup_way, victim, type;
	
	switch(queue_select)
	{
		case 0: //Forward queue
		
			assert(cobj->pending_forward == num_queue_elements(cobj->forward));
	
			if(cobj->pending_forward<=0) break; //Nothing to forward			
		
			while(check_dispatch_ready(cobj->forward, timestep)!=0)	//To flush out all of the requests ready to go at current time step
			{
				if(cobj->pending_forward<=0) break; // Redundant actually

				if(cobj->next == NULL)
				{
					dram_access();
	                                //dram_access(queue_remove_timestep(cobj->forward, timestep)); //TODO make this function
					//If its a clean writeback, just drop it
				}
				
				else
				{
					if(access_perm(cobj->next)!=0)  //If the next level is busy and the permission is denied, Update the counters and exit
					{
						dispatch_update(cobj->forward, timestep, 1);	
						assert(cobj->pending_forward == num_queue_elements(cobj->forward));
						break;
					}

					schedule_queue(cobj->next, queue_remove_timestep(cobj->forward, timestep, 0), 1); 
				}
				cobj->pending_forward--;
			}
	
			assert(cobj->pending_forward == num_queue_elements(cobj->forward));
		

		break;

		case 1: // Input queue
	
			if(cobj->pending_input<=0) break; //Nothing in the input queue
	
			while(check_dispatch_ready(cobj->input, timestep)!=0)
			{

				if(cobj->pending_input<=0) break; //Again, redundant
				
				lookup_way = -1;
				req = queue_remove_timestep(cobj->input, timestep, 1); //Retain and return from input queue
				
				if(req->stage==0) //This means its a tag lookup
				{
					lookup_way = lookup(cobj,req);
					if(lookup_way == -2) { printf("\n%llu: %s - Error in value passed as address.... Exiting", timestep, cobj->name); exit(0);}
					
					else if(lookup_way>=0) //Cache hit
					{
						req->dispatch_time = cobj->data_next +1; //optime should look at stage == 0 and return a lookup time for this cache
						req->stage=1;
						req->completion_time = req->dispatch_time + optime(cobj, req); //optime should look at stage and operation to decide
						cobj->data_next = req->completion_time;
					}

					else //Looking into forward queue now
					{
						lookupreq = queue_lookup(cobj, cobj->forward, req);
                                                if(lookupreq!=NULL)  //Forward queue look up was a hit
						{
							//printf("\n%llu: %s - FORWARD QUEUE HIT"); //TODO Verbose
							req->dispatch_time = cobj->data_next +1;
							req->stage=2; // Using to say that the lookup happened on the forward queue
							req->completion_time = req->dispatch_time + optime(cobj, req);
							cobj->data_next = req->completion_time; 
						}

						else //Miss
						{
							if(req->mode==0) 
							{
								req->count++;
								req->cache_queue[req->count]=cobj;
								schedule_queue(cobj, req, 0); //Send to forward queue if ld/st lookup failed
							}

							else //if its a writeback request
							{
								req->dispatch_time = (cobj->data_next, cobj->tag_next)+1;
								req->stage=3;
								req->completion_time = req->dispatch_time + optime(cobj, req); //looks at stage to decide 
								cobj->data_next = cobj->tag_next = req->completion_time; 
							}
						}

					}

				}
		
				else if(req->stage==1) //Bank hit - Data handling 
				{	//Normal LD/ST
					lookup_way = lookup(cobj, req);
					update(cobj, req, lookup_way); //Update sees ldst, mode and stage to update validity, dirty, counters for LRU
					if(req->ldst==0) //If its a load, youneed to return it back
					{
						//update properties of req
						schedule_queue(cobj, req, 3); //Pushing to return queue
					}

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
                                                update(cobj, req, victim);
						schedule_queue(cobj, req, 0); //To send the original request forward as well as evicted one					
                                                schedule_queue(cobj, insr, 0); //Schedule to forward queue
				}
				

			}
	
		break;

		case 2: //Response queue

				if(cobj->pending_response<=0) break;
				while(check_dispatch_ready(cobj->response, timestep)!=0)
				{

				lookup_way = -1;
                                req = queue_remove_timestep(cobj->input, timestep, 1); //Retain and return from input queue

                                if(req->stage==0) //This means its a tag lookup
                                {
                                        lookup_way = lookup(cobj,req);
                                        if(lookup_way == -2) { printf("\n%llu: %s - Error in value passed as address.... Exiting", timestep, cobj->name); exit(0);}

                                        else if(lookup_way>=0) //Cache hit
                                        {
                                                req->dispatch_time = cobj->data_next +1; //optime should look at stage == 0 and return a lookup time for this cache
                                                req->stage=4;
                                                req->completion_time = req->dispatch_time + optime(cobj, req); //optime should look at stage and operation to decide
                                                cobj->data_next = req->completion_time;
                                        }

                                        else //Looking into forward queue now
                                        {
                                                lookupreq = queue_lookup(cobj, cobj->forward, req);
                                                if(lookupreq!=NULL)  //Forward queue look up was a hit
                                                {
                                                        //printf("\n%llu: %s - FORWARD QUEUE HIT"); //TODO Verbose
                                                        req->dispatch_time = cobj->data_next +1;
                                                        req->stage=5; // Using to say that the lookup happened on the forward queue
                                                        req->completion_time = req->dispatch_time + optime(cobj, req);
                                                        cobj->data_next = req->completion_time;
                                                }

                                                else //Miss
                                                {
                                                                req->dispatch_time = (cobj->data_next, cobj->tag_next)+1;
                                                                req->stage=6; //Just to keep things unique
                                                                req->completion_time = req->dispatch_time + optime(cobj, req); //looks at stage and direction to decide
                                                                cobj->data_next = cobj->tag_next = req->completion_time;
                                                }
                                        }
					break;
				}

			
				if(req->stage==4) //Bank hit - Data handling 
                                {       //Normal LD/ST
                                        lookup_way = lookup(cobj, req);
                                        update(cobj, req, lookup_way); //Update sees ldst, mode and stage to update validity, dirty, counters for LRU
					queue_mremove(cobj, cobj->response, req);
                                }

                                else if(req->stage==5) //Forward queue hit
                                {       //Lookup was hit in the forward queue
                                        lookupreq = queue_lookup(cobj, cobj->forward, req);
                                        if(lookupreq==NULL) //Packet left before reading it. So it is queued back
                                        {
                                                printf("\n%llu: %s - Packet left from forward queue before you could read it...Oops!", timestep, cobj->name);
                                                req->stage=0;
                                                req->dispatch_time = cobj->tag_next + 1; // (+wire_latency?) 
                                                req->completion_time = insr->dispatch_time + optime(cobj, insr);
                                                cobj->tag_next = insr->completion_time;
						break;
                                        }
                                        else
                                        {
                                                //Increment hit counter here

                                                insr = make_packet_update(cobj, lookupreq, type); 
                                                queue_mremove(cobj, cobj->forward, lookupreq);

                                                //change property and mode of insr
                                                schedule_queue(cobj, insr, 0); //Schedule victim to forward queue

                                                //update properties of req
                                             
                                         }

                                }


				else if(req->stage==6) //Miss while returning
                                {
                                               
                                                insr = make_packet_update(cobj, req, type); 
                                             
						//update properties of req
                                                schedule_queue(cobj, req, 3); //To send the original request to return to next level                                     
                                                schedule_queue(cobj, insr, 0); //Schedule victim to forward queue
                                }
				
				cobj->pending_ret-=queue_mremove(cobj, cobj->response, req);
				schedule_queue(cobj, req, 3);
				}	


		break;

		case 3: //Return queue
			if(cobj->pending_ret<=0) 
			{
				output->valid = 0;
				break; //Nothing to return 
			}

			while(check_dispatch_ready(cobj->ret, timestep)!=0)
			{
				
				cobj->pending_input-=queue_mremove(cobj, cobj->input, req);
				req->count--;
				if(req->cache_queue[req->count]!=NULL)
					schedule_queue(req->cache_queue[req->count], req, 2);		
				output->req = req;
				output->valid = 1; 
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

**************************************************************************************************/

query * cpuio(unsigned long long int timestep)
{
	int i, flag=0;

	if(qcount>=qmax) 
	{
		printf("\nQueries ended.. Exiting");
		exit(0);
	}

	if(output->ack==0) { printf("\nStall"); }	

	if(qlist[qcount]->iptime==timestep) flag=1;


	if(output->valid==1) 
	{
		printf("\nGot something back: ");
		dump_request_data(output->req);	
	}

	if(flag==0) 
	{
		return NULL;
	}
	else
	{
		qcount++;
		return qlist[qcount-1];
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
	qcount =0;

	int temp;
	cache * prop = malloc(sizeof(cache));
	prop = cobj;

	while(1)
	{
		printf("\nTimestep: %llu", global_time);

		output->ack = 1;
		input = cpuio(global_time);
		while(prop!=NULL) 
		{
			if((input!=NULL) && (prop==cobj) && (input->valid==1) && (input->iptime>=global_time)) //CHECK equality in the last condition. Should it be ==?
			{
				if(access_perm(cobj)!=0) 
					{
						temp=0;					
					}
				else
				{
					temp=1;
					schedule_queue(cobj, input->req, 1); //Inserting to input queue of cache if request was valid at its dispatch time
					input->valid=0;
				}
			}
			dispatch_queues(prop, global_time, 0); //Forward
			dispatch_queues(prop, global_time, 1); //Input
			dispatch_queues(prop, global_time, 2); //Response
			if(prop==cobj) 
			{
				output = dispatch_queues(prop, global_time, 3); //Return //TODO Need to return here
				output->ack = temp;
			}
			else 
			{	
				dispatch_queues(prop, global_time, 3);
			}
			if(debug>=1) snapshot(prop);
			if(breakpoint>=1) getchar();
			prop = prop->next;
		}	
		cpuio(global_time);
		prop = cobj;
		global_time++;

		if(global_time>=end_time) return 0;
	}
}

/**************************************************************************************************
dram_access(...)

TODO

**************************************************************************************************/

int dram_access()
{
		printf("\nRequest to DRAM\n");

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

assert(num_queue_elements(cobj->input)==cobj->pending_input);
assert(num_queue_elements(cobj->forward)==cobj->pending_forward);
assert(num_queue_elements(cobj->response)==cobj->pending_response);
assert(num_queue_elements(cobj->ret)==cobj->pending_ret);

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


int print_bits(unsigned long long int add)
{
	//Warning. This prints bits int he reverse order and truncates the "MSB".. Need to fix later

	unsigned long long int addr = add;
	if(addr==0) { printf("0"); return 0;}
	while(addr)
	{
		if(addr & 0x01) printf("1");
		else printf("0");
		addr>>=1;
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
	if(debug>=1) print_bits(gen_addr(cobj, 0, set_num, tag_num));
return 0;
}

