// test_mmap.c
// Suite de pruebas completa y robusta para la syscall mmap en selfie.c
// Escrita en C* (subconjunto de C de Selfie)

uint64_t fd;
uint64_t addr1;
uint64_t addr2;
uint64_t addr_err;
uint64_t addr_align;
uint64_t addr_next;

uint64_t main() {
  write(1, "--- INICIANDO PRUEBAS DE ROBUSTEZ DE MMAP ---\n", 46);

  // Paso 1: Abrir selfie.c
  fd = open("selfie.c", 0, 0);
  if (fd == 4294967295) {
    write(1, "ERROR: No se pudo abrir selfie.c\n", 33);
    return 1;
  }

  // --- CASO DE EXITO 1: Mapeo estandar ---
  addr1 = mmap(0, 4096, 0, fd, 0);
  if (addr1 == 0) {
    write(1, "FALLO: addr1 es 0\n", 18);
    return 1;
  }
  if (addr1 == 4294967295) {
    write(1, "FALLO: addr1 es -1 (error inesperado)\n", 38);
    return 1;
  }
  write(1, "TEST 1 PASS: Mapeo basico exitoso\n", 34);

  // --- CASO DE ERROR 1: offset no alineado a pagina (ej. offset = 123) ---
  addr_err = mmap(0, 4096, 0, fd, 123);
  if (addr_err != 4294967295) {
    write(1, "FALLO: offset no alineado deberia retornar -1\n", 46);
    return 1;
  }
  write(1, "TEST 2 PASS: offset no alineado detectado correctamente y retorna -1\n", 69);

  // --- CASO DE ERROR 2: longitud cero (length == 0) ---
  addr_err = mmap(0, 0, 0, fd, 0);
  if (addr_err != 4294967295) {
    write(1, "FALLO: length = 0 deberia retornar -1\n", 38);
    return 1;
  }
  write(1, "TEST 3 PASS: longitud cero detectada correctamente y retorna -1\n", 64);

  // --- CASO DE ROBUSTEZ 3: longitud no alineada (ej. length = 1) ---
  // Debe redondearse hacia arriba a PAGESIZE (4096) internamente.
  addr_align = mmap(0, 1, 0, fd, 0);
  if (addr_align == 0) {
    write(1, "FALLO: addr_align es 0\n", 23);
    return 1;
  }
  if (addr_align == 4294967295) {
    write(1, "FALLO: addr_align es -1\n", 24);
    return 1;
  }

  // Si se alineó a PAGESIZE (4096), el siguiente mmap automatico
  // debe asignarse en addr_align + 4096 o posterior (sin solapamiento).
  addr_next = mmap(0, 4096, 0, fd, 0);
  if (addr_next < addr_align + 4096) {
    write(1, "FALLO: Alineamiento de tamaño incorrecto, colision de direcciones\n", 67);
    return 1;
  }
  write(1, "TEST 4 PASS: Redondeo de longitud no alineada a PAGESIZE funciona correctamente\n", 80);

  // --- CASO DE ROBUSTEZ 4: fd invalido (ej. fd = -1 o 999) ---
  // No debe crashear el emulador y debe asignar un bloque de memoria
  addr2 = mmap(0, 4096, 0, 4294967295, 0);
  if (addr2 == 0) {
    write(1, "FALLO: mmap con fd invalido retorno 0\n", 38);
    return 1;
  }
  if (addr2 == 4294967295) {
    write(1, "FALLO: mmap con fd invalido retorno -1\n", 39);
    return 1;
  }
  write(1, "TEST 5 PASS: mmap con fd invalido maneja la peticion con robustez\n", 66);

  // --- CASO DE EXITO 2: Sin solapamiento entre multiples mappings ---
  if (addr1 == addr_align) {
    write(1, "FALLO: Colision entre addr1 y addr_align\n", 41);
    return 1;
  }
  if (addr_align == addr_next) {
    write(1, "FALLO: Colision entre addr_align y addr_next\n", 45);
    return 1;
  }
  if (addr_next == addr2) {
    write(1, "FALLO: Colision entre addr_next y addr2\n", 40);
    return 1;
  }
  write(1, "TEST 6 PASS: Todos los mappings contiguos no se solapan\n", 56);

  write(1, "\n--- TODAS LAS PRUEBAS DE ROBUSTEZ PASARON CON EXITO ---\n", 57);
  return 0;
}

