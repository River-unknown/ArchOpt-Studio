#include<stdio.h>
#include "power.h"
//#include "benchmark.h" for set operations
#include "benchmark_cmp.h"// for comparator
#define block_size 64
#define sets 256
#define blocks 16
#define memory_size 1073741824L//2^{30}
#define offset_bits 6
#define offset_mask 0x0000003F
#define set_bits 8
//#define set_mask 0x000000FF
#define valid_bit_mask 0x01
#define dirty_bit_mask 0x02
#define pg_rep_policy_mask 0x01
#define wr_policy_mask 0x02
#define memory_block_mask 0xFFFFFFC0
#define l1_dcache_assoc 4
#define l1_icache_assoc 4
#define l2_cache_assoc 16
#define memory_l2cache 0
#define l2cache_l1dcache 1
#define l1dcache_cpu 2
#define max_bus_width 32 // 256 bits
#define mem_l2cache_bus_width 8 // 64 bits
#define l2cache_l1dcache_bus_width 16 // 128 bits
#define l1dcache_cpu_bus_width 32 // 256 bits 
#define cycles_per_bus_transfer 2 // cycles required per bus transfer 
#define dram_size 512*1024*1024*8               // 512MB in bits
#define sram_size ((l1_icache_assoc+l1_dcache_assoc+l2_cache_assoc)*sets*block_size*8)            // in bits
#define signbit_mask   0x80000000// sign bit mask
#define magnitude_mask 0x07FFFFFF// magnitude mask
#define EXP_MASK 0x7F800000// exponent mask
#define MAN_MASK 0x007FFFFF// matissa mask
#define EXP_NAN_INF 0x7F800000 // exponent of NAN and INF
#define MAN_INF 0x00000000 // mantissa of INF
#define ZERO 0x00000000 // +-0.0
#define num_alu_components 4
#define comparator 0 
#define adder 1
#define multiplier 2
#define divider 3
unsigned char main_memory[memory_size];// DRAM physical memory
struct cache
 {
   unsigned char data[blocks][block_size];// data array
   unsigned int tag[blocks];// tag array
   unsigned char flags[blocks]; //bit 0  valid bit, bit 1 dirty bit
   unsigned char access[blocks]; // page replacement data structure
   unsigned char entry;// number of entries in page replacement data structure
 };
