uint64_t* ptr;
uint64_t* buf;

uint64_t main() {
  uint64_t fd_init;
  uint64_t fd_map;
  uint64_t mapped;
  uint64_t pid;

  write(1, "--- TEST SHARED: cache frames compartidos entre procesos ---\n", 61);

  // 1. Crear el archivo con contenido inicial "AAAAAAAA"
  //    577 = O_WRONLY | O_CREAT | O_TRUNC, 420 = 0644
  fd_init = open("shared_data.txt", 577, 420);
  if (fd_init == 4294967295) {
    write(1, "ERROR: no se pudo crear shared_data.txt\n", 40);
    return 1;
  }
  buf = malloc(8);
  *buf = 4702111234474983745; // "AAAAAAAA"
  write(fd_init, buf, 8);

  // 2. Abrir en O_RDWR (2) y mapear ANTES del fork
  fd_map = open("shared_data.txt", 2, 0);
  if (fd_map == 4294967295) {
    write(1, "ERROR: no se pudo abrir en O_RDWR\n", 34);
    return 1;
  }

  mapped = mmap(0, 4096, 2, fd_map, 0);
  if (mapped == 4294967295) {
    write(1, "ERROR: mmap retorno -1\n", 23);
    return 1;
  }
  ptr = (uint64_t*) mapped;

  // 3. El padre accede una vez: demand paging asigna el cache frame compartido
  if (*ptr != 4702111234474983745) {
    write(1, "FALLO: contenido inicial del mapping incorrecto\n", 48);
    return 1;
  }
  write(1, "PADRE: mapping cargado con 'AAAAAAAA'\n", 38);

  // 4. Crear el proceso hijo (hereda el mapping y el cache frame)
  pid = fork();
  if (pid == 4294967295) {
    write(1, "FALLO: fork fallo\n", 18);
    return 1;
  }

  if (pid == 0) {
    // ----- HIJO -----
    // Escribe en la memoria mapeada. Como el frame es compartido, el padre
    // debera observar este cambio sin necesidad de msync ni de releer el disco.
    *ptr = 4774451407313060418; // "BBBBBBBB"
    write(1, "HIJO: escribio 'BBBBBBBB' en la memoria mapeada\n", 48);
    exit(0);
  } else {
    // ----- PADRE -----
    wait((uint64_t*) 0);

    if (*ptr != 4774451407313060418) {
      write(1, "FALLO: el padre NO ve el cambio del hijo (frames NO compartidos)\n", 64);
      return 1;
    }
    write(1, "PADRE: observa 'BBBBBBBB' -> cache frame COMPARTIDO con el hijo\n", 63);
  }

  write(1, "--- TEST SHARED PASS: procesos comparten cache frames ---\n", 58);
  return 0;
}
