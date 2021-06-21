//
// Created by student on 6/18/21.
//

size_t aligned_size(size_t size){
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

/******************************IMPORTANT NOTE******************************/
/* don't forget to add call for aligned_size in smalloc
 * I'm not sure we need to add this support for mmap() as well since it's not continuous region of memory
 * but a seperate area... check on Piazza.
 */
