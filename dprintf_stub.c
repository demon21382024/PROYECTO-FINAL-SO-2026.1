/* dprintf_stub.c
   Stub de compatibilidad para dprintf en Windows/MinGW.
   dprintf() escribe a un file descriptor POSIX, que MinGW no soporta.
   Redirigimos fd=1 a stdout y cualquier otro a stderr.
*/
#include <stdio.h>
#include <stdarg.h>

int dprintf(int fd, const char* fmt, ...) {
  va_list args;
  int result;

  va_start(args, fmt);

  if (fd == 1)
    result = vfprintf(stdout, fmt, args);
  else
    result = vfprintf(stderr, fmt, args);

  va_end(args);

  return result;
}
