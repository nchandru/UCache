/**************************************************************************************************

Functionalities to remember:

TODO
1) While asking for permission to send to next cache, both input AND forward queues of the next 
cache is to be checked for a free spot. When I think of it now, I feel we shoudl check for all
queues. 

2) Generic queue removal and counter update etc is taken care by dispatcher and NOT by scheduler. 
The scheduler only takes care of adding and its counters

DONE
1) When pushed to input queue. The queue looks at policy and automatically forwards to forward
queue if WB packet is not inclusive. The higher levels dont maintain the policies. Only the 
lower levels do. 

2) The update function takes care of eviction packet. 

**************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define pa_size 64
#define MAX_LEVELS 3

//Check if a strcuture type of CPU can be added during cache init? Or somewhere
//Need to update input counter only if the request was for a different cache block
//Verbose modes


int print_bits(unsigned long long int);

typedef struct request request;
typedef struct cache cache;

request * req_init(int, unsigned long long int, int, int);

typedef struct
{
	unsigned long addr;
	int valid;
	int dirty;
	unsigned long counter;
}blk;

typedef struct
{
	unsigned long long int  set;
	unsigned long long int tag; 

}naddress;

naddress * nexta;

struct cache
{
	char * name;        
	long block_size;
        long ways;
        long sets;
	long miss;
	long hit;
	blk **tag_block;
	struct cache *next;

	request * response; //To buffer the response from lower levels
	request * input; //To buffer up the inputs for lookup from higher levels. Inflight is also kep there, but pending would not be updated on duplicate
	request * forward; //To forward/evict/writeback to lower levels
	request * ret; //To respond to higher level cache with result of lookup

	int pending_forward;
	int pending_input;
	int pending_response;
	int pending_ret;

	int max_forward;
	int max_input;
	int max_response;
	int max_ret;

	unsigned long long int next_available;
	
	int find_victim_latency; 
	int lookup_latency;
	int read_latency;
	int write_latency;
};

struct request
{
	//Also change dulicate_request if adding/removing variables from this structure
	unsigned long long int addr;
	cache** cache_queue;	
	int count;
	int ldst;
	int mode; 
	int forwarded;
	request * next;
	long long int dispatch_time;
	long long int completion_time;

};


int check_policy(cache * cobj, request * req)
{
//returns policy decision. 

}


//TODO: Can implement tail based insertion. Maybe for a later commit

/**************************************************************************************************

duplicate_request(...)

Used to duplicate a request object. 

IMP: Need to update this for all the data variables in the request packet. 
TODO Need to see id this can be done using memory Syscalls. Memset like?

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
        unsigned long long int ones = 0xFFFFFFFFFFFFFFFF;

        

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
                                count++;
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
remove ALL requests which are for the same cache block. There is an added functionality of lookup 
in this queue. If the option is 2, the function returns the pointer to the request which looks up the address.

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

int queue_mremove(cache * cobj, request * ptr, request * req, int choice) 
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
				if(choice == 1) return &cur;
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

TODO Finalize the arguments 

Timing things as well.

Return 
Type - Cache * 
The Cache object pointer

**************************************************************************************************/

