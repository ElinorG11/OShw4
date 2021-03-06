//
// Created by student on 6/18/21.
//

#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

struct MallocMetadata { // size of metadata is 57 -> rounded to a power of 2, size is 64 bytes
    size_t size; // 8 bytes
    bool is_free; // 1 byte
    MallocMetadata *next; // 8 bytes
    MallocMetadata *prev; // 8 bytes
    MallocMetadata *next_mmap; // 8 bytes
    MallocMetadata *prev_mmap; // 8 bytes
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

// challenge 5

/******************************IMPORTANT NOTE******************************/
/* don't forget to add call for aligned_size in smalloc
 * I'm not sure we need to add this support for mmap() as well since it's not continuous region of memory
 * but a seperate area... check on Piazza.
 */
/*************************************************************************/
size_t _align_size(size_t size){
    // we want to make sure the size INCLUDING the meatadata is aligned
    // metadata size is 8 + 1 + 8 + 8 = 25
    size += sizeof(MallocMetadata);

    // check if size + metadata has reminder after division by 8
    // addr should have multiplication of 8 alignment
    size_t mod = size % 8;

    // get effective size (we will return to the user bit more than he requested for alignment)
    size -= sizeof(MallocMetadata);

    if (mod)
        return size + (8 - mod);
    else
        return size;
}

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
    if(static_cast<int>(chunk_to_split->size) - static_cast<int>(sizeof(MallocMetadata)) - static_cast<int>(allocation_size) < 128){
        return;
    }

    // create new metadata element and make it point to the space after the pointer we got from smalloc by allocation
    // size bytes
    MallocMetadata *new_metadata = (MallocMetadata*)((char*)((void *)(chunk_to_split)) + sizeof(MallocMetadata) + allocation_size);
    // update all necessary field such as size, prev & next for the new element
    new_metadata->size = chunk_to_split->size - sizeof(MallocMetadata) - allocation_size;
    new_metadata->next = chunk_to_split->next;
    new_metadata->prev = chunk_to_split;
    new_metadata->is_free = true;
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
            if(block_to_combine->next->next) {
                block_to_combine->next->next->prev = block_to_combine;
            }
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
        if(block_to_combine->next->next) {
            block_to_combine->next->next->prev = block_to_combine->prev;
        }

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
        if(block_to_combine->next->next) {
            block_to_combine->next->next->prev = block_to_combine;
        }
        block_to_combine->next = block_to_combine->next->next;
        index = GetHistogramIndex(block_to_combine->size);
        AddItemBysize(free_blocks_histogram[index], block_to_combine, index);
        return;

    }
    index = GetHistogramIndex(block_to_combine->size);
    AddItemBysize(free_blocks_histogram[index], block_to_combine, index);
    return;
}

/* adds item to head_mmap list. we assume the following fields are updated:
 * item_to_add->size, item_to_add->is_free, item_to_add->next_free_item, item_to_add->prev_free_item,
 * item_to_add->next, item_to_add->prev
 * should update only: next_mmap, prev_mmap fields
 * */
void _add_to_mmap(MallocMetadata *item_to_add) {
    // case head is null
    if(head_mmap == nullptr) {
        item_to_add->prev_mmap = nullptr;
        head_mmap = item_to_add;
        return;
    }

    // case item should be added last
    MallocMetadata *iterator = head_mmap;
    MallocMetadata *iterator_prev = head_mmap;
    while (iterator != nullptr) {
        iterator_prev = iterator;
        iterator = iterator->next_mmap;
    }

    iterator_prev->next_mmap = item_to_add;
    item_to_add->prev_mmap = iterator_prev;
    return;
}

/* removes item from head_mmap list. we assume the following fields are updated:
 * item_to_add->size, item_to_add->is_free, item_to_add->next_free_item, item_to_add->prev_free_item,
 * item_to_add->next, item_to_add->prev
 * should update only: next_mmap, prev_mmap fields
 * */
