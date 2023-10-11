/**
 * ideal_indirection
 * CS 341 - Fall 2023
 */
#include "mmu.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MSB_20 0xFFFFF000
#define LSB_10 ((1 << 10) - 1)
#define LSB_12 ((1 << 12) - 1)

mmu *mmu_create() {
    mmu *my_mmu = calloc(1, sizeof(mmu));
    my_mmu->tlb = tlb_create();
    return my_mmu;
}

void mmu_rw_from_virtual_address(mmu *this, addr32 virtual_address,
                                   size_t pid, void *buffer, size_t num_bytes, int write) {

    if (pid != this->curr_pid) // context switch
    {
        tlb_flush(&this->tlb);
        this->curr_pid = pid;
    }
    if (!address_in_segmentations(this->segmentations[pid], virtual_address)) { // Make sure that the address is in one of the segmentations.
        mmu_raise_segmentation_fault(this); //  If not, raise a segfault and return
        return;
    }

    if (write) // check write permission
    {
        vm_segmentation *seg = find_segment(this->segmentations[pid], virtual_address);
        int allow_write = seg->permissions & WRITE;
        if (!allow_write) { //  If not, raise a segfault and return
            mmu_raise_segmentation_fault(this);
            return;
        }
    }
    else { // check read permission

        vm_segmentation *seg = find_segment(this->segmentations[pid], virtual_address);
        int allow_read = seg->permissions & READ;
        if (!allow_read) { //  If not, raise a segfault and return
            mmu_raise_segmentation_fault(this);
            return;
        }
    }

    addr32 base_virtual_addr_tlb = (virtual_address & MSB_20) >> NUM_OFFSET_BITS; // The virtual address with the offset removed.
    page_table_entry *pte = tlb_get_pte(&this->tlb, base_virtual_addr_tlb); // Check the TLB for the page table entry

    if (!pte) { // if it is not in TLB

    mmu_tlb_miss(this); // Raise a TLB miss
    
    page_directory *pd = this->page_directories[pid];  // Get the page directory for the specific process; each process has diff. Page Directories
    addr32 pde_id = (virtual_address >> 22); // TODO: ASK! Lab
    page_directory_entry *pde = &pd->entries[pde_id]; // get the page directory entry

    if (!pde->present) // not present in memory
    {
        mmu_raise_page_fault(this);
        addr32 frame_addr = (ask_kernel_for_frame(NULL) >> NUM_OFFSET_BITS); // ask_kernel_for_frame  returns 4KiB = 2^12 alligned bytes => remove LSB 12 0s
        pde->base_addr = frame_addr; // 20bits; !!! -> where pde points to (to the frame memory addr)
        // ASK: do we have to?
        read_page_from_disk((page_table_entry *)pde); //  page table entry points to a frame of memory that has been swapped to disk, and the page table entry is now pointing to a valid physical memory frame
        pde->present = 1;
        pde->read_write = 1;
        pde->user_supervisor = 0; // supervisor priviliges

    }

    page_table *pt = (page_table *)get_system_pointer_from_pde(pde); // Get the page table using the PDE
    assert(((virtual_address >> NUM_OFFSET_BITS) & LSB_10) == ((virtual_address & 0x003FF000) >> NUM_OFFSET_BITS));
    addr32 pt_id = (virtual_address >> NUM_OFFSET_BITS) & LSB_10; 
    pte = &pt->entries[pt_id]; // Get the page table entry from the page table

    // Idea: Do it at the end, as we have to add it to TLB anyways
    // tlb_add_pte(&this->tlb, base_virtual_addr_tlb, pte); // Add the entry to the TLB
    }


    if (!pte->present) { // If the page table entry is not present in memory:
        mmu_raise_page_fault(this);

                                // The page table entry that will point to this frame.
        addr32 frame_addr = (ask_kernel_for_frame(pte) >> NUM_OFFSET_BITS); // ask_kernel_for_frame  returns 4KiB = 2^12 alligned bytes => remove LSB 12 0s
        pte->base_addr = frame_addr; // 20bits; !!! -> where pde points to (to the frame memory addr)
        read_page_from_disk(pte); //  page table entry points to a frame of memory that has been swapped to disk, and the page table entry is now pointing to a valid physical memory frame
        pte->present = 1;
        pte->read_write = 1;
        pte->user_supervisor = 1; // user priviliges
    }

    //TODO: !
    // Check that the user has permission to perform the read or write operation. If not, raise a segfault and return
    // if (write) {
    //     if (!pte->read_write) {
    //         mmu_raise_segmentation_fault(this);
    //         return;
    //     }
    // }
     
     //TODO: Ask lab
     // Use the page table entry’s base address and the offset of the virtual address to compute the physical address. 
     addr32 offset = (virtual_address & LSB_12);
    // Before
     addr32 phys_addr = ((pte->base_addr << NUM_OFFSET_BITS) + offset);
     void *phys_addr_ptr = get_system_pointer_from_address(phys_addr); // Get a physical pointer from this address
     
     // After
    //  void *phys_addr_ptr = get_system_pointer_from_pte(pte) + offset;

     if (write)
         memcpy(phys_addr_ptr, buffer, num_bytes);
     else // perform read op
        memcpy(buffer, phys_addr_ptr, num_bytes);

     tlb_add_pte(&this->tlb, base_virtual_addr_tlb, pte); // Add the entry to the TLB
     pte->accessed = 1; //Page Table was read from
     if (write)
        pte->dirty = 1;
}