cache * cache_init(char * name, long block_size, long ways, long sets, int max_input, int max_forward,long lookup_latency, long read_latency, long write_latency)
{
	long i;
	cache * cobj = malloc(sizeof(cache));
        cobj->block_size=block_size;
        cobj->ways = ways;
        cobj->sets = sets;
	cobj->max_input = max_input;
	cobj->max_forward = max_forward;
	cobj->pending_input =0;
	cobj->pending_forward =0;
	cobj->name = name;
	cobj->tag_block = malloc(sets * sizeof(blk*));
	cobj->read_latency = read_latency;
	cobj->write_latency = write_latency;
	cobj->lookup_latency = lookup_latency;

        for(i=0; i<sets; i++)
        {
		cobj->tag_block[i] = malloc(ways * sizeof(blk));
        }

	cobj->input = req_init(0, 0, 0, 0);	
	cobj->forward = req_init(0, 0, 0, 0);
	cobj->ret = req_init(0, 0, 0, 0);
	cobj->response = req_init(0, 0, 0, 0);

	cobj->next = NULL;

	nexta = malloc(sizeof(naddress));

return cobj;
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
		
		/*
		//Timing parameters
		temp->access_time = 0;
		temp->return_time = 0;
		*/
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
	printf("\nCache snapshot of %s \n------------------------------\n", cobj->name);

	for(i=0;i<cobj->sets;i++)
	{
		printf("\nSET %d ",i);
		for(j=0;j<cobj->ways;j++)
			printf("|v=%d, d=%d| ", cobj->tag_block[i][j].valid, cobj->tag_block[i][j].dirty);
	}

	printf("\n-----------------------------\n\n");
return 0;
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
}

/**************************************************************************************************

schedule_queue(...)

Schedule queue is an important function which uses the arguments to decide where and 
when the request passed is to be scheduled. The queue_select argument hints on which output queue
needs to be considered while deciding scheduling.  

Arguments:

1) cobj - Cache structure object pointer
Type - Cache *

2) ptr - The starting pointer of the queue 
Type - Request *

3) req - The request under consideration
Type - Request *

4) queue_select - just an identifier to show what queue is being worked on
Type - Integer

0 -> Forward queue
1 -> Input queue
2 -> Response queue
3 -> Return queue

Return:

TODO
1) queue_select can be ENUM

**************************************************************************************************/

schedule_queue(cache * cobj, request * req, int queue_select)
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

			//If request is writeback, check policy==exclusive and add to forward queue directly. Otherwise, schedule for writebactime(cobj)

			if(check_policy(cobj,req)==0) //0 means not in policy 
			{	
				req->completion_time = 0;
				schedule_queue(cobj, req, 0); //Pushing to output queue if not in policy
				break;
			}
			
			insr->dispatch_time = 0; //making this default zero to not allow duplicates

			insr = duplicate_request(insr, req);

			if(queue_lookup(cobj, cobj->input, req)!=NULL) //Do all this only if the request is original 
			{
			insr->dispatch_time = cobj->next_available + 1; // (+wire_latency?) 
			insr->completion_time = insr->dispatch_time + optime(cobj, req);
			cobj->next_available = insr->completion_time;
			cobj->pending_input++;
			}
			queue_add(cobj, cobj->input, insr);
			
			//allot dispatch as next_Avail+1 (+latency?)
			//allot completion_time as dispatch+optime
			//allot next_avail as completion_time

		break;

		case 2: //Response queue

			if(check_policy(cobj, req)==0)
			{
				schedule_queue(cobj, req, 3); //Pushing to return if not in policy 
				break;
			}

			insr = duplicate_request(insr, req);
			insr->dispatch_time = cobj->next_available+1;
			insr->completion_time = insr->dispatch_time + optime(cobj,req);
			cobj->next_available = insr->completion_time;
			queue_add(cobj, cobj->response, insr);
			cobj->pending_response++;

			// check if its dependent (OR! Check the input queue for matching requests). If not, skip to GO:
			// If dependent, add to queue with
			// dispatch = next_avail+1
			// completion_time = dispatch + optime
	
			//GO: push to return queue, after checking status. if unavailable, update counter

		break;

		case 3: //Return queue

			// dispatch = max(completion_time+1,max(return_queue)+1)
			insr = duplicate_request(insr,req);
			insr->dispatch_time = max_cmp(req->completion_time, find_max_dispatch(cobj->ret))+1;
                        queue_add(cobj, cobj->ret, insr);
                        cobj->pending_ret++;


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
	if(cur==NULL) { printf("\nNothing to lookup"); return temp; } //Nothing to remove. Mostly this would be called out of indexing error

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


