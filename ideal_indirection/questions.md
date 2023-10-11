/**
 * ideal_indirection
 * CS 341 - Fall 2023
 */
For all of these problems we want you to answer them in your own words to the best of your abilities. You are still allowed to refer to resources like lecture notes, the coursebook, Google, your peers and your friends. We care a lot more that you learn from these questions then being able to do them 100% yourself. So if you get stuck ... "ask your neighbor" :)


For these questions please read the [coursebook](http://cs241.cs.illinois.edu/coursebook/Ipc#translating-addresses)
* What is a 'virtual address'? -> address used by the programmer
* What is a 'physical address'? -> actual address in the machine (RAM) 
* Can two processes read and write to the same virtual address? -> NO, as processes have separate virtual address spaces for ISOLATION purposes
* What is an MMU? -> Memory Management Unit; piece of hardware that does virtual -> physical address conversion
* What is a Page Table? -> Lookup table that maps Logical (virtual) pages to Physical (Page) Frames
  * What is the difference between a page and a frame? Page is virtual, frame is physical
    * Which is larger? - They have the same size
  * What is a virtual page number? - Index / address of a page in virtual adress space
  * What is an offset? -> "how far apart" from the base address we have to go in order to index a specific byte (assuming byte-addressable memory) from the page / frame
* How much physical memory (in GB) can a 32 bit machine address? Explain. -> 2^32 Bytes = 4GBs, as there are 2^32 addresses in the physical space
* How much physical memory (in GB) can a 64 bit machine address? Explain. -> 2^64 Bytes = 160PetaBytes, -//-
* On a 32 bit machine with 2kb pages and 4byte entries how big would a Page Table (single tier) have to be to address all of physical memory? Explain.
2kb / 4 bytes = 2^9 Page Table Entries (2kb = 2^9 PTEs * 2^2 bytes/PTE); 32-bit machine => addr. space of 2^32, need 32bits = 4bytes to index the physical mem.
* On a 64 bit machine with 8kb pages and 8byte entries how big a Page Table (single tier) have to be to address all of physical memory? Explain.
8kb / 8 bytes = 2^10 Page Table Entries (8kb = 2^13 PTEs * 2^3 bytes/PTE); 64-bit machine => addr. space of 2^64, need 64bits = 8bytes to index the physical mem.
  * [ASK!] If you had 32GB of ram would this work out? Why or why not?
  32 GB of RAM = 2^35 Bytes of RAM; 64-bit machine => phy addr = 8 bytes =? 2^35 / 2^3 = 2^32 physical adddresses; It would work, as for 64-bit machines we have 2^64 entries / physical addresses, as 2^64 > 2^32 => (2^64 - 2^32) entries won't be used   
* What is a multi-level page table?
  Page table w/ multiple levels of indirection; there exist multiple Page Tables:
  Step 1: Lookup page dictionary's MSBs to index the correct Page Table
  Step 2: Repepat process from single-level page table
  * What problem do they solve ("All problems in computer science can be solved by another level of indirection")? - Finite amount of memory, tries to reduce using memory, make it feasible for large address spaces
  * But what problem do they introduce ("... except of course for the problem of too many indirections")? - More interaction, more memory needed to store the Data structures for these indirections (i.e: in multi-level page table we tradeoff the memory that we index in physical address by the extra memory used to store extra tables); Store more stuff in RAM (ie.: extra page tables)
    * How can we solve this problem / What is a TLB ("... and this is problem is solved by the cache")? - By adding a cache that keeps track of most-recently virtual-physical page mappings; this is the TLB = Translation Lookaside Buffer, which allows us to cache entries used frequently / recently from Page Tables; stored on Chip (not in RAM)
    * Provide me one example where you use indirection in computer-science and one example where you use indirection in real life. 
    In CS: Interfaces, classes, Network layers
    In real life: tell a friend to pursue another friend, as he is more close to him than I am
* What is a page fault? - When the frame that is being addresses is not found in RAM 
* What is a cache miss? - When an entry that is looked-up in the cache is missing

For these questions please read mmu.h, page_table.h and tlb.h:

* How is our page table implemented? - An array of page table entries (PTEs)
  * What does each entry point to? - to a memory frame
* How many levels of page tables are we working with? - 2
* How many entries does a page table have? - 2^10 = PAGE_SIZE / sieof(page_directory_entry)
* How many bytes is a page table? -  4KiB = 2^12 bytes
* How much memory (in GB) can we address with our multi-level page table (for a single process)? Explain.
[ASK!] 2 ^ 10 Page Tables * 4 bytes / physical address * 4 KiB bytes / frame = 16MiB  

* How is our TLB implemented (Note that it is an LRU Cache) - SLL (Singly Linked List), most recently used at HEAD (where we add/insert), LRU at tail (where we evict)
* What does LRU mean? - Least-Recently Used eviction policy used in Caching
* Which element is the most recently used element? - HEAD
* Which element is the least recently used element? - TAIL
* What is the runtime of TLB_get_physical_address()?  - O(N) have to traverse entire LL (cache)
* What is the runtime of TLB_add_physical_address()? - O(N) have to search if cache is full
* Why do TLB_get_physical_address() and TLB_add_physical_address() need a double pointer? -> Pointer to a `tlb*`
* Why do TLB_get_physical_address() and TLB_add_physical_address() need both a virtual address and pid? - [ASK!] Did not find this method; so that they can transform virtual address and pid to true physical address (make use of `get_system_pointer` methods from `kernel.h`)

THIS IS NOT GRADED! But it will really help you get to know virtual memory and our simulation to complete the assignment.
