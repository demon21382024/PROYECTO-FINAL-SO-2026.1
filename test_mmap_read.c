// test_mmap_read.c

uint64_t open(uint64_t* filename, uint64_t flags, uint64_t mode);
uint64_t mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t fd, uint64_t offset);
void exit(uint64_t status);

int main() {
    uint64_t fd;
    uint64_t* mapped_mem;
    uint64_t first_word;

    // Abrimos el archivo dummy.txt para leer
    fd = open((uint64_t*) "dummy.txt", 0, 0);

    if (fd == 4294967295) {
        // fd == -1 truncado a 32 bits
        return 1;
    }

    // Mapeamos 4096 bytes (1 pagina) del archivo dummy.txt
    mapped_mem = (uint64_t*) mmap(0, 4096, 0, fd, 0);

    if ((uint64_t) mapped_mem == 4294967295) {
        // mmap failed
        return 2;
    }

    // Al acceder a mapped_mem, el page fault handler deberia:
    // 1. Detectar que mapped_mem esta dentro de un mmap
    // 2. Localizar o crear un cache frame
    // 3. Leer la primera pagina de dummy.txt a ese frame
    // 4. Mapear la pagina virtual a ese frame
    
    first_word = *mapped_mem;

    // "12345678" en memoria little-endian es 0x3837363534333231
    // Su equivalente en decimal es: 4050765991979987505

    if (first_word == 4050765991979987505) {
        // Exito
        return 0;
    }

    // Fallo
    return 3;
}
