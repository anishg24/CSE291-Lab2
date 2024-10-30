#include "utility.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>

#define BUFF_SIZE (1<<21)
#define SENSITIVITY 40
#define LINE_SIZE 64
#define SECRET_SIZE 100
#define BOUND 20

//** Write your victim function here */
// Assume secret_array[47] is your secret value
// Assume the bounds check bypass prevents you from loading values above 20
// Use a secondary load instruction (as shown in pseudo-code) to convert secret value to address

// void victim_function(int idx) {
//   int tmp;
//   if (idx < BOUND)
//     tmp = dummy_array[secret_array[idx*LINE_SIZE] * LINE_SIZE];
// }

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
    int *secret_array = (int *)huge_page;
    int *dummy_array = (int *)huge_page;

    // Initialize the array
    for (int i = 0; i < SECRET_SIZE*LINE_SIZE; i += LINE_SIZE) {
      secret_array[i] = i/LINE_SIZE;
    }

    //** STEP 2: Mistrain the branch predictor by calling victim function here */
    // To prevent any kind of patterns, ensure each time you train it a different number of times

    // for (int i = 0; i < BOUND; i++) {
    //   victim_function(i); 
    // } 
    //
    // lfence();
    //
    // //** STEP 3: Clear cache using clflush from utility.h */
    // for (int a = 0; a < SECRET_SIZE*LINE_SIZE; a += LINE_SIZE) {
    //   clflush(&secret_array[a]);
    // }

    //** STEP 4: Call victim function again with bounds bypass value */
    int message = 47;
    secret_array[message * LINE_SIZE] = 1;
    // victim_function(message);
    // clflush(&message);

    //** STEP 5: Reload mmap to see load times */
    // Just read the mmap's first 100 integers
    for (int j = 0; j < SECRET_SIZE; j++) {
      if (measure_one_block_access_time((uint64_t)(secret_array + (j*LINE_SIZE))) < SENSITIVITY) {
        printf("Line is %d.\n", j);
      }
    }

    printf("Done.\n");

    return 0;
}
