//
// Created by student on 6/18/21.
//

#include <unistd.h>

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata *next;
    MallocMetadata *prev;
};

MallocMetadata *head = nullptr;

// Do we want to hold a class?
// From pdf: You should always search for empty blocks in an ascending manner.
//           Hint: you should maintain a sorted list of all the allocations in the system.
// perhaps we can just insert new blocks in sorted order?
/*
class MallocMetadataList{
    MallocMetadata *head = nullptr;
public:
    MallocMetadataList;
    ~MallocMetadataList;
    ...
};
 */

/* Searches for a free block with up to ‘size’ bytes or allocates (sbrk()) one if none are found.
 * Return value:
 * Success: returns pointer to the first byte in the allocated block (excluding the meta-data)
 * Failure:
 *      - If ‘size’ is 0 returns NULL.
 *      - If ‘size’ is more than 10^8 , return NULL.
 *      - If sbrk fails, return NULL.
 */
void smalloc(size_t size) {
    // check size == 0 || size > 10^8

    // iterate over the MallocMetadata list & try to find allocation of size bytes

    // if not found - use sbrk() for allocation

    // check return values from sbrk() - two options for conversion that I checked (& hopefully they really do work):
    // int conversion = *(int*)res;
    // int conversion = *reinterpret_cast<int*>(res);
    // if (conversion == -1) ...

    // add new allocation at the end of the list - make sure it's sorted.
    // consider corner case where list is empty
    // As I see it, since sbrk() always increases the heap, i.e., it returns higher addresses,
    // and we never decrease the brk ptr or remove elements from the list - it is sorted by default
    // all we need to do is to ass new allocation using sbrk() to the end of the list

    // return something
}

/* Searches for a free block of up to ‘num’ elements, each ‘size’ bytes that are all set to 0
 * or allocates if none are found. In other words, find/allocate size * num bytes and set all bytes to 0.
 * Return value:
 * Success: returns pointer to the first byte in the allocated block
 * Failure:
 *      - If ‘size’ is 0 returns NULL.
 *      - If ‘size’ is more than 10^8 , return NULL.
 *      - If sbrk fails, return NULL.
 * */
void* scalloc(size_t num, size_t size) {
    // calc allocation_size num*size

    // use addr = smalloc for allocation

    // reset bits using memset(addr,0,allocation_size)
    // as indicated here http://www.cplusplus.com/reference/cstring/memset/

    // return addr
}

/*Releases the usage of the block that starts with the pointer ‘p’.
 * If ‘p’ is NULL or already released, simply returns.
 * Presume that all pointers ‘p’ truly points to the beginning of an allocated block.
 * */
void sfree(void* p){
    // calculate somehow block_to_free = &p - sizeof(MallocMetadata)
    // mark bock_to_free->is_free = true;
}

/* If ‘size’ is smaller than the current block’s size, reuses the same block. Otherwise,
 * finds/allocates ‘size’ bytes for a new space, copies content of oldp into the new allocated space and frees the oldp.
 * Return value:
 * Success:
 *      - Returns pointer to the first byte in the (newly) allocated space.
 *      - If ‘oldp’ is NULL, allocates space for ‘size’ bytes and returns a pointer to it.
 * Failure:
 *      - If ‘size’ is 0 returns NULL.
 *      - If ‘size’ is more than 10^8 , return NULL.
 *      - If sbrk fails in allocating the needed space, return NULL.
 *      - Do not free ‘oldp’ if srealloc() fails.
 * */
void* srealloc(void* oldp, size_t size) {
    // if(!oldp) return smalloc(size)

    // calculate address old_block = &oldp - sizeof(MallocMetadata)
    // check if old_block->size >= size. if true, return oldp

    // use addr = smalloc to allocate space - check for success

    // copy data using memcpy(addr, oldp, old_block->size)
    // as indicated here http://www.cplusplus.com/reference/cstring/memcpy/

    // mark old_block->is_free = true

    // return addr
}

/* Returns the number of allocated blocks in the heap that are currently free */
size_t _num_free_blocks() {
    // check if list is null

    // iterate list & count elements which satisfy element->is_free = true
}

/* Returns the number of bytes in all allocated blocks in the heap that are currently free,
excluding the bytes used by the meta-data structs. */
size_t _num_free_bytes() {
    // check if list is null

    // iterate list & count elements * element->size if element->is_free == true - sizeof(metadata)
}

/* Returns the overall (free and used) number of allocated blocks in the heap. */
size_t _num_allocated_blocks() {
    // check if list is null

    // iterate list & count elements
}

/* Returns the overall number (free and used) of allocated bytes in the heap, excluding
the bytes used by the meta-data structs. */
size_t _num_allocated_bytes() {
    // check if list is null

    // iterate list & count elements * element->size
}

/* Returns the number of bytes of a single meta-data structure in your system. */
size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}

/* Returns the overall number of meta-data bytes currently in the heap. */
size_t _num_meta_data_bytes() {
    // check if list is null

    // iterate list & count num_of_elements * metadata->size
    // ... *iterator = ...
    // size_t count = 0;
    // while(iterator) {
    //      count++;
    //      iterator = iterator->next;
    // }
    // return count * _size_meta_data();
}




