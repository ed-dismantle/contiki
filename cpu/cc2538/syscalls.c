#include <errno.h>
#include <stdio.h>
#include <debug-uart.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/stat.h>
#include <core/net/ip/uip.h>

uip_ipaddr_t uip_hostaddr;

register char * stack_ptr asm ("sp");

caddr_t
_sbrk(int incr) {
  extern char _heap;   /* Defined by the linker */
  extern char _eheap;  /* Defined by the linker */
  static char *allocated_end = &_heap;
  char *prev_allocated_end;

  prev_allocated_end = allocated_end;
  if (allocated_end + incr > &_eheap) {
    printf("Heap full (requested %d, available %d)\n", incr, (int)(&_eheap - allocated_end));
    errno = ENOMEM;
    return (caddr_t)-1;
  }

  allocated_end += incr;
  //printf("Allocated %d bytes at 0x%x. Heap remaining = 0x%x\n", incr, prev_allocated_end, &_eheap - allocated_end);
  return (caddr_t) prev_allocated_end;
}

void
init_mmap() {
  extern char _heap;
  extern char _eheap;
  //printf("Heap (start, end) = (0x%x, 0x%x). Size = %d\n", (uint32_t)&_heap, (uint32_t)&_eheap, (uint32_t)(&_eheap - &_heap));
}

/* Copy memory like memcpy, but no return value required.  */
void
__aeabi_memcpy(void *dest, const void *src, size_t n) {
  (void) memcpy(dest, src, n);
}

void
__aeabi_memset(void *dest, size_t n, int c) {
  (void) memset(dest, c, n);
}

int
_open(const char *name, int flags, int mode) {
  errno = ENOENT;
  return -1;
}

int
_close(int file)
{
  if (file == 1 || file == 2) {
    return 0;
  }
  errno = EBADF;
  return -1;
}

int
_isatty(int file)
{
  if (file >= 0 && file <= 2) return 1;
   return 0;
}

int
_read(int file, char *ptr, int len){
  return 0;
}

int
_write(int file, const char *ptr, int len){
  int sent = -1;
  if (file == 1 || file == 2) {
    sent = dbg_send_bytes((const unsigned char*)ptr, len);
  }
  return sent;
}

int
_lseek(int file, int ptr, int dir){
    return 0;
}

int
_fstat(int file, struct stat *st) {
  if (file >= 0 && file <= 2) {
    st->st_mode = S_IFCHR;
    return 0;
  }
  errno = EBADF;
  return -1;
}

int
_stat(char *file, struct stat *st) {
  errno = ENOENT;
  return -1;
}

int
fsync(int fd)
{
  if (fd == 1 || fd == 2) {
    return 0;
  }
  if (fd == 0) return 0;
  errno = EBADF;
  return -1;
}

void
exit(int status)
{
  while(1);
}

void
_exit(int status)
{
  while(1);
}

void
_abort()
{
  while(1);
}

void
_kill()
{
  while(1);
}

pid_t
_getpid(void)
{
  return 1;
}
