//
// Created by student on 6/18/21.
//
#include <unistd.h>

/* Tries to allocate ‘size’ bytes.
 * Return value:
 * Success: a pointer to the first allocated byte within the allocated block.
 * Failure:
 *      - If ‘size’ is 0 returns NULL.
 *      - If ‘size’ is more than 10^8 , return NULL.
 *      - If sbrk fails, return NULL.
 * */
void smalloc(size_t size) {
    // check size == 0 || size > 10^8

    // use sbrk() for allocation

    // check return values from sbrk(). https://nxmnpg.lemoda.net/2/sbrk by the sbrk documentation here should check
    // if (res == (void*)-1) ...

    // return something
}