void mmu_read_from_virtual_address(mmu *this, addr32 virtual_address,
                                   size_t pid, void *buffer, size_t num_bytes) {
    assert(this);
    assert(pid < MAX_PROCESS_ID);
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
    // TODO: Implement me!

    mmu_rw_from_virtual_address(this, virtual_address, pid, buffer, num_bytes, 0);
}

void mmu_write_to_virtual_address(mmu *this, addr32 virtual_address, size_t pid,
                                  const void *buffer, size_t num_bytes) {
    assert(this);
    assert(pid < MAX_PROCESS_ID);
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
    // TODO: Implement me!

    void *buffer_non_const = strdup(buffer);
    mmu_rw_from_virtual_address(this, virtual_address, pid, buffer_non_const, num_bytes, 1);
    free(buffer_non_const);
}

void mmu_tlb_miss(mmu *this) {
    this->num_tlb_misses++;
}

void mmu_raise_page_fault(mmu *this) {
    this->num_page_faults++;
}

void mmu_raise_segmentation_fault(mmu *this) {
    this->num_segmentation_faults++;
}

void mmu_add_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    addr32 page_directory_address = ask_kernel_for_frame(NULL);
    this->page_directories[pid] =
        (page_directory *)get_system_pointer_from_address(
            page_directory_address);
    page_directory *pd = this->page_directories[pid];
    this->segmentations[pid] = calloc(1, sizeof(vm_segmentations));
    vm_segmentations *segmentations = this->segmentations[pid];

    // Note you can see this information in a memory map by using
    // cat /proc/self/maps
    segmentations->segments[STACK] =
        (vm_segmentation){.start = 0xBFFFE000,
                          .end = 0xC07FE000, // 8mb stack
                          .permissions = READ | WRITE,
                          .grows_down = true};

    segmentations->segments[MMAP] =
        (vm_segmentation){.start = 0xC07FE000,
                          .end = 0xC07FE000,
                          // making this writeable to simplify the next lab.
                          // todo make this not writeable by default
                          .permissions = READ | EXEC | WRITE,
                          .grows_down = true};

    segmentations->segments[HEAP] =
        (vm_segmentation){.start = 0x08072000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[BSS] =
        (vm_segmentation){.start = 0x0805A000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[DATA] =
        (vm_segmentation){.start = 0x08052000,
                          .end = 0x0805A000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[TEXT] =
        (vm_segmentation){.start = 0x08048000,
                          .end = 0x08052000,
                          .permissions = READ | EXEC,
                          .grows_down = false};

    // creating a few mappings so we have something to play with (made up)
    // this segment is made up for testing purposes
    segmentations->segments[TESTING] =
        (vm_segmentation){.start = PAGE_SIZE,
                          .end = 3 * PAGE_SIZE,
                          .permissions = READ | WRITE,
                          .grows_down = false};
    // first 4 mb is bookkept by the first page directory entry
    page_directory_entry *pde = &(pd->entries[0]);
    // assigning it a page table and some basic permissions
    pde->base_addr = (ask_kernel_for_frame(NULL) >> NUM_OFFSET_BITS);
    pde->present = true;
    pde->read_write = true;
    pde->user_supervisor = true;

    // setting entries 1 and 2 (since each entry points to a 4kb page)
    // of the page table to point to our 8kb of testing memory defined earlier
    for (int i = 1; i < 3; i++) {
        page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
        page_table_entry *pte = &(pt->entries[i]);
        pte->base_addr = (ask_kernel_for_frame(pte) >> NUM_OFFSET_BITS);
        pte->present = true;
        pte->read_write = true;
        pte->user_supervisor = true;
    }
}

void mmu_remove_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    // example of how to BFS through page table tree for those to read code.
    page_directory *pd = this->page_directories[pid];
    if (pd) {
        for (size_t vpn1 = 0; vpn1 < NUM_ENTRIES; vpn1++) {
            page_directory_entry *pde = &(pd->entries[vpn1]);
            if (pde->present) {
                page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
                for (size_t vpn2 = 0; vpn2 < NUM_ENTRIES; vpn2++) {
                    page_table_entry *pte = &(pt->entries[vpn2]);
                    if (pte->present) {
                        void *frame = (void *)get_system_pointer_from_pte(pte);
                        return_frame_to_kernel(frame);
                    }
                    remove_swap_file(pte);
                }
                return_frame_to_kernel(pt);
            }
        }
        return_frame_to_kernel(pd);
    }

    this->page_directories[pid] = NULL;
    free(this->segmentations[pid]);
    this->segmentations[pid] = NULL;

    if (this->curr_pid == pid) {
        tlb_flush(&(this->tlb));
    }
}

void mmu_delete(mmu *this) {
    for (size_t pid = 0; pid < MAX_PROCESS_ID; pid++) {
        mmu_remove_process(this, pid);
    }

    tlb_delete(this->tlb);
    free(this);
    remove_swap_files();
}


