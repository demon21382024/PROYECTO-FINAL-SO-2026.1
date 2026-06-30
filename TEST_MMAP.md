# Test de la Syscall `mmap` en Selfie

## Descripción

Este documento explica el archivo [`test_mmap.c`](./test_mmap.c), que es una suite de pruebas funcional y de robustez para la syscall `mmap` implementada en `selfie.c`.

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

## Qué evalúa la suite de pruebas (Pruebas de Robustez)

La suite de pruebas evalúa de manera rigurosa los límites, casos de éxito y de error de la syscall:

### 1. Mapeo estándar de éxito (`TEST 1`)
* **Llamada**: `mmap(0, 4096, 0, fd, 0)`
* **Objetivo**: Verificar que se asigne una dirección virtual libre (`addr != 0`) y no se retorne error (`addr != 4294967295`).

### 2. Detección de `offset` desalineado (`TEST 2`)
* **Llamada**: `mmap(0, 4096, 0, fd, 123)`
* **Objetivo**: Asegurar que si el `offset` no es múltiplo de `PAGESIZE` (4096 bytes), la syscall falle devolviendo `-1` (`4294967295` tras encoger a 32 bits en Mipster).

### 3. Validación de `length` igual a cero (`TEST 3`)
* **Llamada**: `mmap(0, 0, 0, fd, 0)`
* **Objetivo**: Validar que no se permitan mappings de tamaño cero, retornando error `-1` (`4294967295`).

### 4. Redondeo automático de `length` (`TEST 4`)
* **Llamada**: `mmap(0, 1, 0, fd, 0)`
* **Objetivo**: Probar que una longitud pequeña (1 byte) sea redondeada internamente al múltiplo superior de página (4096 bytes). Para verificarlo, una llamada subsiguiente a `mmap` no debe solaparse con este espacio de 4096 bytes.

### 5. Manejo robusto de `fd` inválido (`TEST 5`)
* **Llamada**: `mmap(0, 4096, 0, 4294967295, 0)` (usando `-1` o `fd` erróneo)
* **Objetivo**: Verificar que el emulador sea robusto y registre la asociación virtual de manera regular sin abortar ni arrojar fallos de segmentación.

### 6. Control de no solapamiento (`TEST 6`)
* **Objetivo**: Compara todas las direcciones virtuales mapeadas de forma consecutiva (`addr1`, `addr_align`, `addr_next`, `addr2`) para certificar que el algoritmo de asignación de direcciones virtuales evita colisiones.

---

## Resultado esperado (salida exitosa)

```text
./selfie: 64-bit mipster executing 64-bit RISC-U binary test_mmap.c with 128MB physical memory
./selfie: >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

--- INICIANDO PRUEBAS DE ROBUSTEZ DE MMAP ---
TEST 1 PASS: Mapeo basico exitoso
./selfie: mmap offset 123 is not page-aligned
TEST 2 PASS: offset no alineado detectado correctamente y retorna -1
./selfie: mmap length must be greater than zero
TEST 3 PASS: longitud cero detectada correctamente y retorna -1
TEST 4 PASS: Redondeo de longitud no alineada a PAGESIZE funciona correctamente
TEST 5 PASS: mmap con fd invalido maneja la peticion con robustez
TEST 6 PASS: Todos los mappings contiguos no se solapan

--- TODAS LAS PRUEBAS DE ROBUSTEZ PASARON CON EXITO ---

./selfie: <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
./selfie: 64-bit mipster terminating 64-bit RISC-U binary test_mmap.c with exit code 0
```

**Indicadores de éxito:**
- ✅ `exit code 0` → sin errores en el proceso de prueba
- ✅ Mensajes explícitos `TEST X PASS` en cada caso crítico de robustez
- ✅ Mensaje final `TODAS LAS PRUEBAS DE ROBUSTEZ PASARON CON EXITO`
- ✅ Sin fallos de segmentación en el emulador de Selfie

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
- **`offset`** → debe ser múltiplo de `PAGESIZE`, si no retorna `-1` (encogido a 32 bits en la syscall)
- **No escribe en el archivo** → solo registra la relación virtual/física en el contexto
- **PC se incrementa** → `set_pc(context, get_pc(context) + INSTRUCTIONSIZE)` al finalizar

