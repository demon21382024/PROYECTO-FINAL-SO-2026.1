// test_munmap.c
uint64_t open(uint64_t* filename, uint64_t flags, uint64_t mode);
uint64_t mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t fd, uint64_t offset);
uint64_t munmap(uint64_t addr);

int main() {
    uint64_t fd;
    uint64_t mapped_mem;
    uint64_t ret;

    fd = open((uint64_t*) "dummy.txt", 0, 0);
    if (fd == 4294967295) {
        return 1;
    }

    mapped_mem = mmap(0, 4096, 0, fd, 0);
    if (mapped_mem == 4294967295) {
        return 2;
    }

    // Try to unmap the valid address
    ret = munmap(mapped_mem);
    if (ret != 0) {
        return 3;
    }

    // Try to unmap an invalid address (should fail)
    ret = munmap(4096);
    if (ret != 4294967295) {
        return 4;
    }

    return 0;
}
