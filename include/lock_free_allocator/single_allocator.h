#ifndef SINGLE_ALLOCATOR_H
#define SINGLE_ALLOCATOR_H

#include "slab.h"
#include "lowlockqueue.h"
#include <deque>
#include <stack>

struct SingleAllocator {
    LowLockQueue free_slabs;
        // Queue for all the free slabs available

    size_t sz;

    // used for debugging
    std::deque<Slab*> all_slabs;

    void viewState() {
        std::cout << "\t----- Viewing state for SingleAllocator(" << sz << ") -----" << std::endl;
        int slab_num = 0;
        for (Slab* slab : all_slabs) {
            std::cout << "\t----- Slab# " << slab_num << "-----\n"
                << "\tnum_free = " << slab->num_free << '\n'
                << "\tfree_slots = " << slab->free_slots << '\n'
                << "\t----------\n" << std::endl;
            ++slab_num;
        }
        std::cout << "\t----- End viewState for SingleAllocator -----\n\n\n" << std::endl;
    }

    // Constructs a SingleAllocator that contains Slabs, where each slot
    // in the Slab is of size 's'
    SingleAllocator(size_t s) : sz(s)
    {
        //std::cout << "Create singleallocator" << std::endl;
        free_slabs.push_back(new Slab(sz));
        all_slabs.emplace_back(free_slabs.front());
    }

    /** POTENTIAL LOCK FREE IMPLEMENTATION using a lock-free queue
     * Slab *slab;
     * void *p;
     * while (p == nullptr) {
     *     slab = free_slabs.front();
     *     p, new_num_free = slab->insert();
     * }
     *
     * if (new_num_free == 0) {
     *   free_slabs.pop_front();
     *      // Should bake the invariant of always having at least one slab
     *      // with open space in the queue INTO the lock-free queue itself
     * }
     *
     * return p;
     */

    [[nodiscard]]
    void* allocate()
    {
        Slab *slab;

        bool success = free_slabs.pop_front(slab, sz);
        while (!success) {
            success = free_slabs.pop_front(slab, sz);
        }

        std::pair<void*, int> res = slab->insert();

        return res.first;
    }

    void deallocate(void* p)
    {
        // Alignment for Slab::data
        size_t alignment = sz * 2 * (Slab::MAX_SLOTS + 1);

        // Offset (# of bytes) into Slab::data
        size_t offset = ((long long)p) % alignment;

        // The slot index for p
        int slot_num = (offset - sizeof(void*)) / sz;

        Slab *slab = *((Slab**) (((char*)p) - offset));
        
        int old_num_free = slab->erase(slot_num);

        // If the slab was full, add it to the free list since it now
        // has a free slot
        if (old_num_free == 0) {
            free_slabs.push_back(slab);
        }
    }

    ~SingleAllocator() {
    }

};

#endif /* ifndef SINGLE_ALLOCATOR_H */
