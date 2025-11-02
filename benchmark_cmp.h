#include<stdio.h>
#define max_benchmark 2
#define min_input_size 1
#define tot_parameters_benchmark 8
#define tot_energy_benchmark 0
#define delay_benchmark 1
#define content_switching_activity_benchmark 2
#define content_switching_energy_benchmark 3
#define bus_switching_activity_benchmark 4
#define bus_switching_energy_benchmark 5
#define cmp_switching_activity_benchmark 6
#define cmp_switching_energy_benchmark 7
#define sneak_path_energy_benchmark 8
#define benchmark_union 0
#define benchmark_bsort 1
#define benchmark_datatype_unsigned 0
#define benchmark_datatype_signed 1
#define benchmark_datatype_fp 2
unsigned char cpu_sim;
const char*benchmark_program[][3]={
                                    {"union_unsigned","union_signed","union_fp"},
                                    {"bsort_unsigned","bsort_signed","bsort_fp"}
								  };
const char*parameter_file[][3]={
                                {"unsigned_tot_energy.txt","signed_tot_energy.txt","fp_tot_energy.txt"},
                                {"unsigned_delay.txt","signed_delay.txt","fp_delay.txt"},
							    {"unsigned_mem_content_switching_activity.txt","signed_mem_content_switching_activity.txt","fp_mem_content_switching_activity.txt"},
							    {"unsigned_mem_content_switching_energy.txt","signed_mem_content_switching_energy.txt","fp_mem_content_switching_energy.txt"},
						        {"unsigned_bus_switching_activity.txt","signed_bus_switching_activity.txt","fp_bus_switching_activity.txt"},
							    {"unsigned_bus_switching_energy.txt","signed_bus_switching_energy.txt","fp_bus_switching_energy.txt"},
							    {"unsigned_cmp_switching_activity.txt","signed_cmp_switching_activity.txt","fp_cmp_switching_activity.txt"},
							    {"unsigned_cmp_switching_energy.txt","signed_cmp_switching_energy.txt","fp_cmp_switching_energy.txt"}
							   };
const char*parameter[]={"tot_energy","delay","mem_switching","mem_energy","bus_switching","bus_energy","cmp_switching","cmp_energy"};	
double parameter_values[tot_parameters_benchmark];						  					  
//unsigned int max_input_size[]={8388608,4096,8192,16};//2^23,2^12,2^13,2^4
unsigned int max_input_size[]={1024,1024};//2^10,2^10
unsigned int curr_benchmark,curr_input_size,curr_datatype;
void print_performance();
void print_performance()
{
	 FILE*fp[tot_parameters_benchmark];
	 unsigned int p,i;	
	 char file_access_mode[2];
	 sprintf(file_access_mode,((cpu_sim==1)&&(curr_benchmark==benchmark_union)&&(curr_input_size==min_input_size))?"w":"a");
	    for(p=0;p<tot_parameters_benchmark;p++)
          {
			 fp[p]=fopen(parameter_file[p][curr_datatype],file_access_mode);	 //open parameter files
             /*print x-axis values as input size*/
	         if((cpu_sim==1)&&(curr_input_size==min_input_size))
	            {
		              fprintf(fp[p],"x_n_%s=[",benchmark_program[curr_benchmark][curr_datatype]);
	                  for(i=min_input_size;i<=max_input_size[curr_benchmark];i=i*2)
	                    {
	    	                fprintf(fp[p],"%u",i);
	    	                if(i<max_input_size[curr_benchmark])
	    	                         fprintf(fp[p],",");
	                    }
		              fprintf(fp[p],"]\n");
		        }
		   }
          /*print y-axis values as performance value*/      
     if(curr_input_size==min_input_size)
       {
         for(p=0;p<tot_parameters_benchmark;p++)	  
	            fprintf(fp[p],(cpu_sim==1)?"y_%s_%s_cpu=[":"y_%s_%s_mpu=[",parameter[p],benchmark_program[curr_benchmark][curr_datatype]);
       }
	 for(p=0;p<tot_parameters_benchmark;p++)	 
       { 
        fprintf(fp[p],"%0.2g",parameter_values[p]);
        if(curr_input_size<max_input_size[curr_benchmark])
	                fprintf(fp[p],",");
       }
     if(curr_input_size==max_input_size[curr_benchmark])
       {
	    	for(p=0;p<tot_parameters_benchmark;p++)	  
	                fprintf(fp[p],"]\n");
	   }
  	 for(p=0;p<tot_parameters_benchmark;p++)	  //close parameter files
                    fclose(fp[p]); 
}
 
