//
// Created by student on 6/18/21.
//

// For Debugging
#include <iostream>

#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

struct MallocMetadata { // size of metadata is 25 -> rounded to a power of 2, size is 32 bytes
    size_t size; // 8 bytes
    bool is_free; // 1 byte
    MallocMetadata *next; // 8 bytes
    MallocMetadata *prev; // 8 bytes
    MallocMetadata *next_free_item; // 8 bytes
    MallocMetadata *prev_free_item; // 8 bytes
};

MallocMetadata *head = nullptr;

// challenge 4

// in the pdf they recommended to use another list for mmap allocated blocks, separate from the list of other allocations
MallocMetadata *head_mmap = nullptr;

// challenge 0

// should use 128-bin (histogram) to maintain free blocks of different size
MallocMetadata *free_blocks_histogram[128];

/*
 * Get histogram index by size
 */
int GetHistogramIndex(size_t size){
    int index = size/1024;
    return index;
}

void AddItemBysize(MallocMetadata *head, MallocMetadata *item, int index){

    // if head is null
    if(head == nullptr){
        head = item;
        item->next_free_item = nullptr;
        item->prev_free_item = nullptr;
        free_blocks_histogram[index] = item;
        return;
    }

    // if item should be first
    if(item->size <= head->size){
        head->prev_free_item = item;
        item->next_free_item = head;
        item->prev_free_item = nullptr;
        free_blocks_histogram[index] = item;
        return;
    }
  // if there is only one item in the list
    if(head->next_free_item == nullptr){
        head->next_free_item = item;
        item->next_free_item = nullptr;
        item->prev_free_item = head;
        return;
    }

    // 2 or more items and the new item is not the smallest
    MallocMetadata* itr = head->next_free_item;
    MallocMetadata* itr_prev = head;

    while(itr != nullptr){
        if(itr_prev->size <= item->size && itr->size >= item->size) {
            itr_prev->next_free_item = item;
            itr->prev_free_item = item;
            item->prev_free_item = itr_prev;
            item->next_free_item = itr;
            return;
        }
        itr_prev = itr;
        itr = itr->next_free_item;
    }
    //item is the largest element
    itr_prev->next_free_item = item;
    item->prev_free_item = itr_prev;
    return;

}


void RemoveFreeItem(MallocMetadata *head, MallocMetadata *item, int index) {
    if(head == nullptr) return;

    // case where item to remove is head
    if(item == head){
        if(head->next_free_item != nullptr){
            item->next_free_item->prev_free_item = nullptr;
        }
        free_blocks_histogram[index] = item->next_free_item;
        item->next_free_item = nullptr;
        item->prev_free_item = nullptr;
        return;
    }
    // case where item is last
    if(item->next_free_item == nullptr){
        item->prev_free_item->next_free_item = nullptr;
        item->prev_free_item = nullptr;
        return;
    }

    item->prev_free_item->next_free_item = item->next_free_item;
    item->next_free_item->prev_free_item = item->prev_free_item;
    item->next_free_item = nullptr;
    item->prev_free_item = nullptr;
    return;

}

// challenge 1

// should we use _split_blocks since it's our assistance func?
void _split_blocks(MallocMetadata *chunk_to_split, size_t allocation_size) {
    // check if after subtracting from the block size the desired allocation size + metadata size
    // we remain with at least 128 byte sized block
    if( chunk_to_split->size - sizeof(MallocMetadata) - allocation_size < 128){
        return;
    }

    // create new metadata element and make it point to the space after the pointer we got from smalloc by allocation
    // size bytes
    MallocMetadata *new_metadata = (MallocMetadata*)((char*)((void *)(chunk_to_split)) + sizeof(MallocMetadata) + allocation_size);
    // update all necessary field such as size, prev & next for the new element
    new_metadata->size = chunk_to_split->size - sizeof(MallocMetadata) - allocation_size;
    new_metadata->next = chunk_to_split->next;
    new_metadata->prev = chunk_to_split;
    // update all necessary field such as size, prev & next for the actual allocation
    int index = GetHistogramIndex(new_metadata->size);
    AddItemBysize(free_blocks_histogram[index],new_metadata, index);
    chunk_to_split->next = new_metadata;
    chunk_to_split->size = allocation_size;
}

// challenge 2