struct cache l1_dcache[sets];
struct cache l1_icache[sets];
struct cache l2_cache[sets];
struct abus
{
  unsigned int address_lines;
  unsigned long long int switching_activity;	
};
struct dbus
{
  unsigned char data_lines[max_bus_width];
  unsigned long long int switching_activity;	
};
struct ALU
{
  unsigned long long int switching_activity;
  double switching_energy;
};
unsigned long long int memory_content_switching_activity[3];// switching activity of memory contents 0 memory, 1 l2 cache, 2 l1 dcache
struct abus address_bus[3];
struct dbus data_bus[3];
struct ALU alu_components[num_alu_components];// component (0 -- comparator)
unsigned int bus_width[]={8,16,32};
double bus_capacitance[]={mem_l2_bus_cap,l2_dl1_bus_cap,dl1_cpu_bus_cap};
double alu_component_capacitance[]={comparator_cap,adder_cap,multiplier_cap,divider_cap};
unsigned int memory_access(unsigned int addr,unsigned char access,unsigned int data,char type);// cache search for hit miss
unsigned char FIFO_page_replacement(struct cache*c,unsigned char assoc,unsigned int mem_type);// FIFO, returns a page to be replaced from a set 
unsigned char LRU_page_replacement(struct cache*c,unsigned char assoc,unsigned int mem_type);// LRU, returns a page to be replaced from a set
void LRU_update(struct cache*c,unsigned char assoc,unsigned int pg,unsigned int mem_type);// update page access status
double transfer_block(unsigned char*src,unsigned char*tar,unsigned int start_address,unsigned int bus_type,unsigned int to_mem_type);// tranfers a block between cache and memory
unsigned int get_memory_start_address(unsigned int tag,unsigned int set);
unsigned int get_hamming_distance(unsigned int,unsigned int);
void put_integer(unsigned char*,unsigned int,unsigned int);
unsigned int get_integer(unsigned char*,unsigned int);
void termination();// writeback l2 cache memory contents before termination 
unsigned int get_hamming_distance(unsigned int,unsigned int);
void get_performance();
unsigned char l1_policy;// bit 0 LRU=0/FIFO=1,  bit 1 Write back,=0/Write trough=1 
unsigned char l2_policy;// bit 0 LRU=0/FIFO=1,  bit 1 Write back=0/Write trough=1
unsigned long long int l1_dcache_miss,l2_cache_miss,mem_access,num_bus_transfers[3],num_mem_updates[3];
unsigned long long int tot_dram_reads,tot_dram_writes,tot_sram_reads,tot_sram_writes,tot_clock_cycles;
unsigned char is_first_program;//first benchmark program
void compare(unsigned int,unsigned int);// compare unsigned integers
void compares(int,int);// compare signed integers
void comparef(float,float);// compare floating point numbers
unsigned int add(unsigned int,unsigned int,unsigned int);// 32-bit full adder
unsigned int ADD(unsigned int,unsigned int);// 32-bit add instruction 
unsigned int SUB(unsigned int,unsigned int);// 32-bit subtract instruction
unsigned int MUL(unsigned int,unsigned int);// 32-bit multiplication instruction
unsigned int flag_reg; //0x1 EQ, 0x2 GT, 0x4 LT ; 
unsigned int memory_access(unsigned int addr,unsigned char access,unsigned int data,char type)// cache search for hit miss
{
	unsigned int tag,set,l1_block,l2_block,memory_block,memory_start_address,offset,b,hit=0,byte;
	unsigned char prev,curr;
	mem_access++;
	offset=addr&offset_mask;
	memory_block=addr>>offset_bits;
	set=memory_block%sets;
	address_bus[l1dcache_cpu].switching_activity+=get_hamming_distance(address_bus[l1dcache_cpu].address_lines,addr);
	address_bus[l1dcache_cpu].address_lines=addr;
	tag=addr>>(set_bits+offset_bits);
	//printf("Memory access location: %u\n",addr);
	tot_clock_cycles++;// l1 dcache and l2 cache hit-miss delay
	for(b=0;b<l1_dcache_assoc;b++)
	 {
	 	if((l1_dcache[set].flags[b]&valid_bit_mask)&&(l1_dcache[set].tag[b]==tag))
	 	 {
	 	 //	printf("L1 DCACHE hit: tag=%x set=%x block=%x offset=%x\n",tag,set,b,offset);
	        hit=1;
	        l1_block=b;
	 	    break;// l1 dcache hit
	     }
	 }
	if(!hit)// l1 dcache miss
	  {
	  	//printf("L1 DCACHE miss: tag=%x set=%x offset=%x\n",tag,set,offset);
	  	l1_dcache_miss++;
	  		address_bus[l2cache_l1dcache].switching_activity+=get_hamming_distance(address_bus[l2cache_l1dcache].address_lines,set);
	        address_bus[l2cache_l1dcache].address_lines=set;
        	for(b=0;b<l2_cache_assoc;b++)// l2 cache search
	          {
	 	          if((l2_cache[set].flags[b]&valid_bit_mask)&&(l2_cache[set].tag[b]==tag))
	 	              {
	 	//              printf("L2 CACHE hit: tag=%x set=%x block=%x offset=%x\n",tag,set,b,offset);
	                    hit=1;
	                    l2_block=b;
	 	                break;// l2 cache hit
	                  }
	           }
         if(!hit)// l2 cache miss
         {  
            //printf("L2 CACHE miss: tag=%x set=%x offset=%x\n",tag,set,offset);
         	l2_cache_miss++;
         	address_bus[memory_l2cache].switching_activity+=get_hamming_distance(address_bus[memory_l2cache].address_lines,addr);
	        address_bus[memory_l2cache].address_lines=addr;
         	//printf("L2 CACHE page replacement: in set=%x ",set);
           if(l2_policy&pg_rep_policy_mask)
                l2_block=FIFO_page_replacement(&l2_cache[set],l2_cache_assoc,l2cache_l1dcache);	
	         else
		        l2_block=LRU_page_replacement(&l2_cache[set],l2_cache_assoc,l2cache_l1dcache);
           if(!(l2_policy&wr_policy_mask)&&(l2_cache[set].flags[l2_block]&dirty_bit_mask))// check dirty bit 
              {
               	memory_start_address=get_memory_start_address(l2_cache[set].tag[l2_block],set);// main memory block to replace l2 block
		   //   printf("L2 cache write back: from set %x, block %x to memory[%x]\n",set,l2_block,memory_start_address);
			   transfer_block(l2_cache[set].data[l2_block],main_memory+memory_start_address,memory_start_address,memory_l2cache,memory_l2cache); // write back an l2 block to main memory
		       num_mem_updates[memory_l2cache]+=block_size;// number of updates in main memory
		       	tot_sram_reads+=block_size;
		   	  	tot_dram_writes+=block_size;
			 }
           memory_start_address=get_memory_start_address(tag,set);// starting address of new main memory block to be transferred to l2
		  // printf("\nMemory start address %u \n",memory_start_address);
		   transfer_block(main_memory+memory_start_address,l2_cache[set].data[l2_block],memory_start_address,memory_l2cache,l2cache_l1dcache); // bring a main memory block to l2
           num_mem_updates[l2cache_l1dcache]+=block_size;// number of updates in l2 cache
           tot_dram_reads+=block_size;
		   tot_sram_writes+=block_size;
		   memory_content_switching_activity[l2cache_l1dcache]+=get_hamming_distance(l2_cache[set].tag[l2_block],tag); 	           	 
		   l2_cache[set].tag[l2_block]=tag;
		   prev=l2_cache[set].flags[l2_block];
           l2_cache[set].flags[l2_block]|=valid_bit_mask;
           curr=l2_cache[set].flags[l2_block];
		   memory_content_switching_activity[l2cache_l1dcache]+=get_hamming_distance(prev,curr);//flag bit switching activity      
		 }// end l2 cache miss
		 if(!(l2_policy&pg_rep_policy_mask))
		          LRU_update(&l2_cache[set],l2_cache_assoc,l2_block,l2cache_l1dcache);
		// printf("L1 DCACHE page replacement: in set=%x ",set);         
		 if(l1_policy&pg_rep_policy_mask)
		    l1_block=FIFO_page_replacement(&l1_dcache[set],l1_dcache_assoc,l1dcache_cpu);
	        else	 
		    l1_block=LRU_page_replacement(&l1_dcache[set],l1_dcache_assoc,l1dcache_cpu);
            if(!(l1_policy&wr_policy_mask)&&(l1_dcache[set].flags[l1_block]&dirty_bit_mask))	
                    {
                       l2_block=0;
					   for(b=0;b<l2_cache_assoc;b++)// find the l2 block to write back l1 block
                          {
                              	if(l2_cache[set].tag[b]==l1_dcache[set].tag[l1_block])
                                 	{
                                      		l2_block=b;
                                    		break;
		                  	     }
		                  }    
       	            transfer_block(l1_dcache[set].data[l1_block],l2_cache[set].data[l2_block],0,l2cache_l1dcache,l2cache_l1dcache); // replace an l1 block to l2 if Write back and dirty bit is 1
       	            num_mem_updates[l2cache_l1dcache]+=block_size;
       	            tot_sram_reads+=block_size;
		   	  	     tot_sram_writes+=block_size;
       				} 
 		transfer_block(l2_cache[set].data[l2_block],l1_dcache[set].data[l1_block],0,l2cache_l1dcache,l1dcache_cpu); // bring a new l2 block to relaced l1 block 
 		num_mem_updates[l1dcache_cpu]+=block_size;
 		tot_sram_reads+=block_size;
		tot_sram_writes+=block_size;
 		memory_content_switching_activity[l1dcache_cpu]+=get_hamming_distance(l1_dcache[set].tag[l1_block],tag); 	           	 
        l1_dcache[set].tag[l1_block]=tag;
        prev=l1_dcache[set].flags[l1_block];
        l1_dcache[set].flags[l1_block]|=valid_bit_mask;
        curr=l1_dcache[set].flags[l1_block];
		memory_content_switching_activity[l1dcache_cpu]+=get_hamming_distance(prev,curr);  //flag bit switching activity      
	  }	 // end of l1 dcache miss
	   if(access=='r') 
	     {
	       switch(type)
		     {
	           case 'c':data=l1_dcache[set].data[l1_block][offset];  // l1 read 
						data_bus[l1dcache_cpu].switching_activity+=get_hamming_distance(data_bus[l1dcache_cpu].data_lines[0],data);
	                    data_bus[l1dcache_cpu].data_lines[0]=data;	  
						num_bus_transfers[l1dcache_cpu]++;  
						tot_sram_reads+=2;// 1 icache read for fetch instruction and operand, 1 dcache read
						tot_clock_cycles+=3; // 1 cycle for icache read for fetch instruction and operand, 1 cycle for dcache reads, 1 cycle for assignment
	           break;
	           case 'i':data=get_integer(l1_dcache[set].data[l1_block],offset);
	           	   for(byte=0;byte<4;byte++)
	               		{
				           data_bus[l1dcache_cpu].switching_activity+=get_hamming_distance(data_bus[l1dcache_cpu].data_lines[byte],l1_dcache[set].data[l1_block][offset+byte]);
	                       data_bus[l1dcache_cpu].data_lines[byte]=l1_dcache[set].data[l1_block][offset+byte];	           
						}
						num_bus_transfers[l1dcache_cpu]++; 
					    tot_sram_reads+=5; // 1 icache read for fetch instruction and operand, 4 dcache reads
						tot_clock_cycles+=4;  //1 icache read for fetch instruction and operand, 2 cycles for dcache reads, 1 cycle for assignment  
	           break;
	           default: printf("Data type error!");
	         }
	     //  printf("r");
	     }
        else
         if(access=='w') 
          {
		   	//printf("w");	
		   	switch(type)
		     {
	           case 'c':memory_content_switching_activity[l1dcache_cpu]+=get_hamming_distance(l1_dcache[set].data[l1_block][offset],data);
			            l1_dcache[set].data[l1_block][offset]=data; // l1 write	                    
						data_bus[l1dcache_cpu].switching_activity+=get_hamming_distance(data_bus[l1dcache_cpu].data_lines[0],data);
	                    data_bus[l1dcache_cpu].data_lines[0]=data;	
						num_mem_updates[l1dcache_cpu]++;   // number of updates in l1 dcache
						num_bus_transfers[l1dcache_cpu]++; 
						tot_sram_reads++;//  1 icache read to fetch instruction and operand
						tot_sram_writes++;  // 1 dcache write		
						tot_clock_cycles+=3; // 1 cycle for fetch instruction and operand,1 cycle for execute 1 cycle for assignment
	           break;
	           case 'i':for(byte=0;byte<4;byte++)
		                  memory_content_switching_activity[l1dcache_cpu]+=get_hamming_distance(l1_dcache[set].data[l1_block][offset+byte],(data>>(8*byte))&0x000000ff); 	           	 
			            put_integer(l1_dcache[set].data[l1_block],data,offset);
	           	       for(byte=0;byte<4;byte++)
	               		{
				           data_bus[l1dcache_cpu].switching_activity+=get_hamming_distance(data_bus[l1dcache_cpu].data_lines[byte],l1_dcache[set].data[l1_block][offset+byte]);
	                       data_bus[l1dcache_cpu].data_lines[byte]=l1_dcache[set].data[l1_block][offset+byte];	           
						}
						num_mem_updates[l1dcache_cpu]+=4;   // number of updates in l1 dcache
						num_bus_transfers[l1dcache_cpu]++;  
						tot_sram_reads++;// 1 icache read to fetch instruction and operand
						tot_sram_writes+=4;// 4 dcache writes 		
						tot_clock_cycles+=4; // 1 cycles for fetch instruction and operand, 2 cycles for execute, 1 cycle for assignment
	           break;
	           default: printf("Data type error!");
	         }		   
		    if(l1_policy&wr_policy_mask)// write through l2
		      { 
			           l2_block=0;
					   for(b=0;b<l2_cache_assoc;b++)// find the l2 block for l1 block
                          {
                              	if(l2_cache[set].tag[b]==l1_dcache[set].tag[l1_block])
                                 	{
                                      		l2_block=b;
                                    		break;
		                  	     }
		                  }
		//		printf("L1 dcache write through: from set %x, block %x to L2 cache set %x, block %x\n",set,block,set,l2_block);    
			 switch(type) // write data to l2 data array
		         {
		         	case 'c':memory_content_switching_activity[l2cache_l1dcache]+=get_hamming_distance(l2_cache[set].data[l2_block][offset],data);
					         l2_cache[set].data[l2_block][offset]=data;
		         		     data_bus[l2cache_l1dcache].switching_activity+=get_hamming_distance(data_bus[l2cache_l1dcache].data_lines[0],data);
	                         data_bus[l2cache_l1dcache].data_lines[0]=data;	 
							 num_mem_updates[l2cache_l1dcache]++;  // number of updates in l2cache    
							 num_bus_transfers[l2cache_l1dcache]++;
							 tot_sram_writes++;  // write through takes place parallely, at l1-dcache and l2-cache in same cycles 
		         	break;
		         	case 'i':
					 for(byte=0;byte<4;byte++)
		                  memory_content_switching_activity[l2cache_l1dcache]+=get_hamming_distance(l2_cache[set].data[l2_block][offset+byte],(data>>(8*byte))&0x000000ff); 
					    put_integer(l2_cache[set].data[l2_block],data,offset);
		         	    for(byte=0;byte<4;byte++)
						 {
				           data_bus[l2cache_l1dcache].switching_activity+=get_hamming_distance(data_bus[l2cache_l1dcache].data_lines[byte],l2_cache[set].data[l2_block][offset+byte]);
	                       data_bus[l2cache_l1dcache].data_lines[byte]=l2_cache[set].data[l2_block][offset+byte];	           
						}
						num_mem_updates[l2cache_l1dcache]+=4;// number of updates in l2cache  
						num_bus_transfers[l2cache_l1dcache]++;  
						tot_sram_writes+=4;  // write through takes place parallely, at l1-dcache and l2-cache in same cycles 
		         	break;
	                default: printf("Data type error!");
				 }
		         if(!(l2_cache[set].flags[l2_block]&dirty_bit_mask)) 
		             {					
					   prev=l2_cache[set].flags[l2_block];
				       l2_cache[set].flags[l2_block]|=dirty_bit_mask;// set l2 block dirty bit 
				       curr=l2_cache[set].flags[l2_block];
		               memory_content_switching_activity[l2cache_l1dcache]+=get_hamming_distance(prev,curr);//flag bit switching activity      
		            }
				address_bus[l2cache_l1dcache].switching_activity+=get_hamming_distance(address_bus[l2cache_l1dcache].address_lines,offset);
        	    address_bus[l2cache_l1dcache].address_lines=offset;		  
			  }	 //end of write through     
		  }
		  else
		   {
		     printf("Access error!");
		   }
	    address_bus[l1dcache_cpu].switching_activity+=get_hamming_distance(address_bus[l1dcache_cpu].address_lines,offset);
	    address_bus[l1dcache_cpu].address_lines=offset;	
	   if(!(l1_policy&pg_rep_policy_mask))
	     LRU_update(&l1_dcache[set],l1_dcache_assoc,l1_block,l1dcache_cpu);  
	return data;
}
unsigned char FIFO_page_replacement(struct cache*c,unsigned char assoc,unsigned int mem_type)// FIFO
{
	unsigned char i=0,rep_pg,prev,curr;
	tot_clock_cycles++; //check whether FIFO queue is full
	if(c->entry<assoc)
	   { 
	   	   prev=c->access[c->entry];//for measuring switching activity of FIFO queue
		   c->access[c->entry]=c->entry;
		   tot_clock_cycles++; //update FIFO queue with new entry
		   curr=c->access[c->entry];//for measuring switching activity of FIFO queue
		   memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//FIFO switching activity    
	   	   rep_pg=c->entry;
	   	   prev=c->entry;//for measuring switching activity of FIFO counter
		   c->entry++;   
		   tot_clock_cycles++; //increment of FIFO counter
		   curr=c->entry;//for measuring switching activity	of FIFO counter
	       memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//FIFO switching activity    
	   }
	else
	{
		tot_clock_cycles+=2;//1 cycle for parallel check of c->access[i]==0 and 1 cycle for c->access[i]=assoc-1 or c->access[i]--
		for(i=0;i<assoc;i++)
		  {
			  if(c->access[i]==0)
		  	   {	 
				prev=c->access[i];//for measuring switching activity of FIFO queue
		  		c->access[i]=assoc-1;
		  		curr=c->access[i];//for measuring switching activity of FIFO queue
		  		memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//FIFO switching activity    
		  		rep_pg=i;
		  	   }
		  		else
		  		{
				  prev=c->access[i];//for measuring switching activity of FIFO queue	
		  		  c->access[i]--;
		  	      curr=c->access[i];//for measuring switching activity of FIFO queue
		  		  memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//FIFO switching activity    		  		  
		  	   }
		  }
	}
	//printf("block %x replaced\n",rep_pg);
	return rep_pg;
}
unsigned char LRU_page_replacement(struct cache*c,unsigned char assoc,unsigned int mem_type)// LRU
{
	unsigned char i=0,rep_pg=0,lru_pg,prev,curr;
	tot_clock_cycles++;  //check whether LRU data structure is full
	if(c->entry<assoc)
	   {
		   prev=c->access[c->entry];//for measuring switching activity of LRU data structure
	   	   c->access[c->entry]=c->entry;
	   	   tot_clock_cycles++;  //update LRU data structure 
	   	   curr=c->access[c->entry];//for measuring switching activity of LRU data structure
	   	   memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//LRU switching activity
	   	   rep_pg=c->entry;
	   	   prev=c->entry;//for measuring switching activity of LRU counter
		   c->entry++;  
		   tot_clock_cycles++;  //update LRU counter
		   curr=c->entry;//for measuring switching activity of LRU counter
		   memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//LRU switching activity 	
	   }
	else
	{
		prev=lru_pg;//lru_pg switching activity 
		lru_pg=c->access[0];
		curr=lru_pg;//lru_pg switching activity 
		 memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//LRU switching activity 
		for(i=1;i<assoc;i++)
		  {
			  if(c->access[i]>lru_pg)
		  	   {
				prev=lru_pg;	 //lru_pg switching activity 
		  		lru_pg=c->access[i];
		  		curr=lru_pg;//lru_pg switching activity 
		         memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//LRU switching activity 
		  		rep_pg=i;
		  	   }
		  }
		tot_clock_cycles+=assoc;//find lru_pg   
	}
	//printf("block %x replaced\n",rep_pg);
	return rep_pg;
}
void LRU_update(struct cache*c,unsigned char assoc,unsigned int pg,unsigned int mem_type)
{
	unsigned int i;
	unsigned char prev,curr;
	c->access[pg]=0;
	tot_clock_cycles++;// parallel update of LRU data structure
	for(i=0;i<c->entry;i++)
		  {		  	
			  if(i!=pg)
			  {
				prev=c->access[i];  //for measuring switching activity of LRU data structure  
		  		c->access[i]++;
		  		curr=c->access[i];	 //for measuring switching activity of LRU data structure 
        		memory_content_switching_activity[mem_type]+=get_hamming_distance(prev,curr);//LRU switching activity 			  		
		      }
		  }	
}
double transfer_block(unsigned char*src,unsigned char*tar,unsigned int start_addr,unsigned int bus_type,unsigned int to_mem_type)
{
	unsigned int i,j,byte,bus_transfers;
//	printf("transfer:");
        for(i=0;i<block_size;i+=bus_width[bus_type])
           {
           	 address_bus[bus_type].switching_activity+=get_hamming_distance(address_bus[bus_type].address_lines,start_addr+i);
	         address_bus[bus_type].address_lines=start_addr+i;
           	 for(byte=0;byte<bus_width[bus_type];byte++)
           	   {
           	   	 memory_content_switching_activity[to_mem_type]+=get_hamming_distance(tar[i+byte],src[i+byte]);
           	   	 tar[i+byte]=src[i+byte];
           	     data_bus[bus_type].switching_activity+=get_hamming_distance(data_bus[bus_type].data_lines[byte],src[i+byte]);
      	         data_bus[bus_type].data_lines[byte]=src[i+byte];      	   	
			  }
			 // num_bus_transfers[bus_type]++;
		   }
        bus_transfers=block_size/bus_width[bus_type]; // number of bus transfers currently required between memory and cache
		num_bus_transfers[bus_type]+=bus_transfers; //total number of bus transfers transfer between memory and cache
		tot_clock_cycles+=bus_transfers*cycles_per_bus_transfer;//delay of memory block transfer between memory and cache
}
unsigned int get_memory_start_address(unsigned int tag,unsigned int set)
{
	unsigned int start_address;
	start_address=(tag<<(set_bits+offset_bits))|(set<<offset_bits);
	return start_address; 
}
void put_integer(unsigned char*p,unsigned int data,unsigned int i)
{
	unsigned int byte;
	for(byte=0;byte<4;byte++)
		p[i+byte]=(data>>(8*byte))&0x000000ff;
}
unsigned int get_integer(unsigned char*p,unsigned int i)
{
	unsigned int data=0x00000000,val,byte;
	for(byte=0;byte<4;byte++)
	  {  
	    val=0x000000ff&p[i+byte];
	    data=data|(val<<(8*byte));
      }
      return data;
}
void termination()
{
  	//copy l2 blocks with dirty bits set to main memory
  unsigned int memory_start_address,block,set,offset;
  //printf("Program terminates\n");
  for(set=0;set<sets;set++)
    {
      for(block=0;block<l2_cache_assoc;block++)	
        {
	       if((l2_cache[set].flags[block]&valid_bit_mask)&&(l2_cache[set].flags[block]&dirty_bit_mask))
	        {
	        	memory_start_address=get_memory_start_address(l2_cache[set].tag[block],set);// main memory block to replace l2 block
		        transfer_block(l2_cache[set].data[block],main_memory+memory_start_address,memory_start_address,memory_l2cache,memory_l2cache); // write back l2 block to main memory		       
		        num_mem_updates[memory_l2cache]+=block_size;
		        tot_sram_reads+=block_size;
		   	  	tot_dram_writes+=block_size;
				//printf("%u ",main_memory+memory_start_address);
			   //for(offset=0;offset<64;offset+=4) 
		       // printf("(%u,%u,%u,%u),%u ",l2_cache[set].tag[block],set,block,offset,get_integer(l2_cache[set].data[block],offset));
		   }
       }
  }
}
unsigned int get_hamming_distance(unsigned int p,unsigned int q)
{
	unsigned int r,b,d=0;
	r=p^q;
	for(b=1;b!=0;b=b<<1)
	  {
	  	if(r&b)
	  	   d++;
	  }
	return d;  
}
void get_performance()
{
  unsigned int i;
  unsigned long long int total_bus_switching_activity=0L,total_content_switching_activity=0L,tot_cont_updates=0L,tot_bus_trans=0L;
  double tot_energy_bus=0.0,tot_energy_mem,tot_energy_sram,tot_energy_dram,tot_energy_cpu,tot_energy,alu_component_energy,tot_alu_energy=0.0;
  char mem_bus_type[][10]={"Memory","L2Cache","L1DCache","CPU"};
  char alu_component_name[][16]={"Comparator","Adder","Multiplier","Divider"};
  /*opening files to store simulation results*/
  printf("******Program performance*****\n");
  printf("Memory accesses=%llu\nL1-DCACHE misses=%llu\nL2-CACHE misses=%llu\n",mem_access,l1_dcache_miss,l2_cache_miss);    
  printf("###### Bus Transfers ######\n");
  for(i=0;i<3;i++)
    {
	  printf("%s-%s bus transfers:%llu\n",mem_bus_type[i],mem_bus_type[i+1],num_bus_transfers[i]);	
      tot_bus_trans+=num_bus_transfers[i];
   }  	
   printf("Total bus transfers: %llu\n",tot_bus_trans);
  printf("###### Content updations ######\n");
  for(i=0;i<3;i++)
    {
	  printf("%s content updates: %llu\n",mem_bus_type[i],num_mem_updates[i]);		
      tot_cont_updates+=num_mem_updates[i];
   }  	  
    printf("Total content updates: %llu\n",tot_cont_updates); 
  printf("###### Switching activities ######\n");
  for(i=0;i<3;i++)
    {
	  printf("%s-%s address bus:%llu\n",mem_bus_type[i],mem_bus_type[i+1],address_bus[i].switching_activity);	
	  printf("%s-%s data bus:%llu\n",mem_bus_type[i],mem_bus_type[i+1],data_bus[i].switching_activity);	
      printf("%s content: %llu\n",mem_bus_type[i],memory_content_switching_activity[i]);	
      total_bus_switching_activity+=address_bus[i].switching_activity+data_bus[i].switching_activity;  
      total_content_switching_activity+=memory_content_switching_activity[i];
    //  tot_energy_bus+=(0.5*bus_capacitance[i]*vdd*vdd*(double)address_bus[i].switching_activity/(double)num_bus_transfers[i])*(double)tot_clock_cycles;
    //  tot_energy_bus+=(0.5*bus_capacitance[i]*vdd*vdd*(double)data_bus[i].switching_activity/(double)num_bus_transfers[i])*(double)tot_clock_cycles;
     tot_energy_bus+=(0.5*bus_capacitance[i]*vdd*vdd*(double)address_bus[i].switching_activity*(double)cycles_per_bus_transfer);
      tot_energy_bus+=(0.5*bus_capacitance[i]*vdd*vdd*(double)data_bus[i].switching_activity*(double)cycles_per_bus_transfer); 
   }  	
 for(i=0;i<num_alu_components;i++)
 {
 	printf("%s switching:%llu\n",alu_component_name[i],alu_components[i].switching_activity);
 }
  printf("Total bus switching activity: %llu\n",total_bus_switching_activity);
   printf("Average bus switching activity: %0.2f\n",(float)total_bus_switching_activity/(float)tot_bus_trans);  
  printf("Total memory content switching activity: %llu\n",total_content_switching_activity);
  printf("Average memory content switching activity: %0.2f\n",(float)total_content_switching_activity/(float)tot_cont_updates);
  printf("###### Total execution time ######\n"); 
  printf("CPU clock cycles required: %llu\n",tot_clock_cycles);
  printf("###### Power Consumption ######\n");
  for(i=0;i<num_alu_components;i++)
 {
 	//alu_component_energy=0.5*alu_component_capacitance[i]*vdd*vdd*(double)alu_components[i].switching_activity*(double)tot_clock_cycles;
 	alu_component_energy=0.5*alu_component_capacitance[i]*vdd*vdd*(double)alu_components[i].switching_activity;//one cycle per switching activity
 	printf("%s energy (in nJ):%0.2g\n",alu_component_name[i],alu_component_energy*1.0e+9);
 	alu_components[i].switching_energy=alu_component_energy;
    tot_alu_energy+=alu_component_energy;
 }
  tot_energy_sram=sram_energy_per_read*tot_sram_reads+sram_energy_per_write*tot_sram_writes+sram_lkg_energy_per_cycle*sram_size*(double)tot_clock_cycles;// l1-dcache and l2 cache energy
  tot_energy_dram=(tot_dram_reads+tot_dram_writes)*dram_energy_per_access+dram_lkg_energy_per_cycle*dram_size*(double)tot_clock_cycles;// main memory energy
  tot_energy_mem=tot_energy_sram+tot_energy_dram;//memory system energy
  tot_energy_cpu=tot_alu_energy+(cpu_dyn_energy_per_cycle+cpu_lkg_energy_per_cycle)*(double)tot_clock_cycles;
  tot_energy=tot_energy_cpu+tot_energy_mem+tot_energy_bus;
  printf("l1-dcache & l2 cache (SRAM) energy (in nJ): %0.2g\n",tot_energy_sram*1.0e+9);
  printf("Main memory (DRAM) energy (in nJ): %0.2g \n",tot_energy_dram*1.0e+9);	
  printf("Memory system energy (in nJ): %0.2g\n",tot_energy_mem*1.0e+9);
  printf("Bus energy (in nJ): %0.2g\n",tot_energy_bus*1.0e+9);
  printf("CPU energy (in nJ): %0.2g\n",tot_energy_cpu*1.0e+9);
  printf("Total energy (in nJ): %0.2g\n",tot_energy*1.0e+9);
  /*writing program performance to files*/
  parameter_values[tot_energy_benchmark]=tot_energy*1.0e+9;//parameter 0
  parameter_values[delay_benchmark]=(double)tot_clock_cycles;//parameter 1
  parameter_values[content_switching_activity_benchmark]=(double)total_content_switching_activity;//parameter 2
  parameter_values[content_switching_energy_benchmark]=tot_energy_mem*1.0e+9;// parameter 3
  parameter_values[bus_switching_activity_benchmark]=(double)total_bus_switching_activity;//parameter 4
  parameter_values[bus_switching_energy_benchmark]=tot_energy_bus*1.0e+9;// parameter 5
  parameter_values[cmp_switching_activity_benchmark]=(double)alu_components[comparator].switching_activity;//parameter 6
  parameter_values[cmp_switching_energy_benchmark]=(double)alu_components[comparator].switching_energy*1.0e+9;// parameter 7
 // print_performance();
}
void compare(unsigned int A,unsigned int B) 
{	
	unsigned int eq,gt,lt,x,y,b,prev,curr;
	prev=flag_reg;
	flag_reg=0U;// reset flag register
	curr=flag_reg;
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	tot_clock_cycles+=1;  
	 for(b=0x80000000;b!=0;b=b>>1)
	 {
	 	x=A&b;
	 	y=B&b;
	 	prev=0;
	 	curr=(x&~y);
	     alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	 	if((x&~y)!=0)
	 	 {
	 	 prev=flag_reg;	
		  flag_reg=2U;// A>B   
		  curr=flag_reg;
		  alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
   		  return;
		 }
		 else
		 {
		   	prev=0;
	     	curr=(~x&y);
	        alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr); 
	 	   if((~x&y)!=0)
	 	     {
	 	      prev=flag_reg;		
		      flag_reg=4U;// A<B 
			  curr=flag_reg;
		       alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);  
   		       return;
		    }
	    }
		 
	 }
	 prev=flag_reg;	
	 flag_reg=1U;
	 curr=flag_reg;
	 alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
}
void compares(int a,int b)
{
    unsigned int x,y,prev,curr;
    prev=flag_reg;
	flag_reg=0U;
	curr=flag_reg;
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
    prev=0;
    x=a&signbit_mask;
    curr=x;
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	prev=0;
    y=b&signbit_mask;
    curr=y;
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);    
	prev=0;
	curr=(x&~y);
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	tot_clock_cycles+=1; 
	if((x&~y)!=0)
	 	 {
		  	prev=0;
	        curr=(~x&y);
	        alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		  if((~x&y)==0)
		   {
		     prev=flag_reg;
		     flag_reg=4U;// a<b ,   a negative and b positive
		     curr=flag_reg;
		     alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		   }
   		  else 
   		   {
			  prev=0;
			  curr=b&magnitude_mask;
			  alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
			  prev=0;
			  curr=a&magnitude_mask;
			  alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
   		     compare(b&magnitude_mask,a&magnitude_mask); // compare magnitude, both a and b negative
   		    }
		 }
	else
		 {
		 	prev=0;
	        curr=(~x&y);
	        alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		 	if((~x&y)!=0)
		 	 {
		 	   prev=flag_reg;
			   flag_reg=2U;// a>b,   a positive and b negative
			   curr=flag_reg;
		       alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		    }
			else
		   	 {
			   prev=0;
			  curr=a&magnitude_mask;
			  alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
			  prev=0;
			  curr=b&magnitude_mask;
			  alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		    	compare(a&magnitude_mask,b&magnitude_mask); // compare magnitude  a and b positive	 	
		     }
		 }
}
void comparef(float a,float b) 
{
	unsigned int *p,*q,x,y,prev,curr;
	p=(unsigned int*)&a;
	q=(unsigned int*)&b;
	prev=flag_reg;
	flag_reg=0U;
	curr=flag_reg;
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
    prev=0;
	x=(*p)&signbit_mask;
	curr=x;
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	prev=0;
    y=(*q)&signbit_mask;
    curr=y;
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	prev=0;
	curr=(x&~y);
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	tot_clock_cycles+=1; 
	/* masking of mantissa */
	prev=0;
	curr=(*p)&EXP_MASK;	
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	prev=0;
	curr=(*q)&EXP_MASK;	
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	/* masking of exponent */
	prev=0;
	curr=(*p)&MAN_MASK;	
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	prev=0;
	curr=(*q)&MAN_MASK;	
	alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
	/* check for NAN INF a*/
	compare((*p)&EXP_MASK,EXP_NAN_INF);// check EXP of a
	if(flag_reg==1)
	 {
	 	compare((*p)&MAN_MASK,MAN_INF);// check MAN of a
	 	if(flag_reg==0)
	 	 {
		  printf("a is NAN\n");
	      return;
		 }
	 }
	 	/* check for NAN INF b*/
	compare((*q)&EXP_MASK,EXP_NAN_INF);// check EXP of b
	if(flag_reg==1)
	 {
	 	compare((*q)&MAN_MASK,MAN_INF);// check MAN of b
	 	if(flag_reg==0)
	 	 {
		  printf("b is NAN\n");
	      return;
		 }
	 }
		/* compare -0.0 +0.0 */
	compare((*p)&magnitude_mask,ZERO);// check magnitude of a = 0
	if(flag_reg==1)
	  {
		compare((*q)&magnitude_mask,ZERO);// check magnitude of b = 0
	 	if(flag_reg==1)
	 	 {
		  flag_reg=1;
	      return;
		 }
	 }
	if((x&~y)!=0)
	 	 {
	 	 	prev=0;
	        curr=(~x&y);
	        alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		  if((~x&y)==0)
		   {
		     prev=flag_reg;
		     flag_reg=4U;// a<b ,   a negative and b positive
		     curr=flag_reg;
		     alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		   }
   		  else 
   		    {
   		      compare((*q)&EXP_MASK,(*p)&EXP_MASK); // compare exponent, both a and b negative
				 if(flag_reg==1)// equal exponents
				  	 compare((*q)&MAN_MASK,(*p)&MAN_MASK); // compare mantissa, both a and b negative
   		    }
		 }
	else
		 {
		 	prev=0;
	        curr=(~x&y);
	        alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		 	if((~x&y)!=0)
		 	 	{
		          prev=flag_reg;
		 	      flag_reg=2U;// a>b,   a positive and b negative
		 	      curr=flag_reg;
		          alu_components[comparator].switching_activity+=get_hamming_distance(prev,curr);
		        }	      
			else
			{	
			  compare((*p)&EXP_MASK,(*q)&EXP_MASK); // compare exponent, both a and b positive
				 if(flag_reg==1)// equal exponents
				  	 compare((*p)&MAN_MASK,(*q)&MAN_MASK); // compare mantissa, both a and b positive  
		    }
		 }
}
unsigned int add(unsigned int A,unsigned int B,unsigned int C)// 32-bit full adder; input A, B and carry in C; output sum S and carry out C
{
	unsigned int Ai,Bi,b,Si;
	unsigned int S=0,carry_out=0;
	static unsigned int prev_sum,prev_carry;
	for(b=1;b!=0;b=b<<1)
	  {
	     Ai=A&b;
	     Bi=B&b;
         C=C&b;
         Si=Ai^Bi^C;
	     S=S|Si;
	     C=(Ai&Bi)|(Bi&C)|(Ai&C);  
	     carry_out=carry_out|C;
	     C=C<<1;
	  }
	alu_components[adder].switching_activity+=get_hamming_distance(prev_sum,S);
	prev_sum=S;  
	alu_components[adder].switching_activity+=get_hamming_distance(prev_carry,carry_out);
	prev_carry=carry_out;
	return S;  
}
unsigned int ADD(unsigned int A,unsigned int B)// 32-bit add instruction 
{
  unsigned int sum;
  sum=add(A,B,0);
  return sum;	
}
unsigned int SUB(unsigned int A,unsigned int B)// 32-bit subtract instruction
{
  unsigned int diff, B_comp;
  static unsigned int prev_bcomp;
  B_comp=~B;
  alu_components[adder].switching_activity+=get_hamming_distance(prev_bcomp,B_comp);
  prev_bcomp=B_comp;
  diff=add(A,B_comp,1);// A-B = A + 2's complement of B
  return diff;
}
unsigned int MUL(unsigned int A,unsigned int B)
{
	static unsigned char mul_by_one;
	unsigned char mul_by_one_switching_activity=0;
	unsigned int b,product=0;
	unsigned long long int mul_switching_activity=alu_components[adder].switching_activity;//total adder_switching_activity before mul
	for(b=1;b!=0;b=b<<1)
	 {
		 if(B&b)
	 	  {
	 	  	if(mul_by_one==0) 
				{
				  mul_by_one=1;
			      mul_by_one_switching_activity++;
				}
		    product=add(product,A,0);//product=product+A;
	      }
	      else
	       {
	       	 if(mul_by_one==1) 
				{
				  mul_by_one=0;
			      mul_by_one_switching_activity++;
				}
		   }
	 	A=A<<1;  
	 }
	mul_switching_activity=alu_components[adder].switching_activity-mul_switching_activity;//total adder_switching_activity during mul
	alu_components[multiplier].switching_activity+=mul_switching_activity;//update total mul switcing activities updated after mul
	alu_components[multiplier].switching_activity+=mul_by_one_switching_activity;//add mul_by_one switching activity
	alu_components[adder].switching_activity-=mul_switching_activity;//deduct mul_switching_activity from adder_switching_activity after mul
	return product;
}
