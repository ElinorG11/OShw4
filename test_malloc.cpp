//
// Created by student on 6/18/21.
//

int main() {
    // find maxial allocation for sbrk() to fail
    // struct rlimit rlp;
    // getrlimit(RLIMIT_DATA,&rlp);
    // std::cout << rlp.rlim_max << std::endl;

    /* Start tests Part 1 */

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

    // Test concurrent allocations & check their size
    ptr = smalloc(20);
    void* new_ptr = (void*)((char*)head + 0x20);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 20. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 20. head: " << head << " new ptr: " << ptr << std::endl;
    }

    ptr = smalloc(50);
    new_ptr = (void*)((char*)head + 0x54);
    if(new_ptr == ptr) {
        std::cout << "SUCCESS: Test allocation size 50. head: " << head << " new ptr: " << ptr << std::endl;
    } else {
        std::cout << "FAIL: Test allocation size 50. head: " << head << " new ptr: " << ptr << std::endl;
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
    /* Done tests Part 1 */

    /* Start tests Part 2 */

    return 0;
}