int dispatch_queues(cache * cobj, unsigned long long int timestep, int queue_select)
{
	request * req;
	request * lookupreq;
	int lookup_way;
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
				}
				
				else
				{
					if(access_perm(cobj->next)!=0)  //If the next level is busy and the permission is denied, Update the counters and exit
					{
						//update_queue_counters(cobj->forward)	//TODO make this function
						assert(cobj->pending_forward == num_queue_elements(cobj->forward));
						break;
					}

					//schedule_queue(cobj->next, cobj->next->input, queue_remove_timestep(cobj->forward, timestep, dest_queue_select); 
					//Adding the request to the input of the next cache. The int_ is for the next queue to decide when to schedule. 
					//It will choose input queue or forward (not in policy case - but isnt this of concern only during response?) 
					//of next and call queue_add in turn //TODO make this function
				}
				cobj->pending_forward--;
			}
	
			assert(cobj->pending_forward == num_queue_elements(cobj->forward));
		

		break;

		case 1: // Input queue
				
			//TODO need to do that ting where a hit/miss needs to adjust the queue dispatch times			
			//Not asserting the queue lengths because the duplicate elements are not considered
	
			if(cobj->pending_input<=0) break; //Nothing in the input queue
	
			while(check_dispatch_ready(cobj->input, timestep)!=0)
			{
				if(cobj->pending_input<=0) break; //Again, redundant
				
					
				lookup_way = -1;
				req = queue_remove_timestep(cobj->input, timestep, 1); //Retain and return from input queue
				
				switch(req->mode)
				{
					case 0: //Normal LD/ST
						lookup_way = lookup(cobj, req);
						
						if(lookup_way == -2) { printf("\nT: %llu - %s :Error in value passed as address.... Exiting", timestep, cobj->name); exit(0); }
						
						else if(lookup_way>=0) //Cache hit
						{
							//printf("\n%llu %s - CACHE HIT", cobj->name); //TODO verbose
							update(cobj, req, lookup_way);
							//schedule_queue(cobj, cobj->ret, req, dest_queue_select) 
	
							//If this operation was a store, schedule a writeback and send to forward queue
	
							//dest-queue -> return q of the same cache. Make sure to add all the requests for the current address under the timestamp 
							//to the return queue. Duplicate from within this and sent to queue_add. And remove the queues from input as 
							//I mentioned beofre from here itself. 
							//The request is looked up and on a hit, the timing is updated in the int_ function and inserted into its own return queue
							//TODO make this fucntion
							break;
						}
						
						else //Lookup the output queue
						{
							lookupreq = queue_lookup(cobj, cobj->forward, req);
							if(lookupreq!=NULL)	//output queue look up was a hit
							{
								dipatch_update(cobj->forward, req->dispatch_time, )									

	
							}
						}
/*
						else if(lookup_way==-1) //Cache miss
						{

							//look in the output queue and if a hit,  
							//update the queue dispatch and completion and next_avail time. Incremental/Decremental
							//printf("\n%llu: %s - CACHE HIT", cobj->name); //TODO verbose
							//Find replacement and swap. ANd add to return queue

							//If output queue is also a miss
							//Change timing parameters of input queue
							//add self to queue cache array if in policy. Also, remove_multiple from input queue right away if not in policy
							//change "forward" of request
							//schedule_queue(cobj, cobj->output, req, dest_queue_select_self_outputq)
							
						}
*/
					break;

					case 1: //Writeback
		
							//lookup, find victim and evict if found
							//mark corresponding block as dirty. 
							//push same request to forward. 						
							 
					break; 

					case 2: //Eviction
							//If inclusive, lookup and dirtyfy
							//If exclusive, find_victim, replace and dirtyfy
							//Write the evicted block with "evicted" tag into forward queue. 
							
							//If inclusive and block is clean, the packet under consideration is dropped							
					break;

					default:
						//TODO make errom enums
					break;


				}

			}
			//TODO Cases of write back and eviction
			//Also completely remove from input queue if not in policy 

		break;

		case 2: //Response queue
				if(cobj->pending_response<=0) break;
				while(check_dispatch_ready(cobj->response, timestep)!=0)
				{
					//check if a free way is found. If found. Update it with that request
					//If not, find a victim, make a request packet and add it to "evict" or "writeback" queue. Check queue first too. As an assert
					//update the cache block of the victim found
					//remove all requests in the input queue of the same type (LD/ST) for the current 				
					//->If the input queue has load first and then store, both of them need to get separate return queue allocations
	
				}	


		break;

		case 3: //Return queue

			if(cobj->pending_ret<=0) break; //Nothing to return 
			while(check_dispatch_ready(cobj->ret, timestep)!=0)
			{
				//Remove all requests fo this address from self's input queue
				//Check if the return is a CPU or not first
				//Check what the policy of the north cache is. 
				//Add to the response queue for performing an update. Add only if response queue is free. Or stall
				
			}

		break;

		default:
			//TODO Define global error ENUMS
		break; 

	}


}