// should we use _combine_blocks since it's our assistance func?
void _combine_blocks(MallocMetadata *block_to_combine) {
    int index;
    //check if block is head and the only block
    if(block_to_combine->prev == nullptr){
        if(block_to_combine->next == nullptr) {
            index = GetHistogramIndex(block_to_combine->size);
            AddItemBysize(free_blocks_histogram[index], block_to_combine, index);
            return;
        }
    // check if block is head and not the only block
        if(block_to_combine->next->is_free){
            index = GetHistogramIndex(block_to_combine->next->size);
            RemoveFreeItem(free_blocks_histogram[index],block_to_combine->next,index);
            block_to_combine->size += block_to_combine->next->size + sizeof(MallocMetadata);
            block_to_combine->next->next->prev = block_to_combine;
            block_to_combine->next = block_to_combine->next->next;
            index = GetHistogramIndex(block_to_combine->size);
            AddItemBysize(free_blocks_histogram[index], block_to_combine, index);
        }
        index = GetHistogramIndex(block_to_combine->size);
        AddItemBysize(free_blocks_histogram[index], block_to_combine, index);
        return;
    }
    // case block is last
    if(block_to_combine->next == nullptr){
        if(block_to_combine->prev->is_free){
            index = GetHistogramIndex(block_to_combine->prev->size);
            RemoveFreeItem(free_blocks_histogram[index],block_to_combine->prev,index);
            block_to_combine->prev->size += block_to_combine->size + sizeof(MallocMetadata);
            block_to_combine->prev->next = block_to_combine->next;
            index = GetHistogramIndex(block_to_combine->prev->size);
            AddItemBysize(free_blocks_histogram[index], block_to_combine->prev, index);
        }
        index = GetHistogramIndex(block_to_combine->size);
        AddItemBysize(free_blocks_histogram[index], block_to_combine, index);
        return;

    }

    //case where both prev and next are free
    if(block_to_combine->prev->is_free && block_to_combine->next->is_free){
        int index = GetHistogramIndex(block_to_combine->prev->size);
        //remove adjacent free blocks from free list
        RemoveFreeItem(free_blocks_histogram[index],block_to_combine->prev,index);
        index = GetHistogramIndex(block_to_combine->next->size);
        RemoveFreeItem(free_blocks_histogram[index],block_to_combine->next,index);
        //combine the three blocks
        block_to_combine->prev->size += block_to_combine->size + block_to_combine->next->size + 2*(sizeof(MallocMetadata));
        block_to_combine->prev->next = block_to_combine->next->next;
        block_to_combine->next->next->prev = block_to_combine->prev;

        //add to free blocks list
        index = GetHistogramIndex(block_to_combine->prev->size);
        AddItemBysize(free_blocks_histogram[index], block_to_combine->prev, index);
        return;

    } else if(block_to_combine->prev->is_free){
        int index = GetHistogramIndex(block_to_combine->prev->size);
        RemoveFreeItem(free_blocks_histogram[index],block_to_combine->prev,index);
        block_to_combine->prev->size += block_to_combine->size + sizeof(MallocMetadata);
        block_to_combine->prev->next = block_to_combine->next;
        index = GetHistogramIndex(block_to_combine->prev->size);
        AddItemBysize(free_blocks_histogram[index], block_to_combine->prev, index);
        return;

    } else if(block_to_combine->next->is_free){
        int index = GetHistogramIndex(block_to_combine->next->size);
        RemoveFreeItem(free_blocks_histogram[index],block_to_combine->next,index);
        block_to_combine->size += block_to_combine->next->size + sizeof(MallocMetadata);
        block_to_combine->next->next->prev = block_to_combine;
        block_to_combine->next = block_to_combine->next->next;
        index = GetHistogramIndex(block_to_combine->size);
        AddItemBysize(free_blocks_histogram[index], block_to_combine, index);
        return;

    }
    index = GetHistogramIndex(block_to_combine->size);
    AddItemBysize(free_blocks_histogram[index], block_to_combine, index);
    return;
}


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

    void* addr;

    // challenge 4

    // check if size fits mmap
    // if so, iterate to the end of the mmap list (I guess it should be sorted as well)
    // now we can call mmap with the following parameters:

    // about mmap from man pages https://linux.die.net/man/3/mmap
    // mmap(addr, len, prot, flags, fildes, off);

    // addr: nullptr (from tutorial 10 slide 21)
    // All implementations interpret an addr value of 0 as granting the implementation complete freedom in selecting pa,
    // subject to constraints described below

    // len = allocation size + metadata size

    //  prot (?): PROT_WRITE, PROT_READ, PROT_EXEC

    // flags (?): MAP_PRIVATE

    // fildes: -1 (tutorial)

    // off: 0 (tutorial)

    // check addr validity
    if(size >= 128*1024){
        MallocMetadata *iterator = head_mmap;
        MallocMetadata *iterator_prev = iterator;

        while(iterator != nullptr) {
            // we just need to get to the end of the list (no free block which were allocated by mmap since munmap()
            // removes the virtual memory area
            iterator_prev = iterator;
            iterator = iterator->next_free_item;
        }

        //void* addr = mmap(NULL, size + sizeof(MallocMetadata), prot, flags, fildes, 0);
        void* addr = mmap(NULL, size + sizeof(MallocMetadata), PROT_WRITE | PROT_READ | PROT_EXEC , MAP_PRIVATE, 0, 0);
        if(addr == (void*)-1) {
            return nullptr;
        }

        MallocMetadata *new_metadata = (MallocMetadata*)addr;
        new_metadata->size = size;
        new_metadata->is_free = false;
        new_metadata->next = nullptr;
        new_metadata->next_free_item = nullptr;
        new_metadata->prev_free_item = nullptr;

        if(head_mmap == nullptr) {
            head_mmap = new_metadata;
            new_metadata->prev = nullptr;
        } else {
            new_metadata->prev = iterator_prev;
            iterator_prev->next = new_metadata;
        }

        addr = (void*)((char*)addr + sizeof(MallocMetadata));
        return addr;
    }

    // iterate over the MallocMetadata list & try to find allocation of size bytes
    int index = GetHistogramIndex(size);
    MallocMetadata *iterator = free_blocks_histogram[index];
    MallocMetadata *iterator_prev = iterator;

    while(iterator != nullptr) {
        if(iterator->size >= size && iterator->is_free){
            iterator->is_free = false;
            addr = (void*)(iterator + sizeof(MallocMetadata));

            // challenge 0

            // calculate bin index. I think it goes something like size / 1024

            // iterate all blocks in this list of current bin and continue to the following bins if required

            // for(i = bin_index; i <= 128; i++)
            //      MallocMetadata *iterator = histogram[bin_index]
            //      while(iterator) search for element
            //      remove from doubly linked list (?) - I'm not sure what they mean by 'maintain valid histogram at any point'
            //      - either we should completely remove the element
            //      - or we should just mark it as used
            // return the addr we found
            RemoveFreeItem(free_blocks_histogram[index],iterator,index);

            // challenge 1

            // so instead of just removing from the doubly linked list we will try to call split_block?
            // and then we should add the allocated part to the allocation list and to keep the free part in the free block hist?
            _split_blocks(iterator,size);
            return addr;
        }
        iterator_prev = iterator;
        iterator = iterator->next_free_item;
    }

    // challenge 3

    // if we reached here, means we didn't find free, big enough, chunk of free memory in our hist.
    // we know that the "Wilderness" chunk is the topmost block. i.e., it has the highest (?) address
    // means we need to iterate all allocated blocks list to find the last free block (will have the biggest addr)
    // basically I think it's iterator_prev if I'm not mistaken.
    // now we hold the "Wilderness" chunk and we can check how much we need to add to it in order to fulfill the
    // required allocation.
    // now, we can call sbrk with the effective size and just update the size field in the metadata of the "Wilderness" block

    if(iterator_prev != nullptr && iterator_prev->is_free) {
        size_t extension_size = size - iterator_prev->size;

        addr = sbrk(extension_size);

        // check return values from sbrk(). by sbrk() documentation https://nxmnpg.lemoda.net/2/sbrk should do the following
        // res = sbrk() ...
        // if (res == (void*)-1) ...
        if(addr == (void*)-1) {
            return nullptr;
        }
        iterator_prev->size += extension_size;

    } else {
        // if not found - use sbrk() for allocation
        addr = sbrk(size + sizeof(MallocMetadata));

        if(addr == (void*)-1) {
            return nullptr;
        }

        // add new allocation at the end of the list - make sure it's sorted.
        // MallocMetadata *metadata = (MallocMetadata*) addr_returned_from_sbrk
        // consider corner case where list is empty
        // As I see it, since sbrk() always increases the heap, i.e., it returns higher addresses,
        // and we never decrease the brk ptr or remove elements from the list - it is sorted by default
        // all we need to do is to ass new allocation using sbrk() to the end of the list

        MallocMetadata *new_metadata = (MallocMetadata*)addr;
        new_metadata->size = size;
        new_metadata->is_free = false;
        new_metadata->next = nullptr;
        new_metadata->next_free_item = nullptr;
        new_metadata->prev_free_item = nullptr;

        iterator = head;
        iterator_prev = iterator;
        while(iterator != nullptr) {
            iterator_prev = iterator;
            iterator = iterator->next;
        }

        if(head == nullptr) {
            head = new_metadata;
            new_metadata->prev = nullptr;
        } else {
            new_metadata->prev = iterator_prev;
            iterator_prev->next = new_metadata;
        }

        // since now addr is pointing to the part of the block which also hold the metadata
        // we need to promote addr by sizeof(MallocMetadata) bytes.
        // since addr is void* type, we can use the following conversions for pointer arithmetics:
        // addr = (void*)((char*)addr + sizeof(MallocMetadata));
        // some shit 'bout void* -> char* from here https://stackoverflow.com/questions/19581161/c-change-the-value-a-void-pointer-represents
        // and here https://www.sparknotes.com/cs/pointers/whyusepointers/section1/
        // If a pointer's type is void*, the pointer can point to (almost) any variable.
        // since we want to perform the pointer arithmetic in byte scale - we cast it to char*

        addr = (void*)((char*)addr + sizeof(MallocMetadata));
    }

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
    // calculate metadata
    MallocMetadata* metadata = (MallocMetadata*)((char*)p - sizeof(MallocMetadata));
    // mark chunk as free
    metadata->is_free = true;

    // challenge 4
    // check if size fits mmap
    if(metadata->size > 128 * 1024) {
        // now call munmap
        munmap(metadata, metadata->size + sizeof(MallocMetadata));
        return;
    }

    // challenge 3
    // combine adds item to free block list
    _combine_blocks(metadata);
    return;
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


    // perhaps we should change that to sfree(oldp) and then sfree will handle all the updates of the free block hist?
    // mark old_block->is_free = true
    sfree(oldp);

    // return addr
    return addr;
}

