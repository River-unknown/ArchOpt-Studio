/*This program performs ALU-based addition on N-bit unsigned integers*/
#include<stdio.h>
#include"memory.h"
//#define N 32
unsigned int addr_X[4][3];// memory locations allocated to X
unsigned int addr_Y[4][1];// memory locations allocated to Y
unsigned int addr_W[1][3];// memory locations allocated to synapse weights W 
unsigned int addr_input[1][3];// memory locations allocated to input
unsigned int addr_output;
int X[4][3]={ //or and bias input
                   {-1,-1,1},
	               {-1,1,1},
	               {1,-1,1},
	               {1,1,1}
	             };
int Y[4][1]={// or output
                   {-1},
	               {1},
	               {1},
	               {1}
				 };
int input[3];
void initialization();
void train_ann();
int or_ann();
void get_output();
int main()
{
	int x,y,z;
	printf("Enter x and y:");
	scanf("%d %d",&x,&y);
	input[0]=(x)?1:-1;//x
	input[1]=(y)?1:-1;//y
	input[2]=1;//bias
	initialization();
	train_ann();
	or_ann();
	termination();
	get_output();
	get_performance();
	return 0;
}
void initialization()
{
	unsigned int i,j,loc=0;
	l1_policy=0x02;//LRU Write through
    l2_policy=0x00;// LRU Write back;
    for(i=0;i<4;i++)
	  {
	  	for(j=0;j<3;j++)
	        {
         	    addr_X[i][j]=loc;	
				put_integer(main_memory,X[i][j],loc);
				loc=loc+4; 
            }
      }
    for(i=0;i<4;i++)
	  {
	  	for(j=0;j<1;j++)
	        {
         	    addr_Y[i][j]=loc;
				put_integer(main_memory,Y[i][j],loc);
				loc=loc+4; 
            }
      }
    for(i=0;i<1;i++)
	  {
	  	for(j=0;j<3;j++)
	        {
         	    addr_W[i][j]=loc;
				loc=loc+4; 
            }
      } 
	for(i=0;i<3;i++)
	{
		addr_input[0][i]=loc;
		put_integer(main_memory,input[i],loc);
		loc=loc+4; 
	}
	addr_output=loc;
}
void train_ann()
{
	unsigned char i,j,k;
	int Xkj,Yik,Wij;
	for(i=0;i<1;i++)
	  {

	  	for(j=0;j<3;j++)
	  	{
	  	  Wij=0;//sum=0;	
	  	  for(k=0;k<4;k++)	
		   {
		   	    Xkj=(int)memory_access(addr_X[k][j],'r',0,'i');
		   	    Yik=(int)memory_access(addr_Y[i][k],'r',0,'i');
				//Wij=Wij+Xkj*Yik;//sum=sum+X[k][j]*Y[i][k];//Y[i][k]*X[k][j]
				Wij=ADD(Wij,MUL(Xkj,Yik));
		   }
		   memory_access(addr_W[i][j],'w',Wij,'i');//W[i][j]=sum;
		}
	  }
}
int or_ann()
{
	unsigned char i,j;
	int Wij,input_j,sum=0;
	for(i=0;i<1;i++)
	  {
	  	for(j=0;j<3;j++)
	  	{
		  Wij=(int)memory_access(addr_W[i][j],'r',0,'i');
		  input_j=(int)memory_access(addr_input[0][j],'r',0,'i');
		  //sum=sum+Wij*input_j; //sum= sum + W[i][j]*input[j];
		  sum=ADD(sum,MUL(Wij,input_j));
		}  
     }
    compares(sum,2);
	memory_access(addr_output,'w',((flag_reg==2U)||(flag_reg==1U))?1:-1,'i');//W[i][j]=sum;
    //memory_access(addr_output,'w',(sum>=2)?1:-1,'i');//W[i][j]=sum; 
    return (sum>=2)?1:-1;
}
void get_output()
{
	unsigned char i,j;
	int z;
	printf("*****Program Output*****\nOR neural network\n");
	printf(" Input Bias  Output\n");
	for(i=0;i<4;i++)
	  {
	  	for(j=0;j<3;j++)
	  	{
	  		printf("%2d ",get_integer(main_memory,addr_X[i][j]));
	    }
	  	printf("     %2d\n",get_integer(main_memory,addr_Y[i][0]));
     }
	printf("Neuron weights: ");
	for(i=0;i<1;i++)
	  {
	  	for(j=0;j<3;j++)
	  	{
	  	  printf("%d ",get_integer(main_memory,addr_W[i][j]));
	    }
      }
    printf("\nNeural network input and bias: ");
    	for(i=0;i<1;i++)
	  {
	  	for(j=0;j<3;j++)
	  	{
	  	  printf("%d ",get_integer(main_memory,addr_input[i][j]));
	    }
      }
	z=(int)get_integer(main_memory,addr_output);
	printf("\nNeural network output: %d\n",z);  
	printf("Actual output: %u\n",(z==-1)?0:1);    
}
