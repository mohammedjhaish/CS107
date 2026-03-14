#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>
#include <assert.h>

#define BYTES_PER_LINE 32
#define HEADER_SIZE 8
#define SMALLEST_MEM_BLOCK 8 + HEADER_SIZE // this incldes the header size

static void *segment_start;
static size_t segment_size;
static size_t nused;


/* Function: roundup // from bump.c
 * -----------------
 * This function rounds up the given number to the given multiple, which
 * must be a power of 2, and returns the result.  (you saw this code in lab1!).
 */
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

//  a funvtion that reads the header, returns one if the block is allocated, and zero if the block is free, and also returns the payload size through the pointer argument

bool read_header(void* cur, size_t* paylod_ptr) {
    *paylod_ptr = *(size_t*)cur & ~1;
    return *(size_t*)cur & 1; // if 0 then free. 1 is Allocated
}

void mark_as_allocated(void* ptr_to_header, size_t requested_size, size_t payload) {
    if ((char*)ptr_to_header + HEADER_SIZE + requested_size < (char*)segment_start + segment_size - SMALLEST_MEM_BLOCK) {
        if (payload >= SMALLEST_MEM_BLOCK + requested_size) {
            *(size_t *)ptr_to_header = requested_size | 1;
            ptr_to_header = (char*)ptr_to_header + HEADER_SIZE + requested_size; // now points to the new header
            *(size_t *)ptr_to_header = (payload - HEADER_SIZE - requested_size) & ~1;
            return;
        }
    }
    *(size_t *)ptr_to_header = payload | 1;
}

bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < SMALLEST_MEM_BLOCK) {
        return false;
    }
    segment_start = heap_start;
    segment_size = heap_size;
    *(size_t*)segment_start = (segment_size - HEADER_SIZE) & ~1; // set the first header to be free and size 0
    nused = HEADER_SIZE;
    return true;
}

void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    *(size_t*)((char*)ptr - HEADER_SIZE) = *(size_t*)((char*)ptr - HEADER_SIZE) & ~1;
}

void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    }
    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }
    void* new_memory_seg = mymalloc(new_size);
    if (new_memory_seg == NULL) {
        return NULL;
    }
    size_t payload;
    read_header((char*)old_ptr - HEADER_SIZE, &payload);
    size_t size = payload < new_size ? payload : new_size;
    memcpy(new_memory_seg, old_ptr, size);
    myfree(old_ptr);
    return new_memory_seg;
}

void *mymalloc(size_t requested_size) {
    if (requested_size == 0) return NULL;
    if (requested_size > MAX_REQUEST_SIZE) return NULL;
    requested_size = roundup(requested_size, ALIGNMENT);
    void* cur = segment_start;
    while ((char*)cur < (char*)segment_start + segment_size) {
        size_t payload;
        bool is_allocated = read_header(cur, &payload);
        if (requested_size <= payload) {
            if (!is_allocated) {
                mark_as_allocated(cur, requested_size, payload);
                return (char*)cur + HEADER_SIZE;
            }
        }
        cur = (char*)cur + HEADER_SIZE + payload;
        continue;
    }
    return NULL;
}

bool validate_heap() {
    return true;
}

/* Function: dump_heap
 * -------------------
 * This function prints out the the block contents of the heap.  It is not
 * called anywhere, but is a useful helper function to call from gdb when
 * tracing through programs.  It prints out the total range of the heap, and
 * information about each block within it.
 */
void dump_heap() {
    // TODO(you!): Write this function to help debug your heap.
}