/* Returns the number of allocated blocks in the heap that are currently free */
size_t _num_free_blocks() {
    size_t counter = 0;
    MallocMetadata *iterator = head;

    // not sure. I think that they want us to maintain 2 lists (free & used) and they should be disjoint
    // so here we should iterate only the free_blocks_hist. else, I'm not sure what to do, we are in danger of counting
    // same free block twice...

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

    // not sure. I think that they want us to maintain 2 lists (free & used) and they should be disjoint
    // so here we should iterate only the free_blocks_hist. else, I'm not sure what to do, we are in danger of counting
    // same free block twice...

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

    // I think we can just iterate free_blocks_hist & the allocation list pointed by head?

    // iterate list & count elements
    size_t counter = 0;
    MallocMetadata *iterator = head;
    // iterate srbk allocation list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        counter++;
        iterator = iterator->next;
    }

    iterator = head_mmap;
    // iterate mmap allocation list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        counter++;
        iterator = iterator->next;
    }

    return counter;
}

/* Returns the overall number (free and used) of allocated bytes in the heap, excluding
the bytes used by the meta-data structs. */
size_t _num_allocated_bytes() {

    // I think we can just iterate free_blocks_hist & the allocation list pointed by head?

    // iterate list & count elements * element->size
    // iterate list & count elements * element->size if element->is_free == true - sizeof(metadata)
    size_t counter = 0;
    MallocMetadata *iterator = head;
    // iterate srbk() list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        counter += iterator->size;
        iterator = iterator->next;
    }

    iterator = head_mmap;
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


    // I think we can just iterate sbrk & mmap allocation lists pointed by head & head_mmap?

    // iterate list & count num_of_elements * metadata->size
    size_t counter = 0;
    MallocMetadata *iterator = head;
    // iterate list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        counter += iterator->size;
        iterator = iterator->next;
    }

    iterator = head_mmap;
    // iterate list & count elements which satisfy element->is_free = true
    while (iterator != nullptr) {
        counter += iterator->size;
        iterator = iterator->next;
    }

    return (counter * _size_meta_data());
}