void _remove_from_mmap(MallocMetadata *item_to_remove) {
    // case it's the first - don't forget to update head
    if(item_to_remove == head_mmap) {
        // case there's only one - don't forget to update head
        if(head_mmap->next_mmap == nullptr) {
            head_mmap = nullptr;
            item_to_remove->next_mmap = nullptr;
            return;
        }
        head_mmap = item_to_remove->next_mmap;
        item_to_remove->next_mmap->prev_mmap = nullptr;
        item_to_remove->next_mmap = nullptr;
        return;
    }

    // case it's last
    if(item_to_remove->next_mmap == nullptr) {
        item_to_remove->prev_mmap->next = nullptr;
        item_to_remove->prev_mmap = nullptr;
        return;
    }

    // case it's in the middle - think about corner case of 3 elements (?)
    item_to_remove->prev_mmap->next_mmap = item_to_remove->next_mmap;
    item_to_remove->next_mmap->prev_mmap = item_to_remove->prev_mmap;
    item_to_remove->next_mmap = nullptr;
    item_to_remove->prev_mmap = nullptr;
    return;
}


/* Searches for a free block with up to ???size??? bytes or allocates (sbrk()) one if none are found.
 * Return value:
 * Success: returns pointer to the first byte in the allocated block (excluding the meta-data)
 * Failure:
 *      - If ???size??? is 0 returns NULL.
 *      - If ???size??? is more than 10^8 , return NULL.
 *      - If sbrk fails, return NULL.
 */