int dram_access()
{
		printf("\nRequest to DRAM\n");

return 0;
}


int access_perm(cache * cobj)
{
//TODO check the corresponding queues and return permission. This is irrespective of request type 

/*
	if(req->mode == x) // South Flow (Response request)
	{
		if(check_queue_len(cobj->response)<=max_response) return 1;
		else return -1;
	}

	else //North flow (CPU to DRAM)
	{
		if(check_queue_len(cobj->input)<=max_input) return 1;
		else return -1;
	}	
*/
}


unsigned long long int gen_addr(cache * cobj, unsigned long long int set, unsigned long long int tag)
{
	if(set>cobj->sets-1) { printf("\n%s: Addressing error. SET out of bounds.\n", cobj->name); exit(0);}

	int tag_bits = pa_size -((log(cobj->sets)/log(2))+(log(cobj->block_size)/log(2)));
	int set_bits = log(cobj->block_size)/log(2);
	
	return ((tag<<(pa_size-tag_bits))|(set<<set_bits));
}

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

int manual_set(cache * cobj, long set_num, long way_num, long tag_num, int valid, int dirty)
{
	cobj->tag_block[set_num][way_num].valid = valid;
	cobj->tag_block[set_num][way_num].dirty = dirty;
        cobj->tag_block[set_num][way_num].addr = gen_addr(cobj, set_num, tag_num);
	print_bits(gen_addr(cobj, set_num, tag_num));
}


int queue_len(cache * cobj)
{
        //TODO Add all queue to this. And maybe add this to snapshot too
        printf("\n%s: Queue_len checker called", cobj->name);
        int j=0, k=0;
        j = num_queue_elements(cobj->input);
        k = num_queue_elements(cobj->forward);
        printf("\n%s: Request queue -> %d, Evict queue -> %d", cobj->name, j, k);
}


int main()
{
	unsigned long long int addr;
	int ldst, mode,	req_q, evict_q, valid, dirty;
	long blk_size, way, set;
	char * name;

	int set_num, tag_num, way_num;

	cache *l1; 
	cache *l2;

	//Cache declaration parameters
	//l1 = init(name,blk_size,way,set,req_q,evict_q); //name, block size, way, set, request queue, evict queue

	l1 = cache_init("L1", 64, 2, 4, 32, 32, 2, 2, 2);
	l2 = cache_init("L2", 64, 4, 16, 32, 32, 2 , 2, 2); //name, block size, way, set, request queue, evict queue
	
	l1->next = l2;
	
	//printf("\nResting state");

	//snapshot(l1);	
	//snapshot(l2);
	
	//temporary test factors
	set_num = 9;
	tag_num = 15;
	way_num = 2;
	valid = 1;
	dirty = 0;

	manual_set(l2, set_num, way_num, tag_num, valid, dirty);
	manual_set(l2, set_num, way_num-1, tag_num+15, valid, dirty);

	printf("\nManual set for L2 performed");

	snapshot(l1);
	snapshot(l2);
	
	//Request declaration	
	addr = gen_addr(l2, set_num, tag_num);
	ldst = 0;
	mode = 0;

	request * req;
		
	req = req_init(1, addr, ldst, mode);	

	access(l1, req);

	queue_len(l1); //See if anything in the queue of l1
	getchar();
		
	snapshot(l1);
	snapshot(l2);

	printf("\nFirst access complete");
	
	addr = gen_addr(l2, set_num, tag_num+15);
	req=req_init(1, addr, ldst, mode);
	
	printf("\nSecond access with address: ");
	
	print_bits(addr);
	
	//access(l1, req);
	
	printf("\nSecond access complete");
	
	//snapshot(l1);
	//snapshot(l2);


	printf("\n\n\n");

return 0;
}


