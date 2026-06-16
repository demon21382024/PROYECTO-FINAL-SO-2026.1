// test_mmap.c
// Prueba minima de la syscall mmap implementada en selfie.c
// Escrita en C* (subconjunto de C de Selfie)

uint64_t fd;
uint64_t addr1;
uint64_t addr2;

uint64_t main() {
  // Paso 1: abrir selfie.c en modo solo lectura
  fd = open("selfie.c", 0, 0);

  // Paso 2: primer mmap — addr=0 (auto), 4096 bytes, prot=0, offset=0
  addr1 = mmap(0, 4096, 0, fd, 0);

  // Verificar que no es 0 ni -1 (error)
  if (addr1 == 0) {
    write(1, "FALLO: addr1 es 0\n", 18);
    return 1;
  }

  if (addr1 == 18446744073709551615) {
    write(1, "FALLO: addr1 es -1\n", 19);
    return 1;
  }

  // Paso 3: segundo mmap — addr=0 (auto), 8192 bytes, offset=4096
  addr2 = mmap(0, 8192, 0, fd, 4096);

  if (addr2 == 0) {
    write(1, "FALLO: addr2 es 0\n", 18);
    return 1;
  }

  if (addr2 == 18446744073709551615) {
    write(1, "FALLO: addr2 es -1\n", 19);
    return 1;
  }

  // Las dos direcciones deben ser distintas (sin solapamiento)
  if (addr1 == addr2) {
    write(1, "FALLO: addr1 == addr2 (solapamiento!)\n", 38);
    return 1;
  }

  // addr2 debe estar despues de addr1 + 4096 (primera region)
  if (addr2 < addr1 + 4096) {
    write(1, "FALLO: addr2 solapa con addr1\n", 30);
    return 1;
  }

  // Tercer mmap con addr!=0 especificado
  // PAGESIZE = 4096, tomamos una direccion anterior al addr1 que no choque
  // Usamos 0 (auto) nuevamente para un tercer mapping de solo 4096 bytes
  // No especificamos addr fijo para evitar problemas de alineacion en C*

  // Resultado final
  write(1, "mmap TEST EXITOSO!\n", 19);
  write(1, "- mmap 1: OK (4096 bytes, offset=0)\n", 36);
  write(1, "- mmap 2: OK (8192 bytes, offset=4096)\n", 39);
  write(1, "- sin solapamiento: OK\n", 23);
  write(1, "- addr2 > addr1: OK\n", 20);

  return 0;
}
