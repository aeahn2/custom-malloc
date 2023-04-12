/**
 * Custom Malloc Wrapper
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define METADATA_SIZE sizeof(metadata_t)
#define LOG_CALL 0

typedef struct _metadata_t {
  unsigned int size;     // The size of the memory block.
  unsigned char isUsed;  // 0 if the block is free; 1 if the block is used.
  void *nextFree; // Pointer to next free metadata block
} metadata_t;

void *startOfHeap = NULL;
void *free_head = NULL; 

void printHeap() {
  metadata_t *curMeta = startOfHeap;
  void *endOfHeap = sbrk(0);
  printf("-- Start of Heap (%p) --\n", startOfHeap);
  while ((void *)curMeta < endOfHeap) {   
    printf("metadata for memory %p: (size=%d, isUsed=%d, nextFree=%p)\n", (void *)curMeta, curMeta->size, curMeta->isUsed, curMeta->nextFree);
    curMeta = (void *)curMeta + curMeta->size + METADATA_SIZE;
  }
  printf("-- End of Heap (%p) --\n\n", endOfHeap);
}

void printFreeList() {
  printf("Free List: "); 
  metadata_t *cur = free_head; 
  while (cur) {
    printf("(address = %p, nextFree = %p) ", (void *)cur, cur->nextFree); 
    cur = (void *)cur->nextFree; 
  }
  printf("\n"); 
}

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
  if (LOG_CALL) printf("in calloc()\n");
  size_t total_size = num * size; 
  void *ptr = malloc(total_size); 
  if (!ptr) {
    return NULL; 
  }
  memset(ptr, 0, total_size); 
  return ptr;
}


/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
  if (!startOfHeap) { startOfHeap = sbrk(0); }
  if (LOG_CALL) printf("in malloc()\n");
  
  void *curr = (void *)free_head; 
  metadata_t *curMeta;
  void *prev = NULL; 
  metadata_t *prev_meta; 
  // Prioritize finding first free memblock found with enough space
  while (curr) {
    curMeta = (metadata_t *)curr; 
    prev_meta = (metadata_t *)prev; 
    if (curMeta->size >= size) {
      // Viable free memblock found. 
      if (curMeta->size >= size + METADATA_SIZE) { // Free block large enough to block split 
        // find split_meta
        metadata_t *splitMeta = (void *)curMeta + METADATA_SIZE + size; 
        splitMeta->size = curMeta->size - size - METADATA_SIZE; 
        splitMeta->isUsed = 0; 
        // update free pointers
        splitMeta->nextFree = (void *)curMeta->nextFree; 
        if (prev_meta != NULL) {
          prev_meta->nextFree = (void *)splitMeta; 
        }
        if ((void *)curMeta == (void *)free_head) {
          free_head = (void *)splitMeta; 
        }
        // update current meta
        curMeta->size = size; 
        curMeta->isUsed = 1; 
        curMeta->nextFree = NULL; 
        return (void *)curMeta + METADATA_SIZE;
      } else { // Free block too small to block split 
        if (prev_meta != NULL) {
          prev_meta->nextFree = (void *)curMeta->nextFree; 
        }
        curMeta->isUsed = 1; 
        curMeta->nextFree = NULL; 
        return (void *)curMeta + METADATA_SIZE; 
      }
    }
    prev = curr; 
    curr = curMeta->nextFree; 
  }
  // No free memblock 
  void *meta_address = sbrk(METADATA_SIZE); 
  metadata_t *meta = (metadata_t *)meta_address; 
  meta->size = size; 
  meta->isUsed = 1; 
  void *ptr = sbrk(size); 
  meta->nextFree = NULL; 
  return ptr; 
}


/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
  if (LOG_CALL) printf("in free()\n");
  void *meta_address = (void *)ptr - METADATA_SIZE; 
  metadata_t *meta = (metadata_t *) meta_address; 
  if (meta->isUsed == 0) {
    return; 
  }
  
  meta->isUsed = 0; 
  if (!free_head) {
    free_head = (void *)meta; 
  } else {
    // COALESCE MEMORY 
    if ((void *)meta < (void *)free_head) {
      if ((void *)meta + meta->size + METADATA_SIZE == (void *)free_head) { 
        meta->size = meta->size + ((metadata_t *)free_head)->size + METADATA_SIZE; 
        meta->nextFree = (void *)((metadata_t *)free_head)->nextFree; 
        free_head = (void *)meta; 
      } else {
        meta->nextFree = (void *)free_head; 
        free_head = (void *)meta; 
      }
    } else {
      void *curFreeAddress = free_head;
      metadata_t *curFree; 
      while (curFreeAddress) { // Iterate through free list starting at the head
        metadata_t *curFree = (metadata_t *)curFreeAddress; 
        // Case: Newly freed meta is right after to current free block 
        if (((void *)curFree + curFree->size + METADATA_SIZE) == (void *)meta) {
          curFree->size += meta->size + METADATA_SIZE;
          // Checks if memory block directly after newly freed meta 
          if (curFree->nextFree != NULL && (((void *)curFree + curFree->size + METADATA_SIZE) == (void *)curFree->nextFree)) { // <-- Needed
            curFree->size += ((metadata_t *)curFree->nextFree)->size + METADATA_SIZE; 
            curFree->nextFree = (void *)((metadata_t *)curFree->nextFree)->nextFree; 
          }
          return; 
        }
        // Case: Newly freed meta is between curFree and the next free block 
        else if (curFree->nextFree != NULL && ((void *)meta < (void *)curFree->nextFree)) {
          // check if meta is directly before curFree->nextFree 
          if ((void *)meta + meta->size + METADATA_SIZE == (void *)curFree->nextFree) {
            meta->size += ((metadata_t *)curFree->nextFree)->size + METADATA_SIZE; 
            meta->nextFree = (void *)((metadata_t *)curFree->nextFree)->nextFree; 
            curFree->nextFree = (void *)meta; 
            return; 
          } else {
            meta->nextFree = (void *)curFree->nextFree; 
            curFree->nextFree = (void *)meta; 
            return;
          }
        } 
        // Case: End of free list and meta is after current block
        else if (!curFree->nextFree && ((void *)curFree + curFree->size + METADATA_SIZE) < (void *)meta) {
          curFree->nextFree = (void *)meta; 
          return; 
        }
        curFreeAddress = (void *)curFree->nextFree; 
      }
    }
  } 
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
  if (LOG_CALL) printf("in realloc()\n"); 
  if (!ptr) {
    return malloc(size); 
  }
  if (size == 0) {
    free(ptr); 
    return (void *)NULL; 
  }
  
  void *cur_meta_address = ptr - METADATA_SIZE; 
  metadata_t *p_meta = (metadata_t *)cur_meta_address; 

  if (p_meta->size >= size) {
    return ptr; 
  }
  void *curr = free_head; 
  metadata_t *cur_meta; 
  void *prev = NULL; 
  metadata_t *prev_meta; 
  while (curr) {
    cur_meta = (metadata_t *)curr; 
    prev_meta = (metadata_t *)prev; 
    if ((void *)p_meta + METADATA_SIZE + p_meta->size < (void *)curr) {
      break; 
    }
    // check if current free block is right after p_meta
    if ((void *)p_meta + METADATA_SIZE + p_meta->size == (void *)curr) { 
      if (p_meta->size + cur_meta->size + METADATA_SIZE >= size + METADATA_SIZE) {
        // Block split next free block 
        void *split_meta_address = (void *)p_meta + METADATA_SIZE + size; 
        metadata_t *split_meta = (metadata_t *)split_meta_address;  
        split_meta->size = (p_meta->size + cur_meta->size + METADATA_SIZE) - METADATA_SIZE; 
        split_meta->isUsed = 0;
        // Update free list
        split_meta->nextFree = cur_meta->nextFree; 
        if (prev_meta != NULL) {
          prev_meta->nextFree = split_meta; 
        }
        if ((void *)cur_meta == (void *)free_head) {
          free_head = split_meta; 
        }

        p_meta->size = size; 
        p_meta->isUsed = 1; 
        p_meta->nextFree = NULL; 
        return ptr; 
      } else {
        // too small to block split 
        if (prev_meta != NULL) {
          prev_meta->nextFree = cur_meta->nextFree; 
        }
        p_meta->size = size; 
        p_meta->isUsed = 1; 
        p_meta->nextFree = NULL; 
        return ptr;
      }
    }
    prev = curr; 
    curr = cur_meta->nextFree; 
  }

  void *new_ptr = malloc(size); 
  if (!new_ptr) {
    return (void *)NULL; 
  }
  memcpy(new_ptr, ptr, p_meta->size); 
  free(ptr); 
  return new_ptr;
}