void* smalloc(size_t size) {

    size = _align_size(size);

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
    if(size > 128*1024){
        MallocMetadata *iterator = head_mmap;
        MallocMetadata *iterator_prev = iterator;

        while(iterator != nullptr) {
            // we just need to get to the end of the list (no free block which were allocated by mmap since munmap()
            // removes the virtual memory area
            iterator_prev = iterator;
            iterator = iterator->next_mmap;
        }

        //void* addr = mmap(NULL, size + sizeof(MallocMetadata), prot, flags, fildes, 0);
        void* addr = mmap(NULL, size + sizeof(MallocMetadata), PROT_WRITE | PROT_READ ,MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(addr == (void*)-1) {
            return nullptr;
        }

        MallocMetadata *new_metadata = (MallocMetadata*)addr;
        new_metadata->size = size;
        new_metadata->is_free = false;
        new_metadata->next = nullptr;
        new_metadata->prev = nullptr;
        new_metadata->next_free_item = nullptr;
        new_metadata->prev_free_item = nullptr;

        _add_to_mmap(new_metadata);

        addr = (void*)((char*)addr + sizeof(MallocMetadata));
        return addr;
    }

    // iterate over the MallocMetadata list & try to find allocation of size bytes
    int index = GetHistogramIndex(size);

    for(int i=index; i < 128;i++){
        MallocMetadata *iterator = free_blocks_histogram[i];
        MallocMetadata *iterator_prev = iterator;

        while(iterator != nullptr) {
            if(iterator->size >= size && iterator->is_free){
                iterator->is_free = false;
                addr = (void*)((char*)iterator + sizeof(MallocMetadata));

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
    }


    // challenge 3

    // if we reached here, means we didn't find free, big enough, chunk of free memory in our hist.
    // we know that the "Wilderness" chunk is the topmost block. i.e., it has the highest (?) address
    // means we need to iterate all allocated blocks list to find the last free block (will have the biggest addr)
    // basically I think it's iterator_prev if I'm not mistaken.
    // now we hold the "Wilderness" chunk and we can check how much we need to add to it in order to fulfill the
    // required allocation.
    // now, we can call sbrk with the effective size and just update the size field in the metadata of the "Wilderness" block
    MallocMetadata *iterator = head;
    MallocMetadata *iterator_prev = head;

    while(iterator) {
        iterator_prev = iterator;
        iterator = iterator->next;
    }

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
        iterator_prev->is_free= false;

        int index = GetHistogramIndex(iterator_prev->size);
        RemoveFreeItem(free_blocks_histogram[index],iterator_prev,index);

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

/* Searches for a free block of up to ???num??? elements, each ???size??? bytes that are all set to 0
 * or allocates if none are found. In other words, find/allocate size * num bytes and set all bytes to 0.
 * Return value:
 * Success: returns pointer to the first byte in the allocated block
 * Failure:
 *      - If ???size??? is 0 returns NULL.
 *      - If ???size??? is more than 10^8 , return NULL.
 *      - If sbrk fails, return NULL.
 * */
void* scalloc(size_t num, size_t size) {

    size = _align_size(size);

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

/* Releases the usage of the block that starts with the pointer ???p???.
 * If ???p??? is NULL or already released, simply returns.
 * Presume that all pointers ???p??? truly points to the beginning of an allocated block.
 * */
void sfree(void* p){
    if(p == nullptr) {
        return;
    }
    // calculate metadata
    MallocMetadata* p_metadata = (MallocMetadata*)((char*)p - sizeof(MallocMetadata));

    // by pdf if block is free, simply return
    if(p_metadata->is_free) return;

    // mark chunk as free
    p_metadata->is_free = true;

    // challenge 4
    // check if size fits mmap
    if(p_metadata->size > 128 * 1024) {

        /* remove element from head_mmap */
        _remove_from_mmap(p_metadata);
        /*
        // p is head
        if((MallocMetadata*)((char*)p - sizeof(MallocMetadata)) == head_mmap){
            // if p is the only element metadata->next = nullptr
            head_mmap = metadata->next;
        } else { // p is not head, so there's no need to touch the head_mmap pointer
            // if it's in the middle/last
            metadata->prev->next = metadata->next; // prev isn't null for sure since we're not head

            // case p is not the last element
            if(metadata->next != nullptr) {
                metadata->next->prev = metadata->prev;
            }
        }*/

        // now call munmap
        munmap(p_metadata, p_metadata->size + sizeof(MallocMetadata));

        return;
    }

    // challenge 3
    // combine adds item to free block list
    _combine_blocks(p_metadata);
    return;
}

/* If ???size??? is smaller than the current block???s size, reuses the same block. Otherwise,
 * finds/allocates ???size??? bytes for a new space, copies content of oldp into the new allocated space and frees the oldp.
 * Return value:
 * Success:
 *      - Returns pointer to the first byte in the (newly) allocated space.
 *      - If ???oldp??? is NULL, allocates space for ???size??? bytes and returns a pointer to it.
 * Failure:
 *      - If ???size??? is 0 returns NULL.
 *      - If ???size??? is more than 10^8 , return NULL.
 *      - If sbrk fails in allocating the needed space, return NULL.
 *      - Do not free ???oldp??? if srealloc() fails.
 * */
void* srealloc(void* oldp, size_t size) {

    size = _align_size(size);

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

    // check if it's the wilderness block
    MallocMetadata* iterator = head;
    MallocMetadata* iterator_prev = head;
    while (iterator) {
        iterator_prev = iterator;
        iterator = iterator->next;
    }

    if((void*)iterator_prev == (void*)((char*)oldp - sizeof(MallocMetadata))) {
        size_t extension_size = size - iterator_prev->size;

        addr = sbrk(extension_size);

        // check return values from sbrk(). by sbrk() documentation https://nxmnpg.lemoda.net/2/sbrk should do the following
        // res = sbrk() ...
        // if (res == (void*)-1) ...
        if(addr == (void*)-1) {
            return nullptr;
        }

        iterator_prev->size += extension_size;
        return (void*)((char*)iterator_prev + sizeof(MallocMetadata));
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

    // Piazza: consider using memmove instead
    memmove(addr, oldp, size_oldp);
    // memcpy(addr, oldp, size_oldp);


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
        iterator = iterator->next_mmap;
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
        iterator = iterator->next_mmap;
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
        counter++;
        iterator = iterator->next;
    }

    iterator = head_mmap;
    // iterate list & count elements
    while (iterator != nullptr) {
        counter++;
        iterator = iterator->next_mmap;
    }

    return (counter * _size_meta_data());
}
