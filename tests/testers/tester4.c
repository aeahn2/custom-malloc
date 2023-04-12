#include "tester-utils.h"

// #define START_MALLOC_SIZE (128 * M) //1,048,576 --> 134,217,728
// #define STOP_MALLOC_SIZE (1 * K) //1024

#define START_MALLOC_SIZE (1 * (1024L * 1024L)) 
#define STOP_MALLOC_SIZE (1 * 1024L) 

void *reduce(void *ptr, int size) {
    // printf("Inside: reduce(%p, %d):\n", ptr, size); 
    if (size > STOP_MALLOC_SIZE) {
        void *ptr1 = realloc(ptr, size / 2);
        void *ptr2 = malloc(size / 2);

        if (ptr1 == NULL || ptr2 == NULL) {
            fprintf(stderr, "Memory failed to allocate!\n");
            exit(1);
        }

        ptr1 = reduce(ptr1, size / 2);
        ptr2 = reduce(ptr2, size / 2);
        // printf("AFTER REDUCING!!\n"); 
        // printf("ptr1 @ %p, %d\n", ptr1, *((int *)ptr1));
        // printf("ptr2 @ %p, %d\n", ptr2, *((int *)ptr2));

        if (*((int *)ptr1) != size / 2) {
            fprintf(stderr, "ptr1 - Memory failed to contain correct data after many "
                            "allocations!\n");
            exit(2);
        }
        if (*((int *)ptr2) != size / 2) {
            fprintf(stderr, "ptr2 - Memory failed to contain correct data after many "
                            "allocations!\n");
            exit(2);
        }

        free(ptr2);
        ptr1 = realloc(ptr1, size);
        // printf("-- after free and realloc to size: %d\n", size); 
        // printf("ptr1 @ %p, %d\n", ptr1, *((int *)ptr1));
        // printf("ptr2 @ %p, %d\n", ptr2, *((int *)ptr2));

        if (*((int *)ptr1) != size / 2) {
            fprintf(stderr,
                    "Memory failed to contain correct data after realloc()!\n");
            exit(3);
        }
        // printf("Success!\n\n"); 

        *((int *)ptr1) = size;
        return ptr1;
    } else {
        *((int *)ptr) = size;
        return ptr;
    }
}

int main() {
    (void) malloc(1);

    int size = START_MALLOC_SIZE;
    while (size > STOP_MALLOC_SIZE) {
        // printf("Current size = %d\n\n", size); 
        void *ptr = malloc(size);
        ptr = reduce(ptr, size / 2);
        free(ptr);

        size /= 2;
    }

    fprintf(stderr, "Memory was allocated, used, and freed!\n");
    return 0;
}
