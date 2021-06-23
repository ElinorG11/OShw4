//
// Created by student on 6/18/21.
//

#include <cstring>
#include <unistd.h>

struct MallocMetadata { // size of metadata is 25 -> rounded to a power of 2, size is 32 bytes
    size_t size; // 8 bytes
    bool is_free; // 1 byte
    MallocMetadata *next; // 8 bytes
    MallocMetadata *prev; // 8 bytes
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
void* smalloc(size_t size) {
    // check size == 0 || size > 10^8
    if(size == 0 || size > 1e8) {
        return nullptr;
    }

    // iterate over the MallocMetadata list & try to find allocation of size bytes
    MallocMetadata *iterator = head;
    MallocMetadata *iterator_prev = head;
    void* addr;

    while(iterator != nullptr) {
        if(iterator->size >= size && iterator->is_free){
            iterator->is_free = false;
            addr = (void*)((char*)iterator + sizeof(MallocMetadata));
            return addr;
        }
        iterator_prev = iterator;
        iterator = iterator->next;
    }

    // if not found - use sbrk() for allocation
    addr = sbrk(size + sizeof(MallocMetadata));

    // check return values from sbrk(). by sbrk() documentation https://nxmnpg.lemoda.net/2/sbrk should do the following
    // res = sbrk() ...
    // if (res == (void*)-1) ...
    if(addr == (void*)-1) {
        return nullptr;
    }

    // add new allocation at the end of the list - make sure it's sorted.
    // MallocMetadata *metadata = (MallocMetadata*) addr_returned_from_sbrk
    // consider corner case where list is empty
    // As I see it, since sbrk() always increases the heap, i.e., it returns higher addresses,
    // and we never decrease the brk ptr or remove elements from the list - it is sorted by default
    // all we need to do is to ass new allocation using sbrk() to the end of the list

    // since now addr is pointing to the part of the block which also hold the metadata
    // we need to promote addr by sizeof(MallocMetadata) bytes.
    // since addr is void* type, we can use the following conversions for pointer arithmetics:
    // addr = (void*)((char*)addr + sizeof(MallocMetadata));
    // some shit 'bout void* -> char* from here https://stackoverflow.com/questions/19581161/c-change-the-value-a-void-pointer-represents
    // and here https://www.sparknotes.com/cs/pointers/whyusepointers/section1/
    // If a pointer's type is void*, the pointer can point to (almost) any variable.
    // since we want to perform the pointer arithmetic in byte scale - we cast it to char*

    MallocMetadata *new_metadata = (MallocMetadata*)addr;
    new_metadata->size = size;
    new_metadata->is_free = false;
    new_metadata->next = nullptr;

    if(head == nullptr) {
        head = new_metadata;
        new_metadata->prev = nullptr;
    } else {
        new_metadata->prev = iterator_prev;
        iterator_prev->next = new_metadata;
    }

    addr = (void*)((char*)addr + sizeof(MallocMetadata));

    // return something
    return addr;
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
    // check size == 0 || size > 10^8
    if(size == 0 || size > 1e8) {
        return nullptr;
    }

    // calc allocation_size num*size

    // use addr = smalloc for allocation
    void* addr = smalloc(num*size);

    if(addr == nullptr) {
        return nullptr;
    }

    // reset bits using memset(addr,0,allocation_size)
    // as indicated here http://www.cplusplus.com/reference/cstring/memset/
    memset(addr,0,num*size);

    // return addr
    return addr;
}

/* Releases the usage of the block that starts with the pointer ‘p’.
 * If ‘p’ is NULL or already released, simply returns.
 * Presume that all pointers ‘p’ truly points to the beginning of an allocated block.
 * */
void sfree(void* p){
    if(p == nullptr) {
        return;
    }

    // calculate somehow block_to_free = &p - sizeof(MallocMetadata)
    MallocMetadata* metadata = (MallocMetadata*)((char*)p - sizeof(MallocMetadata));

    // by pdf if block is free, simply return
    if(metadata->is_free) return;

    // mark bock_to_free->is_free = true;
    metadata->is_free = true;
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
    // check size == 0 || size > 10^8
    if(size == 0 || size > 1e8) {
        return nullptr;
    }

    // if(!oldp) return smalloc(size)
    void *addr;
    if(oldp == nullptr) {
        addr = smalloc(size);
        return addr;
    }

    // calculate address old_block = &oldp - sizeof(MallocMetadata)
    // check if old_block->size >= size. if true, return oldp
    MallocMetadata* metadata_oldp = (MallocMetadata*)((char*)oldp - sizeof(MallocMetadata));
    size_t size_oldp = metadata_oldp->size;
    if(size_oldp >= size) {
        return oldp;
    }

    // use addr = smalloc to allocate space - check for success
    addr = smalloc(size);
    if(addr == (void*)-1) {
        return nullptr;
    }


    // copy data using memcpy(addr, oldp, old_block->size)
    // as indicated here http://www.cplusplus.com/reference/cstring/memcpy/
    // no need to check validity of result (they didn't ask for it in the pdf and I'm not sure how to do it properly)
    // the check we've used (*(int*)res > 0) causes SIGSEGV :(
    void* res = memcpy(addr, oldp, size_oldp);

    // mark old_block->is_free = true
    sfree(oldp);

    // return addr
    return addr;
}

/* Returns the number of allocated blocks in the heap that are currently free */
size_t _num_free_blocks() {
    size_t counter = 0;
    MallocMetadata *iterator = head;
    // iterate list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        if(iterator->is_free) {
            counter++;
        }
        iterator = iterator->next;
    }

    return counter;
}

/* Returns the number of bytes in all allocated blocks in the heap that are currently free,
excluding the bytes used by the meta-data structs. */
size_t _num_free_bytes() {
    // iterate list & count elements * element->size if element->is_free == true - sizeof(metadata)
    size_t counter = 0;
    MallocMetadata *iterator = head;
    // iterate list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        if(iterator->is_free) {
            counter += iterator->size;
        }
        iterator = iterator->next;
    }

    return counter;
}

/* Returns the overall (free and used) number of allocated blocks in the heap. */
size_t _num_allocated_blocks() {
    // iterate list & count elements
    size_t counter = 0;
    MallocMetadata *iterator = head;
    // iterate list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        counter++;
        iterator = iterator->next;
    }

    return counter;
}

/* Returns the overall number (free and used) of allocated bytes in the heap, excluding
the bytes used by the meta-data structs. */
size_t _num_allocated_bytes() {
    // iterate list & count elements * element->size
    // iterate list & count elements * element->size if element->is_free == true - sizeof(metadata)
    size_t counter = 0;
    MallocMetadata *iterator = head;
    // iterate list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        counter += iterator->size;
        iterator = iterator->next;
    }

    return counter;
}

/* Returns the number of bytes of a single meta-data structure in your system. */
size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}

/* Returns the overall number of meta-data bytes currently in the heap. */
size_t _num_meta_data_bytes() {
    // iterate list & count num_of_elements * metadata->size
    size_t counter = 0;
    MallocMetadata *iterator = head;
    // iterate list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        counter++;
        iterator = iterator->next;
    }

    return (counter * _size_meta_data());
}


