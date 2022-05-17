#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef BITMAP_SIZE
#define BITMAP_SIZE 4
#endif

#ifndef MY_ADDR
#define MY_ADDR 4026544704
#endif

#ifndef TEST_NUM
#define TEST_NUM 1
#endif

unsigned int get_top_bits(unsigned int value,  int num_bits);
void set_bit_at_index(char *bitmap, int index);
int get_bit_at_index(char *bitmap, int index);


unsigned int master_get_top_bits(unsigned int value,  int num_bits){
	if (num_bits > sizeof(value)*8){
		return 0;
	}
	return value >> ((sizeof(value)*8)-num_bits);
}

void master_set_bit_at_index(char *bitmap, int index,bool big_endian){
	int offset = index%8;
	int idx = index/8;
	if(big_endian){
		bitmap[idx] |= 1 << (7-offset);
	}else{
		bitmap[idx] |= 1 << (offset);
	}
	
}

int master_get_bit_at_index(char *bitmap, int index,bool big_endian){
	int offset = index%8;
	int idx = index/8;
	if(big_endian){
		return (1&(bitmap[idx] >> (7-offset)));
	}else{
		return (1&(bitmap[idx] >> (offset)));
	}
	
}


//autochecks for big endian and little endian tests
float set_even_bits(char* bitmap,void (*student_set)(char *, int), int (*master_get)(char *, int, bool),float total){

	printf("set_even_bits:\n");
	int iters[] = {0,0};
	double scores[] = {0,0};

	int max_len = 100000;
	char* comments [2];
	comments[0] = malloc(max_len);
	memset(comments[0],0, max_len);
	comments[1] = malloc(max_len);
	memset(comments[1],0, max_len);

	for(int big_endian=0;big_endian<=1;big_endian++){
		memset(bitmap,0, BITMAP_SIZE);
		for (int i =0; i < BITMAP_SIZE*8; i+=2){
			(*student_set)(bitmap, i);
		}

		for (int i =0; i < BITMAP_SIZE*8; i+=1){
			iters[big_endian]++;
			int ans = (*master_get)(bitmap, i,big_endian);
			if(((i%2==0) && ans==1) || ((i%2==1) && ans==0)){
				scores[big_endian]+=1;
			}else{
				snprintf(comments[big_endian]+strlen(comments[big_endian]),max_len,"    set_even_bits Failed - Index: %d\n",i);
			}
			
		}
	}

	int winner = 0;
	if(scores[1] > scores[0]){
		winner=1;
	}
	printf("%s",comments[winner]);
	free(comments[0]);
	free(comments[1]);

	if (iters[winner]==scores[winner]){
		printf("    ");
		printf("All test cases passed!\n");
	}

	float final_score =  (total/iters[winner])*scores[winner];
	printf("  Score: %f/%f\n",final_score,total);
	return final_score;
}



//autochecks for big endian and little endian tests
float get_even_bits(char* bitmap,int (*student_get)(char *, int), void (*master_set)(char *, int, bool),float total){

	printf("get_even_bits:\n");

	int iters[] = {0,0};
	double scores[] = {0,0};

	int max_len = 100000;
	char* comments [2];
	comments[0] = malloc(max_len);
	memset(comments[0],0, max_len);
	comments[1] = malloc(max_len);
	memset(comments[1],0, max_len);


	for(int big_endian=0;big_endian<=1;big_endian++){
		memset(bitmap,0, BITMAP_SIZE);
		for (int i =0; i < BITMAP_SIZE*8; i+=2){
			(*master_set)(bitmap, i,big_endian);
		}

		for (int i =0; i < BITMAP_SIZE*8; i+=1){
			iters[big_endian]++;
			int ans = (*student_get)(bitmap, i);

			if(((i%2==0) && ans==1) || ((i%2==1) && ans==0)){
				scores[big_endian]+=1;
			}else{
				
				snprintf(comments[big_endian]+strlen(comments[big_endian]),max_len,"    get_even_bits Failed - Index %d\n",i);
			}
		}
	}

	int winner = 0;
	if(scores[1] > scores[0]){
		winner=1;
	}
	printf("%s",comments[winner]);
	free(comments[0]);
	free(comments[1]);

	if (iters[winner]==scores[winner]){
		printf("    ");
		printf("All test cases passed!\n");
	}

	float final_score =  (total/iters[winner])*scores[winner];
	printf("  Score: %f/%f\n",final_score,total);
	return final_score;
}


float top_bits_test(unsigned int value,unsigned int (*student_get)(unsigned int, int), unsigned int (*master_get)(unsigned int, int),float total){
	printf("top_bits_test:\n");
	int iter = 0;
	double score = 0;
	for (int i =1; i < 10; i++){
		iter++;
		unsigned int ans = (*student_get)(value, i);
		unsigned int key = (*master_get)(value, i);
		if( ans == key){
			score++;
		}else{
			printf("    ");
			printf("get_top_bits Failed - Student Output: %d, Answer Key: %d, num_bits: %d, myaddress: %u\n",ans,key,i,value);
		}
	}

	if (iter==score){
		printf("    ");
		printf("All test cases passed!\n");
	}
	float final_score = (total/iter)*score;
	printf("  Score: %f/%f\n",final_score,total);
	return final_score;
}


void main(void){
	setbuf(stdout, NULL);
	float total_score = 0;
	printf("Parameters:\n");
	printf("  ");
	printf("myaddress: %lu, BITMAP_SIZE: %d\n",MY_ADDR,BITMAP_SIZE);

	total_score += top_bits_test(MY_ADDR,get_top_bits,master_get_top_bits,2.0);

	char* bitmap = (char *)malloc(BITMAP_SIZE);
    memset(bitmap,0, BITMAP_SIZE);

	total_score += set_even_bits(bitmap,set_bit_at_index,master_get_bit_at_index,4.0);
	memset(bitmap,0, BITMAP_SIZE);
	
	total_score += get_even_bits(bitmap,get_bit_at_index,master_set_bit_at_index,4.0);
	memset(bitmap,0, BITMAP_SIZE);

    free(bitmap);

	printf("\n");
	printf("Total Score Test %d: %f/%f\n",TEST_NUM,total_score,2.0+4.0+4.0);
	printf("--------------------------------------------");
	printf("\n\n");
    exit(0);

}
