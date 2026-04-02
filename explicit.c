#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>
#include <stdio.h>

typedef size_t header_t;
#define BYTES_PER_LINE 32
#define HEADER_SIZE 8
#define SMALLEST_MEM_BLOCK 16 + HEADER_SIZE // this incldes the header size

static void *segment_start;
static size_t segment_size;
static size_t nused;
static header_t* free_list_head;

bool read_header(void* cur, size_t* paylod_ptr);
size_t roundup(size_t sz, size_t mult);

header_t** prev_ll_header(header_t* cur) {
    return (header_t**)((char*)cur + 1 * HEADER_SIZE);
}

header_t** next_ll_header(header_t* cur) {
    return (header_t**)((char*)cur + 2 * HEADER_SIZE);
}

header_t* get_header(void* payload_ptr) {
    return (header_t*)((char*)payload_ptr - HEADER_SIZE);
}

header_t* get_next_header(void* ptr) {
    size_t payload;
    read_header(get_header(ptr), &payload);
    return (header_t*)((char*)ptr + payload);
}

header_t* get_next_header_and_payload(void* ptr, size_t* payload_ptr) {
    read_header(get_header(ptr), payload_ptr);
    header_t* next_header = (header_t*)((char*)ptr + *payload_ptr);
    read_header(next_header, payload_ptr);
    return next_header;
}

void add_to_free_list(header_t* ptr) {
    header_t** next = next_ll_header(ptr);
    *next = free_list_head;
    header_t** prev = prev_ll_header(ptr);
    *prev = NULL;
    if (free_list_head != NULL) {
        *prev_ll_header(free_list_head) = ptr;
    }
    free_list_head = ptr;
    *free_list_head &= ~1;
}

void remove_header_from_linked_list(header_t* ptr) {
    header_t* prev = *prev_ll_header(ptr);
    header_t* next = *next_ll_header(ptr);
    if (prev == NULL) {
        free_list_head = next;
    } else {
        *(header_t **)((char*)prev + 2 * HEADER_SIZE) = next;
    }
    if (next != NULL) {
        *(header_t **)((char*)next + HEADER_SIZE) = prev;
    }
    *(header_t **)((char*)ptr + HEADER_SIZE) = NULL;
    *(header_t **)((char*)ptr + 2 * HEADER_SIZE) = NULL;
}

void mark_as_allocated(void* ptr_to_header, size_t requested_size, size_t payload) {
    if (payload >= SMALLEST_MEM_BLOCK + requested_size) {
        *(size_t *)ptr_to_header = requested_size | 1;
        ptr_to_header = (char*)ptr_to_header + HEADER_SIZE + requested_size;
        *(size_t *)ptr_to_header = (payload - HEADER_SIZE - requested_size) & ~1;
        add_to_free_list((header_t*)ptr_to_header);
        return;
    }
    *(size_t *)ptr_to_header = payload | 1;
}

void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    }
    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }
    size_t requested_size = roundup(new_size, ALIGNMENT);
    if (requested_size > MAX_REQUEST_SIZE) {
        return NULL;
    }
    size_t payload;
    size_t cur_payload;
    header_t* header = get_header(old_ptr);
    read_header(header, &cur_payload);
    if (requested_size <= cur_payload) {
        mark_as_allocated(header, requested_size, cur_payload);
        return old_ptr;
    }
    size_t sum_payload = cur_payload;
    header_t *pivot = get_next_header(old_ptr);
    while ((char *)pivot < (char *)segment_start + segment_size) {
        size_t next_payload;
        if (read_header(pivot, &next_payload)) {
            break;
        }
        sum_payload += HEADER_SIZE + next_payload;
        if (sum_payload >= requested_size) {
            break;
        }
        pivot = get_next_header((char*)pivot + HEADER_SIZE);
    }
    if (requested_size <= sum_payload) {
        pivot = get_next_header(old_ptr);
        size_t total = cur_payload;
        while ((char*)pivot < (char*)segment_start + segment_size && total < requested_size) {
            size_t tmp_payload;
            if (read_header(pivot, &tmp_payload)) {
                break;
            }
            total += HEADER_SIZE + tmp_payload;
            remove_header_from_linked_list(pivot);
            pivot = get_next_header((char*)pivot + HEADER_SIZE);
        }
        mark_as_allocated(header, requested_size, total);
        return old_ptr;
    }
    void* new_memory_seg = mymalloc(new_size);
    if (new_memory_seg == NULL) {
        return NULL;
    }
    read_header((char*)old_ptr - HEADER_SIZE, &payload);
    size_t size = payload < new_size ? payload : new_size;
    memcpy(new_memory_seg, old_ptr, size);
    myfree(old_ptr);
    return new_memory_seg;
}

bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < SMALLEST_MEM_BLOCK) {
        return false;
    }
    segment_start = heap_start;
    segment_size = heap_size;
    *(size_t*)segment_start = (segment_size - HEADER_SIZE) & ~1;
    nused = HEADER_SIZE;
    free_list_head = (header_t *)heap_start;
    *next_ll_header(free_list_head) = NULL;
    *prev_ll_header(free_list_head) = NULL;
    return true;
}

bool read_header(void* cur, size_t* paylod_ptr) {
    *paylod_ptr = *(size_t*)cur & ~1;
    return *(size_t*)cur & 1;
}

size_t roundup(size_t sz, size_t mult) {
    sz = sz > 16 ? (sz + mult - 1) & ~(mult - 1) : 16;
    return sz;
}

void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    header_t *header = get_header(ptr);
    size_t payload_of_next_header;
    header_t *next_header = get_next_header(ptr);
    if ((char *)next_header < (char*)segment_start + segment_size && !read_header(next_header, &payload_of_next_header)) {
        remove_header_from_linked_list(next_header);
        size_t payload;
        read_header(header, &payload);
        *header = (payload + payload_of_next_header + HEADER_SIZE) & ~1;
    } else {
        *header &= ~1;
    }
    add_to_free_list(header);
}

void *mymalloc(size_t requested_size) {
    if (requested_size == 0) return NULL;
    if (requested_size > MAX_REQUEST_SIZE) return NULL;
    requested_size = roundup(requested_size, ALIGNMENT);
    header_t* cur = free_list_head;
    while (cur != NULL) {
        size_t payload;
        read_header(cur, &payload);
        if (requested_size <= payload) {
            remove_header_from_linked_list(cur);
            mark_as_allocated(cur, requested_size, payload);
            return (char*)cur + HEADER_SIZE;
        }
        cur = *next_ll_header(cur);
    }
    return NULL;
}
bool validate_heap() {
    if (segment_start == NULL) {
        printf("Heap start is NULL\n");
        breakpoint();
        return false;
    }

    if (segment_size < SMALLEST_MEM_BLOCK) {
        printf("Heap size is too small\n");
        breakpoint();
        return false;
    }

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
    printf("Heap segment starts at address %p, ends at %p. %lu bytes total.\n",
        segment_start, (char *)segment_start + segment_size, segment_size);

    for (size_t i = 0; i < segment_size; i++) {
        unsigned char *cur = (unsigned char *)segment_start + i;

        if (i % BYTES_PER_LINE == 0) {
            printf("\n%p: ", cur);
        }

        printf("%02x ", *cur);
    }
    printf("\n");
}