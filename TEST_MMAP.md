# Test de la Syscall `mmap` en Selfie

## Descripción

Este documento explica el archivo [`test_mmap.c`](./test_mmap.c), que es un programa de prueba funcional para la nueva syscall `mmap` implementada en `selfie.c`.

El programa está escrito en **C*** (el subconjunto de C que entiende el compilador `starc` de Selfie) y se ejecuta dentro del emulador `mipster`.

---

## Cómo ejecutar el test

Desde WSL, en el directorio raíz del proyecto:

```bash
# 1. Compilar selfie (si no está compilado)
make selfie

# 2. Ejecutar el test de mmap con 128MB de memoria virtual
./selfie -c test_mmap.c -m 128
```

---

## Anatomía del comando

| Parte | Significado |
|-------|-------------|
| `./selfie` | Binario nativo compilado con `make` |
| `-c test_mmap.c` | Compila `test_mmap.c` con el compilador integrado `starc` a RISC-U |
| `-m 128` | Ejecuta el binario en `mipster` con **128 MB** de memoria física emulada |

El resultado del programa aparece delimitado por:
- `>>>>>>>>` → inicio de la ejecución del proceso
- `<<<<<<<<` → fin de la ejecución del proceso

---

## Qué prueba el test

### Escenario 1 — Primer `mmap` con dirección automática

```c
addr1 = mmap(0, 4096, 0, fd, 0);
```

| Argumento | Valor | Significado |
|-----------|-------|-------------|
| `addr` | `0` | Selfie elige automáticamente la dirección virtual |
| `length` | `4096` | 1 página = `PAGESIZE` (alineación automática) |
| `prot` | `0` | Solo lectura |
| `fd` | `fd` | File descriptor de `selfie.c` (abierto con `open`) |
| `offset` | `0` | Desde el inicio del archivo |

**Verificaciones:**
- `addr1 != 0` → la dirección no es nula
- `addr1 != -1` → no hubo error

---

### Escenario 2 — Segundo `mmap` con offset distinto

```c
addr2 = mmap(0, 8192, 0, fd, 4096);
```

| Argumento | Valor | Significado |
|-----------|-------|-------------|
| `addr` | `0` | Auto-asignado |
| `length` | `8192` | 2 páginas (alineado a `PAGESIZE`) |
| `prot` | `0` | Solo lectura |
| `fd` | `fd` | Mismo file descriptor |
| `offset` | `4096` | Desde la segunda página del archivo |

**Verificaciones:**
- `addr2 != 0` y `addr2 != -1`
- `addr1 != addr2` → las regiones son distintas
- `addr2 >= addr1 + 4096` → no hay solapamiento de memoria virtual

---

## Resultado esperado (salida exitosa)

```
./selfie: 64-bit mipster executing 64-bit RISC-U binary test_mmap.c with 128MB physical memory
./selfie: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

mmap TEST EXITOSO!
- mmap 1: OK (4096 bytes, offset=0)
- mmap 2: OK (8192 bytes, offset=4096)
- sin solapamiento: OK
- addr2 > addr1: OK

./selfie: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
./selfie: 64-bit mipster terminating 64-bit RISC-U binary test_mmap.c with exit code 0
```

**Indicadores de éxito:**
- ✅ `exit code 0` → sin errores
- ✅ Sin `segmentation fault` ni `uncaught exception`
- ✅ Los 4 mensajes de `OK` aparecen dentro del bloque `>>>...<<<`
- ✅ El resumen de memoria muestra estabilidad (≈ 0.02MB mapeado)

---

## Implementación interna de `mmap`

La syscall implementada en `selfie.c` sigue el patrón estándar:

| Componente | Función | Propósito |
|------------|---------|-----------|
| `SYSCALL_MMAP = 222` | Constante | ID estándar Linux RISC-V |
| `emit_mmap()` | Compilador | Genera código RISC-U para cargar 5 args y llamar `ecall` |
| `implement_mmap(context)` | Emulador | Lógica: valida args, asigna VA, registra mapping |
| `get_mappings / set_mappings` | Contexto | Lista enlazada de mappings (índice 38) |

### Reglas aplicadas (según spec del proyecto)

- **`addr == 0`** → se elige automáticamente desde `program_break`, evitando solapamientos con mappings existentes
- **`length`** → redondeado al múltiplo de `PAGESIZE (4096)` más cercano
- **`offset`** → debe ser múltiplo de `PAGESIZE`, si no retorna `-1`
- **No escribe en el archivo** → solo registra la relación virtual/física en el contexto
- **PC se incrementa** → `set_pc(context, get_pc(context) + INSTRUCTIONSIZE)` al finalizar

---

## Verificación del resumen de ejecución

Al final de la ejecución, `mipster` reporta:

```
./selfie: summary: 243 executed instructions in total
./selfie:          0.02MB mapped memory [0.01% of 128MB physical memory]
./selfie:          11 exceptions handled (9 syscalls, 1 page fault, 0 timers)
```

- **`0.02MB` de memoria mapeada** → el page cache opera de forma estable, sin fugas
- **`8 syscalls`** → `open` + 2×`mmap` + 5×`write` = correctas
- **`exit code 0`** → ejecución completamente exitosa
