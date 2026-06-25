// test_msync_persist.c
// Demuestra que msync PERSISTE en disco los cambios hechos sobre un mapping.
//
// Flujo:
//   1. crear el archivo con contenido inicial "AAAAAAAA"
//   2. abrir el mismo archivo en O_RDWR y mapearlo con prot de escritura
//   3. leer desde el mapping  -> dispara demand paging (carga el archivo)
//   4. modificar la memoria mapeada -> "BBBBBBBB"
//   5. msync(addr)            -> escribe el cache frame de vuelta al archivo
//   6. reabrir el archivo con un fd NUEVO y leer directamente de disco
//   7. verificar que el contenido en memoria secundaria cambio
//
// Cubre el caso 2 de info_tests.md y el entregable #4 del enunciado.

uint64_t* buf;
uint64_t* ptr;

uint64_t main() {
  uint64_t fd_init;
  uint64_t fd_map;
  uint64_t fd_check;
  uint64_t mapped;
  uint64_t disk_value;

  write(1, "--- TEST MSYNC: persistencia a disco ---\n", 41);

  // 1. Crear el archivo con contenido inicial "AAAAAAAA"
  //    577 = O_WRONLY | O_CREAT | O_TRUNC, 420 = 0644
  fd_init = open("msync_data.txt", 577, 420);
  if (fd_init == 4294967295) {
    write(1, "ERROR: no se pudo crear msync_data.txt\n", 39);
    return 1;
  }
  buf = malloc(8);
  *buf = 4702111234474983745; // "AAAAAAAA" en little-endian
  write(fd_init, buf, 8);

  // 2. Abrir el mismo archivo en lectura/escritura (2 = O_RDWR) para mapearlo
  fd_map = open("msync_data.txt", 2, 0);
  if (fd_map == 4294967295) {
    write(1, "ERROR: no se pudo abrir en O_RDWR\n", 34);
    return 1;
  }

  // 3. Mapear con permiso de lectura/escritura (prot = 2 = RW)
  mapped = mmap(0, 4096, 2, fd_map, 0);
  if (mapped == 4294967295) {
    write(1, "ERROR: mmap retorno -1\n", 23);
    return 1;
  }
  ptr = (uint64_t*) mapped;

  // 4. Leer desde el mapping: fuerza demand paging y confirma el contenido inicial
  if (*ptr != 4702111234474983745) {
    write(1, "FALLO: la lectura inicial del mapping no es 'AAAAAAAA'\n", 54);
    return 1;
  }
  write(1, "PASO 1 OK: demand paging cargo el contenido del archivo\n", 56);

  // 5. Modificar la memoria mapeada -> "BBBBBBBB"
  *ptr = 4774451407313060418; // "BBBBBBBB" en little-endian
  write(1, "PASO 2 OK: memoria mapeada modificada a 'BBBBBBBB'\n", 51);

  // 6. Persistir los cambios al archivo
  if (msync(mapped) != 0) {
    write(1, "FALLO: msync retorno error\n", 27);
    return 1;
  }
  write(1, "PASO 3 OK: msync ejecutado correctamente\n", 41);

  // 7. Reabrir el archivo con un fd NUEVO (0 = O_RDONLY) y leer de disco
  fd_check = open("msync_data.txt", 0, 0);
  if (fd_check == 4294967295) {
    write(1, "ERROR: no se pudo reabrir para verificar\n", 41);
    return 1;
  }
  buf = malloc(8);
  *buf = 0;
  read(fd_check, buf, 8);
  disk_value = *buf;

  // 8. Verificar que el cambio quedo en memoria secundaria
  if (disk_value != 4774451407313060418) {
    write(1, "FALLO: el archivo en disco NO refleja el cambio de msync\n", 57);
    return 1;
  }

  write(1, "PASO 4 OK: el archivo en disco ahora contiene 'BBBBBBBB'\n", 57);
  write(1, "--- TEST MSYNC PASS: msync persiste a disco ---\n", 48);
  return 0;
}