int main() {
    // find maximal allocation for sbrk() to fail
    // struct rlimit rlp;
    // getrlimit(RLIMIT_DATA,&rlp);
    // std::cout << rlp.rlim_max << std::endl;

    // Test Errors

    // zero size allocation
    void* ptr;
    int size_met = 0x20; // sizeof MallocMetadata
    /*
    ptr = smalloc(0);
    if(ptr == nullptr) {
        std::cout << "SUCCESS: Test 0 size allocation failed" << std::endl;
    } else {
        std::cout << "FAIL: Test 0 size allocation succeeded" << std::endl;
    }

    // big allocation
    ptr = smalloc(1e8 + 1);
    if(ptr == nullptr) {
        std::cout << "SUCCESS: Test 1e8 size allocation failed" << std::endl;
    } else {
        std::cout << "FAIL: Test 1e8 size allocation succeeded" << std::endl;
    }

    // put size > 1e8 check in comments & try to make sbrk() fail
    ptr = smalloc(18446744073709551615 + 1);
    if(ptr == nullptr) {
        std::cout << "SUCCESS: Test sbrk fail" << std::endl;
    } else {
        std::cout << "FAIL: Test sbrk fail" << std::endl;
    }


    // Test concurrent allocations & check their size
    ptr = smalloc(20);
    void* new_ptr = (void*)((char*)head + size_met);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 20. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 20. head: " << head << " new ptr: " << ptr << std::endl;
    }

    std::cout << "num of allocated blocks #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;


    ptr = smalloc(50);
    std::cout <<  _num_allocated_blocks() << std::endl;
    new_ptr = (void*)((char*)head + _num_allocated_bytes() - 0x32 + _num_allocated_blocks()*size_met);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 50. new_ptr: " << new_ptr << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 50. new_ptr: " << new_ptr << " new ptr: " << ptr << std::endl;
    }

    ptr = smalloc(1e3);
    new_ptr = (void*)((char*)new_ptr + 0x32 + 0x20);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 1e3. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 1e3. head: " << head << " new ptr: " << ptr << std::endl;
    }

    ptr = smalloc(2*1e5);
    new_ptr = (void*)((char*)new_ptr + 1000 + 0x20);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 2*1e5. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 2*1e5. head: " << head << " new ptr: " << ptr << std::endl;
    }

    ptr = smalloc(769);
    new_ptr = (void*)((char*)new_ptr + 200000 + 0x20);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 769. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 769. head: " << head << " new ptr: " << ptr << std::endl;
    }

    ptr = smalloc(1);
    new_ptr = (void*)((char*)new_ptr + 769 + 0x20);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 1. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 1. head: " << head << " new ptr: " << ptr << std::endl;
    }

    ptr = smalloc(3);
    new_ptr = (void*)((char*)new_ptr + 1 + 0x20);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 3. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 3. head: " << head << " new ptr: " << ptr << std::endl;
    }

    */

    // Test sfree
    /*
    std::cout << "Test sfree" << std::endl;
    int num_of_allocations=0, allocated_bytes=0, num_of_frees=0, free_bytes=0;

    ptr = smalloc(70);
    void* new_ptr = (void*)((char*)head + size_met);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 70. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 70. head: " << head << " new ptr: " << ptr << std::endl;
    }

    num_of_allocations++;
    allocated_bytes += 70;

    if(_num_allocated_blocks() == num_of_allocations && _num_allocated_bytes() == allocated_bytes) {
        std::cout << "SUCCESS: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    }

    if(_num_free_blocks() == num_of_frees && _num_free_bytes() == free_bytes) {
        std::cout << "SUCCESS: #" << num_of_frees << " frees of size " << free_bytes << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_frees << " frees of " << free_bytes << "bytes" << std::endl;
    }

    sfree(ptr);
    // check that the address of ptr stayed the same
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: ptr address: " << ptr << " didn't change" << std::endl;
    } else {
        std::cout << "FAIL: ptr address: " << ptr << " changed" << std::endl;
    }

    num_of_frees++;
    free_bytes += 70;

    if(_num_allocated_blocks() == num_of_allocations && _num_allocated_bytes() == allocated_bytes) {
        std::cout << "SUCCESS: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    }

    if(_num_free_blocks() == num_of_frees && _num_free_bytes() == free_bytes) {
        std::cout << "SUCCESS: #" << num_of_frees << " frees of size " << free_bytes << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_frees << " frees of " << free_bytes << "bytes" << std::endl;
    }

    // make sure new allocation has the same address as ptr
    void *new_allocation = smalloc(70);
    if(new_ptr == new_allocation && ptr == new_allocation) {
        std::cout << "SUCCESS: new element was allocated at the same address" << std::endl;
    } else {
        std::cout << "FAIL: new element wasn't allocated at the same address: old addr: " << new_ptr << " new addr: " << new_allocation << std::endl;
    }

    if(_num_allocated_blocks() == num_of_allocations && _num_allocated_bytes() == allocated_bytes) {
        std::cout << "SUCCESS: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    }

    if(_num_free_blocks() == 0 && _num_free_bytes() == 0) {
        std::cout << "SUCCESS: #" << num_of_frees << " frees of size " << free_bytes << std::endl;
    } else {
        std::cout << "FAIL: #" << _num_free_blocks() << " frees of " << _num_free_bytes() << " bytes" << std::endl;
    }

    sfree(new_allocation);
    if(new_ptr == new_allocation && ptr == new_allocation) {
        std::cout << "SUCCESS: new element was allocated at the same address" << std::endl;
    } else {
        std::cout << "FAIL: new element wasn't allocated at the same address" << std::endl;
    }

    if(_num_allocated_blocks() == num_of_allocations && _num_allocated_bytes() == allocated_bytes) {
        std::cout << "SUCCESS: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    }

    if(_num_free_blocks() == num_of_frees && _num_free_bytes() == free_bytes) {
        std::cout << "SUCCESS: #" << num_of_frees << " frees of size " << free_bytes << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_frees << " frees of " << free_bytes << "bytes" << std::endl;
    }

    // try double free
    sfree(new_allocation);

    // shouldn't increase the number of frees sine we've allocated space we've already used before

    if(_num_allocated_blocks() == num_of_allocations && _num_allocated_bytes() == allocated_bytes) {
        std::cout << "SUCCESS: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    }

    if(_num_free_blocks() == num_of_frees && _num_free_bytes() == free_bytes) {
        std::cout << "SUCCESS: #" << num_of_frees << " frees of size " << free_bytes << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_frees << " frees of " << free_bytes << "bytes" << std::endl;
    }

    // new allocation
    ptr = smalloc(500);
    new_ptr = (void*)((char*)head + _num_allocated_bytes() - 0x1F4 + _num_allocated_blocks()*size_met);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 500. head: " << head << " ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 500. new_ptr: " << new_ptr << " ptr: " << ptr << std::endl;
    }

    num_of_allocations++;
    allocated_bytes += 500;

    if(_num_allocated_blocks() == num_of_allocations && _num_allocated_bytes() == allocated_bytes) {
        std::cout << "SUCCESS: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    }

    if(_num_free_blocks() == num_of_frees && _num_free_bytes() == free_bytes) {
        std::cout << "SUCCESS: #" << num_of_frees << " frees of size " << free_bytes << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_frees << " frees of " << free_bytes << "bytes" << std::endl;
    }


    sfree(ptr);

    num_of_frees++;
    free_bytes += 500;

    if(_num_allocated_blocks() == num_of_allocations && _num_allocated_bytes() == allocated_bytes) {
        std::cout << "SUCCESS: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    }

    if(_num_free_blocks() == num_of_frees && _num_free_bytes() == free_bytes) {
        std::cout << "SUCCESS: #" << num_of_frees << " frees of size " << free_bytes << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_frees << " frees of " << free_bytes << "bytes" << std::endl;
    }

    // big allocation
    void* big_alloc_ptr = smalloc(7e5);
    new_ptr = (void*)((char*)head + _num_allocated_bytes() - 0xAAE60 + _num_allocated_blocks()*size_met);
    if(new_ptr == big_alloc_ptr) {
        std::cout << "SUCCESS: Test allocation size 7e5. head: " << head << " ptr: " << big_alloc_ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 7e5. new_ptr: " << new_ptr << " ptr: " << big_alloc_ptr << std::endl;
    }

    num_of_allocations++;
    allocated_bytes += 7e5;

    if(_num_allocated_blocks() == num_of_allocations && _num_allocated_bytes() == allocated_bytes) {
        std::cout << "SUCCESS: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_allocations << " allocations of " << allocated_bytes << " bytes" << std::endl;
    }

    if(_num_free_blocks() == num_of_frees && _num_free_bytes() == free_bytes) {
        std::cout << "SUCCESS: #" << num_of_frees << " frees of size " << free_bytes << std::endl;
    } else {
        std::cout << "FAIL: #" << num_of_frees << " frees of " << free_bytes << "bytes" << std::endl;
    }

    // small allocation
    void* small_alloc_ptr = smalloc(1);
    new_ptr = (void*)((char*)head + size_met);
    if(new_ptr == small_alloc_ptr) {
        std::cout << "SUCCESS: Test allocation size 1. head: " << head << " ptr: " << small_alloc_ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 1. new_ptr: " << new_ptr << " ptr: " << small_alloc_ptr << std::endl;
    }

    sfree(big_alloc_ptr);

    sfree(small_alloc_ptr);

    std::cout << "#" << _num_allocated_blocks() << " allocations of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "#" << _num_free_blocks() << " frees of " << _num_free_bytes() << " bytes" << std::endl;

    */

    // Test scalloc
    /*
    std::cout << "\n" << "Test scalloc" << std::endl;
    ptr = scalloc(3,70);
    void* new_ptr = (void*)((char*)head + size_met);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 3*70. head: " << head << " new ptr: " << ptr << " sample data: " << *(int*)(((char*)ptr+200)) << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 3*70. head: " << head << " new ptr: " << ptr << std::endl;
    }

    std::cout << "Test 1: scalloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 1: scalloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    sfree(ptr);

    std::cout << "Test 1: sfree: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 1: sfree: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    ptr = scalloc(5,50);
    new_ptr = (void*)((char*)head + _num_allocated_bytes() - 5*0x32 + _num_allocated_blocks()*size_met);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 50. head: " << head << " new ptr: " << ptr << " sample data: " << *(int*)(((char*)ptr+900)) << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 50. head: " << head << " new ptr: " << ptr << std::endl;
    }

    std::cout << "Test 2: scalloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 2: scalloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    ptr = scalloc(1,1e6);
    new_ptr = (void*)((char*)head + _num_allocated_bytes() - 0xF4240 + _num_allocated_blocks()*size_met);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 1e6. head: " << head << " new ptr: " << ptr << " sample data: " << *(int*)(((char*)ptr+1000)) << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 1e6. head: " << head << " new ptr: " << ptr << std::endl;
    }

    std::cout << "Test 3: scalloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 3: scalloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    ptr = scalloc(2,1e3);
    new_ptr = (void*)((char*)head + _num_allocated_bytes() - 2*0x3E8 + _num_allocated_blocks()*size_met);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 1e3. head: " << head << " new ptr: " << ptr << " sample data: " << *(int*)(((char*)ptr+400)) << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 1e3. head: " << head << " new ptr: " << ptr << std::endl;
    }

    std::cout << "Test 3: scalloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 4: scalloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;
    */

    // Test srealloc
    /*
    std::cout << "\n" << "Test srealloc" << std::endl;
    ptr = srealloc(nullptr,20);
    std::cout << "Test nullptr to srealloc returned the address: " << ptr << ". head is: " << head << std::endl;

    std::cout << "Test 1: srealloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 1: srealloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    ptr = smalloc(70);
    memset(ptr,5,10);

    // should be the same addr since new size < old size
    std::cout << "previous addr of ptr: " << ptr << std::endl;
    ptr = srealloc(ptr,20);
    std::cout << "Test re-allocation #0. new addr (same): " << ptr <<  " sample data: " << *(int*)ptr << std::endl;

    std::cout << "Test 1: scalloc: allocated blocks(2): #" << _num_allocated_blocks() << " of(90) " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 1: scalloc: free blocks(0): #" << _num_free_blocks() << " of(0) " << _num_free_bytes() << " bytes" << std::endl;

    sfree(ptr);

    std::cout << "Test 1.1: sfree: allocated blocks(2): #" << _num_allocated_blocks() << " of(90) " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 1.1: sfree: free blocks(1): #" << _num_free_blocks() << " of(70) " << _num_free_bytes() << " bytes" << std::endl;


    void* ptr1 = smalloc(110);
    void* ptr2 = smalloc(279);
    void* ptr3 = smalloc(1e4);
    void* ptr4 = smalloc(2);
    void* ptr5 = smalloc(5e7);

    memset(ptr1,5,10);
    memset(ptr2,5,10);
    memset(ptr3,5,1);
    memset(ptr4,5,1);
    memset(ptr5,5,1);

    std::cout << "previous addr of ptr1: " << ptr1 << std::endl;
    ptr1 = srealloc(ptr1,20);
    std::cout << "Test re-allocation #1. new addr: " << ptr1 <<  " sample data: " << *(int*)ptr1 << std::endl;

    std::cout << "Test 1: srealloc: allocated blocks(6): #" << _num_allocated_blocks() << " of(50010479) " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 1: srealloc: free blocks(0): #" << _num_free_blocks() << " of(0) " << _num_free_bytes() << " bytes" << std::endl;

    std::cout << "previous addr of ptr2: " << ptr2 << std::endl;
    ptr2 = srealloc(ptr2,20);
    std::cout << "Test re-allocation #2. new addr: " << ptr2 <<  " sample data: " << *(int*)ptr2 << std::endl;

    std::cout << "Test 2: srealloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 2: srealloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    std::cout << "previous addr of ptr3: " << ptr3 << std::endl;
    ptr3 = srealloc(ptr3,90);
    std::cout << "Test re-allocation #3. new addr: " << ptr3 <<  " sample data: " << *(int*)ptr3 << std::endl;

    std::cout << "Test 3: srealloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 3: srealloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;


    std::cout << "previous addr of ptr4: " << ptr4 << std::endl;
    ptr4 = srealloc(ptr4,1e6);
    std::cout << "Test re-allocation #4. new addr: " << ptr4 <<  " sample data: " << *(int*)ptr4 << std::endl;

    std::cout << "Test 4: srealloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 4: srealloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    sfree(ptr1);

    std::cout << "Test 4.1: srealloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 4.1: srealloc: free blocks(2): #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    sfree(ptr3);

    std::cout << "Test 4.2: srealloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 4.2: srealloc: free blocks(3): #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;

    std::cout << "previous addr of ptr5: " << ptr5 << std::endl;
    ptr5 = srealloc(ptr5,800);
    std::cout << "Test re-allocation #5. new addr: " << ptr5 <<  " sample data: " << *(int*)ptr5 << std::endl;

    std::cout << "Test 5: srealloc: allocated blocks: #" << _num_allocated_blocks() << " of " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "Test 5: srealloc: free blocks: #" << _num_free_blocks() << " of " << _num_free_bytes() << " bytes" << std::endl;
    */


    // Test challenge 0
    /*
    std::cout << "challenge 0 debug" << std::endl;

    ptr = smalloc(70);

    std::cout << "challenge 0: Test 1: allocated blocks(1): #" << _num_allocated_blocks() << " of(70) " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "challenge 0: Test 1: free blocks(0): #" << _num_free_blocks() << " of(0) " << _num_free_bytes() << " bytes" << std::endl;


    // double free
    sfree(ptr);

    std::cout << "challenge 0: Test 1.1: allocated blocks(1): #" << _num_allocated_blocks() << " of(70) " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "challenge 0: Test 1.1: free blocks(1): #" << _num_free_blocks() << " of(70) " << _num_free_bytes() << " bytes" << std::endl;


    sfree(ptr);

    std::cout << "challenge 0: Test 1.2: allocated blocks(1): #" << _num_allocated_blocks() << " of(70) " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "challenge 0: Test 1.2: free blocks(1): #" << _num_free_blocks() << " of(70) " << _num_free_bytes() << " bytes" << std::endl;


    // allocate at the same address
    ptr = smalloc(30);

    std::cout << "challenge 0: Test 2: allocated blocks(1): #" << _num_allocated_blocks() << " of(70) " << _num_allocated_bytes() << " bytes" << std::endl;
    std::cout << "challenge 0: Test 2: free blocks(1): #" << _num_free_blocks() << " of(70) " << _num_free_bytes() << " bytes" << std::endl;


    // print free list & make sure it is sorted

    // allocate TONS
    void* ptr1 = smalloc(100);
    void* ptr2 = smalloc(3000);
    void* ptr3 = smalloc(4000);
    void* ptr4 = smalloc(3500);
    void* ptr5 = smalloc(1e3);
    void* ptr6 = smalloc(1500);
    void* ptr7 = smalloc(2240);
    void* ptr8 = smalloc(7789);
    void* ptr9 = smalloc(1e5);
    void* ptr10 = smalloc(14589);
    void* ptr11 = smalloc(145678);
    void* ptr12 = smalloc(876390);
    void* ptr13 = smalloc(127);
    void* ptr14 = smalloc(20);
    void* ptr15 = smalloc(5);
    void* ptr16 = smalloc(1347990);
    void* ptr17 = smalloc(15);


    sfree(ptr1);
    sfree(ptr2);
    sfree(ptr3);
    sfree(ptr4);
    sfree(ptr5);
    sfree(ptr6);
    sfree(ptr7);
    sfree(ptr8);
    sfree(ptr9);
    sfree(ptr10);
    sfree(ptr11);
    sfree(ptr12);
    sfree(ptr13);
    sfree(ptr14);
    sfree(ptr15);
    sfree(ptr16);
    sfree(ptr17);




    // iterate many

    MallocMetadata *iterator = free_blocks_histogram[0];
    for (int i = 0; i < 128; ++i) {
        while (iterator != nullptr) {
            std::cout << "bin #" << i<< " element size: " << iterator->size << std::endl;
            iterator = iterator->next;
        }

    }

    // much WOW

    // over 9000!!!!!!!!!!!!!!!!!!!1
    */

    // Test challenge 1
    void* ptr1 = smalloc(1000);
    void* ptr2 = smalloc(1000);
    void* ptr3 = smalloc(1000);
    void* ptr4 = smalloc(1000);
    void* ptr5 = smalloc(1000);
    void* ptr6 = smalloc(1000);
    void* ptr7 = smalloc(1000);
    void* ptr8 = smalloc(1000);

    std::cout << "addr ptr2 before call to sfree: " << ptr2 << std::endl;
    sfree(ptr2);

    void* ptr9 = smalloc(20);

    // print addr
    std::cout << "addr ptr9 after call to sfree: " << ptr9 << " should be " << ptr2 << std::endl;

    // print metadata
    std::cout << "Test 1: allocated blocks(9) #" << _num_allocated_blocks() << " of(8000) " << _num_allocated_bytes() << std::endl;
    std::cout << "Test 1: free blocks(1) #" << _num_free_blocks << " of(800) " << _num_free_bytes() << std::endl;


    void* ptr10 = smalloc(20);

    void* ptr11 = smalloc(20);

    void* ptr12 = smalloc(800); // make sure next block we need to split is the one ptr5 was allocated at

    sfree(ptr5);

    void* ptr13 = smalloc(20);

    void* ptr14 = smalloc(20);

    void* ptr15 = smalloc(20);

    void* ptr16 = smalloc(800);

    sfree(ptr8);





    // Test challenge 2

    // Test challenge 3

    // Test challenge 4

    return 0;
}
