#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define pa_size 64
#define MAX_LEVELS 3

//making request duplicates for queues 
//Check if a strcuture type of CPU can be added during cache init? Or somewhere
//Need to add access_permission before sending request

int print_bits(unsigned long long int);
//request * req_init(unsigned long long int, int, int);

typedef struct request request;
typedef struct cache cache;


request * req_init(unsigned long long int, int, int);

typedef struct
{
	unsigned long addr;
	int valid;
	int dirty;
	unsigned long counter;
}blk;

struct cache
{
        long block_size;
        long ways;
        long sets;
	long miss;
	long hit;
	blk **tag_block;
	struct cache *next;
	request * response_start;
	request * pending_start;
	request * evict_start;
	int pending_evict;
	int evict_max;
	int pending_max;

	int pending_req;
	char * name;

	unsigned long long int next_available;
	unsigned long long int lookup_latency;
	unsigned long long int  read_latency;
	unsigned long long int  write_latency;
};

struct request
{
	//Also change dulicate_request if adding/removing variables from this structure
	unsigned long long int addr;
	cache** cache_queue;	
	int count;
	int ldst;
	int mode; //0 for normal ld/st....1 for eviction....2 for writeback  
	request * next;
	unsigned long long int access_time;
	unsigned long long int return_time;

};

//TODO: Can implement tail based insertion. Maybe for a later commit

request * duplicate_request(request * to, request * from)
{
	int i;
	//request * req_init(unsigned long long int addr, int ldst, int mode)
	to = req_init(from->addr, from->ldst, from->mode);
	
	for(i=0; i<=from->count; i++)
		to->cache_queue[i]=from->cache_queue[i];
	
	to->mode = from->mode;
	to->next = NULL;
	to->access_time=from->access_time;
	to->return_time=from->return_time;

return to;
}

