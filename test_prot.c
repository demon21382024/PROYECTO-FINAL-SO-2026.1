// test_prot.c
// Demuestra la validacion de prot: escribir en un mapping de SOLO LECTURA
// (prot = 0) debe ser rechazado (segmentation fault), mientras que leer si
// esta permitido.
//
// Nota: como un segmentation fault termina el programa, este es un test de
// "debe fallar": el exito se observa en que el emulador imprime el segfault
// y NUNCA se imprime la linea "FALLO". El exit code sera != 0 (segfault).

uint64_t open(uint64_t* filename, uint64_t flags, uint64_t mode);
uint64_t mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t fd, uint64_t offset);

uint64_t* ptr;

uint64_t main() {
  uint64_t fd;
  uint64_t mapped;
  uint64_t x;

  write(1, "--- TEST PROT: escritura en mapping de solo lectura ---\n", 56);

  fd = open("dummy.txt", 0, 0);
  if (fd == 4294967295) {
    write(1, "ERROR: no se pudo abrir dummy.txt\n", 34);
    return 1;
  }

  // mapear SOLO LECTURA (prot = 0)
  mapped = mmap(0, 4096, 0, fd, 0);
  if (mapped == 4294967295) {
    write(1, "ERROR: mmap retorno -1\n", 23);
    return 1;
  }
  ptr = (uint64_t*) mapped;

  // leer SI esta permitido (dispara demand paging)
  x = *ptr;
  write(1, "PASO 1 OK: lectura de un mapping de solo lectura permitida\n", 58);

  // escribir NO esta permitido -> debe provocar segmentation fault
  write(1, "PASO 2: intentando escribir (deberia ser bloqueado)...\n", 55);
  *ptr = 42;

  // si llegamos aqui, prot NO se hizo cumplir
  write(1, "FALLO: la escritura en solo lectura NO fue bloqueada\n", 52);
  return 1;
}
