# Explicación Técnica: Implementación de la Syscall `mmap` en Selfie

Este documento detalla **qué** se ha implementado, **cómo** se hizo y el **porqué** técnico de cada decisión de diseño tomada para integrar la llamada al sistema `mmap` en el compilador, emulador y núcleo del proyecto **Selfie**.

---

## 1. Contexto Teórico y Objetivos

`mmap` es una llamada al sistema crucial en los sistemas operativos modernos. Permite asociar un archivo o un recurso del sistema con un rango de direcciones de memoria virtual del proceso. 

En Selfie, la meta era crear una base robusta para esta syscall bajo la arquitectura **RISC-V de 64 bits** (RISC-U en Selfie). Esto requirió:
1. Enseñar al compilador (`starc`) a traducir y emitir instrucciones de llamada para `mmap` con sus 5 parámetros.
2. Adaptar la máquina virtual/emulador (`mipster`) para interceptar la llamada, validar los argumentos, calcular el direccionamiento virtual libre y registrar el mapeo.

---

## 2. Fase 1: Registro y Cableado de la Syscall

### 2.1. El identificador de la Syscall: `SYSCALL_MMAP = 222`
* **Por qué 222**: De acuerdo con el estándar del ABI de llamadas al sistema de **Linux para RISC-V de 64 bits**, la syscall `mmap` tiene el ID `222`. Utilizar este número mantiene la fidelidad arquitectónica de Selfie con RISC-V real.

### 2.2. Modificación de la estructura del Contexto del Proceso
Cada proceso emulado en Selfie mantiene sus datos de control en un arreglo dinámico apuntado por un puntero de contexto (`uint64_t* context`). Originalmente, la constante `CONTEXTENTRIES` definía un tamaño de 38 elementos.
* **Qué se hizo**: Incrementamos `CONTEXTENTRIES` de `38` a `39`.
* **Por qué**: Necesitábamos almacenar la lista enlazada de mapeos activos (`mappings`) creados mediante `mmap` en el propio contexto del proceso. Al asignarlo al índice `38`, cada proceso mantiene aislados sus propios recursos de mapeo virtual.
* **Accesores**: Definimos `get_mappings` y `set_mappings` para manipular de forma limpia este campo usando aritmética de punteros en `selfie.c`:
  ```c
  uint64_t  mappings_offset(uint64_t* context)              { return (uint64_t) (context + 38); }
  uint64_t* get_mappings(uint64_t* context)                 { return (uint64_t*) *(context + 38); }
  void      set_mappings(uint64_t* context, uint64_t* m)    { *(context + 38) = (uint64_t) m; }
  ```

---

## 3. Fase 2: Wrapper del Compilador (`emit_mmap`)

El compilador integrado de Selfie (`starc`) traduce funciones escritas en lenguaje C* a instrucciones RISC-V. Cuando el código C* invoca a `mmap(...)`, el compilador debe generar el código ensamblador correspondiente.

### 3.1. Carga de Argumentos de la Pila a Registros
* **Qué se hizo**: En `emit_mmap()`, se cargan secuencialmente los 5 parámetros de la syscall desde la pila (`SP`) hacia los registros del procesador.
  * `a0` $\leftarrow$ `addr`
  * `a1` $\leftarrow$ `length`
  * `a2` $\leftarrow$ `prot`
  * `a3` $\leftarrow$ `fd`
  * `a4` $\leftarrow$ `offset`
* **Por qué**: La convención de llamadas de RISC-V establece que los primeros argumentos se pasan en los registros `a0` a `a7`. Para syscalls con 5 argumentos, usamos hasta `a4`. 
* **Emisión de la Syscall**: Se carga el identificador `222` en el registro `a7` (indicando al emulador cuál syscall ejecutar) y se emite la instrucción `ecall` (llamada al entorno de ejecución).

### 3.2. Adaptación de `do_ecall`
En el intérprete de Selfie, la función `do_ecall` lee los registros de argumentos del proceso para poder procesarlos en el microkernel.
* **Qué se hizo**: Añadimos soporte específico en `do_ecall` para que lea los registros `REG_A3` y `REG_A4` cuando la llamada solicitada es `SYSCALL_MMAP`.
* **Por qué**: Por defecto, Selfie solo lee hasta `REG_A2` para optimizar el rendimiento de la mayoría de sus syscalls nativas (que toman un máximo de 3 argumentos). De no haber hecho esto, los valores de `fd` (`a3`) y `offset` (`a4`) se leerían como basura o ceros.

---

## 4. Fase 3: Lógica y Robustez en el Emulador Mipster (`implement_mmap`)

Esta es la sección lógica más compleja, pues simula el comportamiento del sistema operativo en el hardware emulado.

```c
void implement_mmap(uint64_t* context) { ... }
```