/*
long lookup(cache * cobj, request * req)
{
	printf("\n%s: Inside lookup", cobj->name);
	//print_bits(req->addr);
	unsigned long long int ones = 0xFFFFFFFFFFFFFFFF;
	long i;
	parse(cobj, req);
	if(nexta->set>=cobj->sets) { printf("\n%s: lookup() Out of bounds SET accessed... Exiting", cobj->name); return -2; } //exit on incorrect argument passed
	int block_bits = log(cobj->block_size)/log(2);

	printf("\n%s: SET lookup is - %llu", cobj->name, nexta->set );	
	for(i=0; i<cobj->ways; i++)
		if(( (cobj->tag_block[nexta->set][i].addr) & (ones<<block_bits)) == ((req->addr) & (ones<<block_bits)) && (cobj->tag_block[nexta->set][i].valid==1)) return i; //return the WAY where it was found

return -1; //None found. Return negative one
}

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


int print_count_values(cache* cobj, long set) //For dumping the counters per set of a cache
{
	if(set>=cobj->sets) { printf("\n%s: Out of bounds SET accessed... Exiting", cobj->name); return -2; } //exit on incorrect argument passed
	
	long i;
	for(i=0; i<cobj->ways; i++)
		printf("\nWay %ld Count: %lu", i, cobj->tag_block[set][i].counter);
}

int eq_add(cache * cobj, request * req)
{
	
	if(cobj->pending_forward>=cobj->max_forward-1) {return -1;}
	queue_add(cobj, cobj->forward, req);
	cobj->pending_forward++;
}

int eq_remove(cache * cobj, request * req)
{
	queue_remove(cobj,cobj->forward, req, 0);
	cobj->pending_forward--;
}

int rq_add(cache * cobj, request * req) //request queue add
{
	int req_present=0;
	if(cobj->pending_input>=cobj->max_input-1) {return -1;} //No queue space to insert request. Should send failure to add to queue 

	//add cobj to request cache array.....increment counter then add self
	
	req->count++;
	req->cache_queue[req->count] = cobj; //adding the cache object to the request array

	//add req pointer to cobj req array...check for previous entries

	req_present = queue_add(cobj, cobj->input, req);
	cobj->pending_input++;


//printf("\nreq_present being returned from rq_add = %d", req_present);
return req_present;
}

int rq_remove(cache * cobj, request * req, int choice) 
{
	int t;
	req->count--;
	t = queue_remove(cobj,cobj->input, req, choice); 	
	cobj->pending_input-=t;
	
}


int queue_len(cache * cobj)
{
	//TODO Add all queue to this. And maybe add this to snapshot too
	printf("\n%s: Queue_len checker called", cobj->name);
	int j=0, k=0;
	j = num_queue_elements(cobj->input);
	k = num_queue_elements(cobj->forward);
	printf("\n%s: Request queue -> %d, Evict queue -> %d", cobj->name, j, k);
}

int insert(cache * cobj, request * req)
{
	//divide this into if its a cache or CPU 

	int victim;
	//update the tag arrays
	
	rq_remove(cobj, req, 1);
	victim = find_victim(cobj, req);
	update(cobj, req, victim); // ppp this should also take care of policy.... Also val/dirty stuff
}


int access_return(request * req)
{
	printf("\nInside access return");
	int i;
	for(i=req->count; i>=0; i--)
	{
		insert(req->cache_queue[i], req);
	}
}

int check_policy(cache * cobj, request * req)
{
	//To check if the current cache is inclusive of the cahce pointers in the request cache_queue

}

int access(cache * cobj, request * req)
{
	
	long found_way, victim;
	int temp, flag=0;
        request * rq;	

	//check if this cache requires the headache of the request (from policy)


	switch(req->mode){

	case 0: //Normal load/store
	printf("\n%s: LD/ST request access at cache with request address: ", cobj->name);
	//print_bits(req->addr);
	
	//if(check_policy(cobj, req)==0) { return(access(cobj->next, req)); } //need to revamp error flags 
	//else
	if(cobj->pending_input>=cobj->max_input-1) return -1;   //checking if request queue can accomodate
	if(cobj->pending_forward>=cobj->max_forward-1) return -1; //revamp error flags

	found_way = lookup(cobj, req);
	
	if(found_way == -2) { printf("Error in value passed... Exiting"); exit(0);}
	
	else if(found_way>=0) //Cache hit
	{
		printf("\n%s: Cache HIT", cobj->name);
		update(cobj, req, found_way);
		access_return(req);
		return 1;
	}

	else //Cache miss
	{
		printf("\n%s: Cache MISS", cobj->name);
		flag = 0;
		temp = rq_add(cobj,req);

		queue_len(cobj);	//Checking if the request was added to the queue
		getchar();

		if(temp==0) //No problem in adding to the queue		
			{
				// Find a victim to replace the current request 
				victim = find_victim(cobj, req);
				printf("\nREQ added to the request queue. Victim found way: %ld", victim);

				parse(cobj, req);
				
				if(cobj->tag_block[nexta->set][victim].valid!=0) //Add to queue only if the victim was valid
				{	
					flag = 1;
					cobj->tag_block[nexta->set][victim].valid=0;
					rq = req_init(1, cobj->tag_block[nexta->set][victim].addr, 1, 1); //third argument indicates eviction 
					eq_add(cobj, rq);
					printf("\n%s: SET %llu WAY %ld added to eviction queue", cobj->name, nexta->set, victim);
				}
				
				if(cobj->next==NULL)
				{
					dram_access();
					return 0; //Need to integrate with USIMM later
				}



				temp = access(cobj->next, req); 
				if(temp == -1) 
				{ 
					rq_remove(cobj, req, 0); //Only last added queue for an address is removed. Under the assumption that overflow is taken care of by the CPU and not the intermediate caches
					//If the next level can't access. Repopulate the invlaidated address
					
					if(flag==1) //Do the removal only if it was added in the first place
					{
					eq_remove(cobj, rq);
					cobj->tag_block[nexta->set][victim].valid = 1;
					cobj->tag_block[nexta->set][victim].addr=rq->addr;
					free(rq); 
					printf("\n%s: SET %llu WAY %ld block readded from eviction queue due to lower level cache unavailability", cobj->name, nexta->set, victim);
					}

					return -1; 
				}//see if this return can be reiisued later from higher level cache

				else return 1;
			}
		else if(temp==-1)
		{
			printf("\n%s: Request queue FULL. Cannot accept more\n",cobj->name);
			return -1; //redundant checking I believe
		}
		else 
		{
		return -2; //request was added to the queue, but the address of the request already existed so new request was not propagated
		}
	}

	break;

	case 1: //Eviction

	//if(check_policy(cobj, req)==0) { return(access(cobj->next, req)); } //need to revamp error flags 
        //else
        if(cobj->pending_forward>=cobj->max_forward-1) return -1; //Eviction operation checks if the current evict queue is full or not
	
	
	break;

	case 2: //Write back

	break;

	default:
	printf("\n Wrong mode set for request....");
	break;

}//end switch case

}//end access

*/