int queue_add(cache * cobj, request* ptr, request * reqin) //TODO: Error checking in case there is a request "object" already present in the queue
{
	request * temp;
	temp = ptr->next;
	
	request * req;
	req = duplicate_request(req, reqin);

	int req_present=0; 
	int set_bits = log(cobj->block_size)/log(2);
	unsigned long long int ones = 0xFFFFFFFFFFFFFFFF;
	
	if(temp == NULL) //First element is being added. i.e No element in the queue
	{
		printf("\nFirst element was added");
		req->next = NULL;
		ptr->next = req;
		//printf("\n2. Address: %lu", (unsigned long)temp->next);
	
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

int queue_remove(cache * cobj, request * ptr, request * req, int choice) //choice = 1 means remove all requests which are for a particualr address. 0 means remove only that queue object which was added last
{
	//Freeing memory should be done
	int count=0;
	request * cur; // = malloc(sizeof(request));
	cur = ptr->next;
	if(cur==NULL) { printf("\nNothing to remove"); return 1; } //Nothing to remove. Mostly this would be called out of indexing error

	request * prev; // = malloc(sizeof(request));
	prev = ptr; 
	unsigned long long int ones = 0xFFFFFFFFFFFFFFFF;
	int set_bits = log(cobj->block_size)/log(2); //cobj was passed just for this 

	while(cur!=NULL)
	{
		if((choice && ((((cur->addr)&(ones<<set_bits))==((req->addr)&(ones<<set_bits)) || (cur==req)))) || ((!choice) && (cur==req)) ) //If address is the same or the request itself is the same, we remove that request from the queue		
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

typedef struct
{
	unsigned long long int  set;
	unsigned long long int tag; 

}naddress;

naddress * nexta;

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


request * req_init(unsigned long long int addr, int ldst, int mode)
{
	int i;
	request * temp = malloc(sizeof(request));
	temp->addr= addr;
	temp->count = -1;
	temp->ldst = ldst;
	temp->mode = mode;

	temp->cache_queue  = malloc(MAX_LEVELS * sizeof(cache*)); //TODO: plus one becasue planning to add CPU object as well
	for(i=0; i<MAX_LEVELS; i++)
		temp->cache_queue[i] = malloc(sizeof(cache));
	temp->next = NULL;

	temp->access_time = 0;
	temp->return_time = 0;

return temp;
}


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

cache * cache_init(char * name, long block_size, long ways, long sets, int pending_max, int evict_max,long lookup_latency, long read_latency, long write_latency)
{
	long i;
        cache * cobj = malloc(sizeof(cache));
        cobj->block_size=block_size;
        cobj->ways = ways;
        cobj->sets = sets;
	cobj->pending_max = pending_max;
	cobj->evict_max = evict_max;
	cobj->pending_req =0;
	cobj->pending_evict =0;
	cobj->name = name;
	cobj->tag_block = malloc(sets * sizeof(blk*));
	cobj->read_latency = read_latency;
	cobj->write_latency = write_latency;
	cobj->lookup_latency = lookup_latency;

        for(i=0; i<sets; i++)
        {
		cobj->tag_block[i] = malloc(ways * sizeof(blk));
        }

	cobj->pending_start = malloc(sizeof(request));	
	cobj->pending_start->next = NULL;

	cobj->evict_start = malloc(sizeof(request));
	cobj->evict_start->next = NULL;

	cobj->next = NULL;

	nexta = malloc(sizeof(naddress));

return cobj;
}

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

	//Recheck this too. Return with best bet
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
	
	if(cobj->pending_evict>=cobj->evict_max-1) { return -1; }
	queue_add(cobj, cobj->evict_start, req);
	cobj->pending_evict++;

}

int eq_remove(cache * cobj, request * req)
{
	queue_remove(cobj,cobj->evict_start, req, 0);
	cobj->pending_evict--;
}

int rq_add(cache * cobj, request * req) //request queue add
{
	int req_present=0;
	if(cobj->pending_req>=cobj->pending_max-1) {return -1;} //No queue space to insert request. Should send failure to add to queue 

	//add cobj to request cache array.....increment counter then add self
	
	req->count++;
	req->cache_queue[req->count] = cobj; //adding the cache object to the request array

	//add req pointer to cobj req array...check for previous entries

	req_present = queue_add(cobj, cobj->pending_start, req);
	cobj->pending_req++;


//printf("\nreq_present being returned from rq_add = %d", req_present);
return req_present;
}

int rq_remove(cache * cobj, request * req, int choice) 
{
	int t;
	req->count--;
	t = queue_remove(cobj,cobj->pending_start, req, choice); 	
	cobj->pending_req-=t;
	
}

int queue_len(cache * cobj)
{
	printf("\n%s: Queue_len checker called", cobj->name);
	int j=0, k=0;
	j = num_queue_elements(cobj->pending_start);
	k = num_queue_elements(cobj->evict_start);
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

int dram_access()
{

	printf("\nRequest to DRAM\n");

return 0;
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
	if(cobj->pending_req>=cobj->pending_max-1) return -1;   //checking if request queue can accomodate
	if(cobj->pending_evict>=cobj->evict_max-1) return -1; //revamp error flags

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
					rq = req_init(cobj->tag_block[nexta->set][victim].addr, 1, 1); //third argument indicates eviction 
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
        if(cobj->pending_evict>=cobj->evict_max-1) return -1; //Eviction operation checks if the current evict queue is full or not
	
	
	break;

	case 2: //Write back

	break;

	default:
	printf("\n Wrong mode set for request....");
	break;

}//end switch case

}//end access

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
		
	req = req_init(addr, ldst, mode);	

	access(l1, req);

	queue_len(l1); //See if anything in the queue of l1
	getchar();
		
	snapshot(l1);
	snapshot(l2);

	printf("\nFirst access complete");
	
	addr = gen_addr(l2, set_num, tag_num+15);
	req=req_init(addr, ldst, mode);
	
	printf("\nSecond access with address: ");
	
	print_bits(addr);
	
	//access(l1, req);
	
	printf("\nSecond access complete");
	
	//snapshot(l1);
	//snapshot(l2);


	printf("\n\n\n");

return 0;
}


