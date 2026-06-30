// test_msync.c
uint64_t open(uint64_t* filename, uint64_t flags, uint64_t mode);
uint64_t mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t fd, uint64_t offset);
uint64_t msync(uint64_t addr);

int main() {
    uint64_t fd;
    uint64_t mapped_mem;
    uint64_t* ptr;
    uint64_t ret;

    // open in read-write mode (2 = O_RDWR is not defined, we use 0 but the emulator doesn't care about flags)
    fd = open((uint64_t*) "dummy.txt", 0, 0);
    if (fd == 4294967295) {
        return 1;
    }

    mapped_mem = mmap(0, 4096, 2, fd, 0);
    if (mapped_mem == 4294967295) {
        return 2;
    }

    ptr = (uint64_t*) mapped_mem;
    
    // Modify the mapped memory (triggering demand paging first)
    *ptr = 4050765991979987505; // Write "12345678" again to trigger a fault and load it

    // Write back to file
    ret = msync(mapped_mem);
    if (ret != 0) {
        return 3;
    }

    // Try to sync invalid address
    ret = msync(4096);
    if (ret != 4294967295) {
        return 4;
    }

    return 0;
}
