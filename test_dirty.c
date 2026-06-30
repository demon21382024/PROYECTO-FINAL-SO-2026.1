// test_dirty.c
// Demuestra el dirty bit: msync solo escribe a disco los frames MODIFICADOS.
//
//   CASO A: solo se LEE el mapping  -> el frame queda LIMPIO
//           => msync NO escribe (el kernel imprime "skipped clean page")
//   CASO B: se MODIFICA el mapping  -> el frame queda SUCIO
//           => msync SI escribe (el kernel imprime "wrote dirty page")
//
// La demostracion son los prints del propio selfie tras cada msync.

uint64_t open(uint64_t* filename, uint64_t flags, uint64_t mode);
uint64_t mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t fd, uint64_t offset);
uint64_t msync(uint64_t addr);

uint64_t* ptr;
uint64_t* buf;

uint64_t main() {
  uint64_t fd_init;
  uint64_t fd_map;
  uint64_t mapped;
  uint64_t x;

  write(1, "TEST DIRTY BIT\n", 15);

  // crear el archivo con contenido inicial "AAAAAAAA"
  fd_init = open("dirty_data.txt", 577, 420);
  if (fd_init == 4294967295) { write(1, "ERROR crear\n", 12); return 1; }
  buf = malloc(8);
  *buf = 4702111234474983745; // "AAAAAAAA"
  write(fd_init, buf, 8);

  // abrir en O_RDWR y mapear con prot = RW
  fd_map = open("dirty_data.txt", 2, 0);
  if (fd_map == 4294967295) { write(1, "ERROR abrir\n", 12); return 1; }
  mapped = mmap(0, 4096, 2, fd_map, 0);
  if (mapped == 4294967295) { write(1, "ERROR mmap\n", 11); return 1; }
  ptr = (uint64_t*) mapped;

  // CASO A: solo leer (dispara demand paging, el frame queda LIMPIO)
  x = *ptr;
  write(1, "A) leer + msync:\n", 17);
  msync(mapped);              // -> "msync skipped clean page"

  // CASO B: modificar (el frame queda SUCIO)
  *ptr = 4774451407313060418; // "BBBBBBBB"
  write(1, "B) escribir + msync:\n", 21);
  msync(mapped);              // -> "msync wrote dirty page"

  write(1, "TEST DIRTY OK\n", 14);
  return 0;
}