### 4.1. Reglas de Validación de Parámetros
* **Validación de Offset**:
  ```c
  if (offset % PAGESIZE != 0) { ... }
  ```
  * **Por qué**: Físicamente, la unidad de manejo de memoria (MMU) gestiona el mapeo virtual-físico a nivel de páginas (`PAGESIZE` = 4096 bytes). Si un offset no estuviera alineado a página, una sola página virtual apuntaría a fragmentos cruzados de dos páginas físicas distintas, rompiendo la coherencia de la memoria. Si falla, retorna `-1`.
* **Validación de Longitud**:
  * **Por qué**: Mapear un rango de 0 bytes carece de sentido lógico e induce a errores de división o solapamientos silenciosos en el kernel. Retorna `-1` si es `0`.
  * **Redondeo**: El tamaño se redondea hacia arriba usando `round_up(length, PAGESIZE)`. Si pides 1 byte, el sistema asigna una página completa de 4096 bytes, pues es el bloque de grano mínimo que el hardware puede mapear.

### 4.2. Algoritmo de Dirección Virtual Libre (si `addr == 0`)
Cuando el usuario no especifica una dirección fija en memoria, el sistema operativo debe elegir una dirección libre y segura.
* **Qué se hizo**: Iniciamos la búsqueda desde el final del heap actual (`program_break` redondeado al múltiplo de página superior). 
* **Prevención de Solapamiento**: Se realiza un escaneo lineal iterativo a través de la lista enlazada de mappings del contexto. Si la dirección candidata colisiona con un mapping ya existente en el rango `[candidato, candidato + length)`, el candidato se desliza de inmediato al final del mapping conflictivo. Este proceso se repite hasta hallar un hueco limpio en memoria.
* **Por qué**: Garantiza que múltiples llamadas consecutivas a `mmap` (con `addr = 0`) no sobrescriban ni corrompan regiones asignadas previamente.

### 4.3. Registro del Mapping en el Contexto
* **Qué se hizo**: Creamos un bloque en memoria dinámica (usando `smalloc`) con la estructura de control:
  `[next_mapping_ptr, virtual_address, aligned_length, fd, offset, protection]`
  y lo insertamos a la cabeza de la lista enlazada del contexto.
* **Por qué**: En esta syscall básica, el objetivo no es cargar físicamente el archivo en memoria de forma inmediata (lectura ávida), sino establecer y registrar el contrato virtual-físico. Este registro es la base técnica para resolver futuros fallos de página (*page faults*) bajo demanda.

### 4.4. Incremento del Program Counter (PC)
* **Qué se hizo**: `set_pc(context, get_pc(context) + INSTRUCTIONSIZE);`
* **Por qué**: La instrucción `ecall` detiene la ejecución del CPU virtual y pasa el control al microkernel. Una vez que el microkernel termina de resolver la llamada al sistema, si no incrementáramos el `PC` del proceso por el tamaño de una instrucción (4 bytes), al reanudar el proceso este volvería a ejecutar exactamente la misma instrucción `ecall` de manera indefinida, creando un bucle infinito (*freeze*).

### 4.5. Retorno de Error: Truncamiento a 32 bits (`4294967295`)
* **Qué se hizo**: Para reportar error, se asigna `sign_shrink(-1, SYSCALL_BITWIDTH)` al registro `a0`.
* **Por qué**: Selfie emula un procesador de 64 bits pero restringe sus llamadas al sistema a interfaces de 32 bits mediante la constante `SYSCALL_BITWIDTH = 32`. Esto significa que cuando el emulador retorna `-1` (que en binario de 64 bits es `0xFFFFFFFFFFFFFFFF`), la función `sign_shrink` extrae solo los primeros 32 bits, resultando en `0xFFFFFFFF` (es decir, el entero sin signo `4294967295`). En el código C*, los fallos de llamada a sistema se deben verificar comparándolos con este valor exacto.

---

## 5. Explicación de la Suite de Pruebas de Robustez (`test_mmap.c`)

El programa [`test_mmap.c`](./test_mmap.c) no es una simple prueba feliz; está diseñado para estresar las validaciones implementadas en `selfie.c`:

1. **Prueba de Éxito Estándar**: Abre `selfie.c` y realiza un mapeo. Asegura la comunicación correcta de los 5 argumentos del compilador al emulador.
2. **Prueba de Offset Inválido**: Intenta mapear con un offset desalineado (`123`). Comprueba que el emulador rechace el offset y devuelva correctamente `4294967295` (el código de error `-1`).
3. **Prueba de Longitud Inválida**: Intenta mapear con `length = 0`. Comprueba que sea rechazado.
4. **Prueba de Alineamiento de Tamaño**: Mapea `1` byte. Valida que se asigne una página entera (4096 bytes) de tal modo que un mapping posterior automático no colisione con el resto de esa página.
5. **Prueba de File Descriptor Erróneo**: Mapea con un fd inválido (`-1`). Garantiza que el sistema operativo registre la asociación de forma tolerante a fallos sin generar excepciones ni caídas de sistema.
6. **Prueba de No Solapamiento**: Llama consecutivamente a `mmap` y asegura que los rangos de direcciones virtuales devueltos se desplacen y mantengan una separación clara.
