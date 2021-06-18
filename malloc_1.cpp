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

    // check return values from sbrk() - two options for conversion that I checked (& hopefully they really do work):
    // int conversion = *(int*)res;
    // int conversion = *reinterpret_cast<int*>(res);
    // if (conversion == -1) ...

    // return something
}
