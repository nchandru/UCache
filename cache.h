#ifndef CACHE_H_
#define CACHE_H_

#include <stdio.h>

int breakpoint;
int debug;

#define pa_size 64
#define MAX_LEVELS 3
#define DRAM_LATENCY 150

FILE * RES_FP; //Results File desciptor
char * res_file_name; //Results file name

unsigned long long int num_req; //To hold number of requests received 
unsigned long long int global_time;
unsigned long long int end_time;


typedef struct request request;
typedef struct cache cache;
typedef struct naddress naddress;
typedef struct blk blk;
typedef struct query query;
naddress * nexta;

unsigned long long int qcount;
unsigned long long int qmax;
query ** qlist;
query * input;
query * output;



struct query
{
	request * req; //the request object this query points to 
	int valid; //To show that something on the line is valid
	int ack; //To show the ACK of the previous instruction that was passed 
	int highest; //To show if this is at the highest or lowest level of the cache
	unsigned long long int iptime; //To designate the time of input 
};

struct blk
{
        unsigned long addr;
        int valid;
        int dirty;
        unsigned long counter;
};

struct naddress
{
        unsigned long long int  set;
        unsigned long long int tag;

};

struct cache
{
        char * name;
        long block_size;
        long ways;
        long sets;
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

        unsigned long long int tag_next;
        unsigned long long int data_next;

        int find_victim_latency;
        int lookup_latency;
        int read_latency;
        int write_latency;

	int miss; //For MPKI

	int loadtime;
	int storetime;
	int victimtime;
	int lookuptime;

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
        int stage;
        request * next;
        long long int dispatch_time;
        long long int completion_time;

};

long find_victim(cache * , request *);
request * req_init(int, unsigned long long int, int, int);


int check_policy(cache *, request *);
request * duplicate_request(request *, request * );
int queue_add(cache * , request* , request * );
int queue_stats(cache * );
request * queue_remove_timestep(request *, unsigned long long int , int );
request * queue_lookup(cache * , request * , request * );
int queue_mremove(cache * , request * , request * );
int check_dispatch_ready(request * , unsigned long long int );
int num_queue_elements(request * );
int parse(cache * , request * );
cache * cache_init(char * , long , long , long );
int snapshot(cache * );
long find_victim(cache * , request * );
long lookup(cache *, request *);
int update(cache * , request * , long );
int find_max_dispatch(request * );
unsigned long long int max_cmp(unsigned long long int , unsigned long long int );
long optime(cache * , request * );
int schedule_queue(cache * , request * , int );
int dispatch_update(request * , unsigned long long int , long );
request * make_packet_update(cache * , request * , int );
query * dispatch_queues(cache * , unsigned long long int , int );
int dram_access(request *);
int access_perm(cache * );
unsigned long long int gen_addr(cache * , unsigned long long int, unsigned long long int, unsigned long long int);
int print_bits(cache *, unsigned long long int );
int manual_set(cache * , long , long , long , int , int );
int queue_len(cache *);
query * query_init(request *, int, int, unsigned long long int);
int dump_request_data(request * );
int simulate(cache *);
query * cpuio(unsigned long long int, int);

#endif
