//
// Created by student on 6/18/21.
//
// For Debugging
#include <iostream>

#include <unistd.h>

/* Tries to allocate ‘size’ bytes.
 * Return value:
 * Success: a pointer to the first allocated byte within the allocated block.
 * Failure:
 *      - If ‘size’ is 0 returns NULL.
 *      - If ‘size’ is more than 10^8 , return NULL.
 *      - If sbrk fails, return NULL.
 * */
void* smalloc(size_t size) {
    // check size == 0 || size > 10^8
    if(size == 0 || size > 1e8) {
        return nullptr;
    }

    // use sbrk() for allocation
    void* addr = sbrk(size);

    // check return values from sbrk(). https://nxmnpg.lemoda.net/2/sbrk by the sbrk documentation here should check
    // if (res == (void*)-1) ...
    if(addr == (void*)-1) {
        return nullptr;
    }

    // return something
    return addr;
}

int main() {
    // Test Errors

    // zero size allocation
    void* ptr;
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

    // Test allocations
    ptr = smalloc(20);
    std::cout << "addr of ptr is: " << ptr << std::endl;

    ptr = smalloc(70);
    std::cout << "addr of ptr is: " << ptr << std::endl;

    ptr = smalloc(1e3);
    std::cout << "addr of ptr is: " << ptr << std::endl;

    ptr = smalloc(2500);
    std::cout << "addr of ptr is: " << ptr << std::endl;

    ptr = smalloc(827);
    std::cout << "addr of ptr is: " << ptr << std::endl;
}
