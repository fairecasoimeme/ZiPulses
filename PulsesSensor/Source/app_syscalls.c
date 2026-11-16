/*****************************************************************************
 *
 * app_syscalls.c
 *
 * System calls required by the C library (newlib) and missing on this
 * microcontroller. Same behaviour as libnosys (failure, ENOSYS), without the
 * linker warnings "is not implemented and will always fail".
 *
 ****************************************************************************/
#include <errno.h>
#include <sys/types.h>

#undef errno
extern int errno;

int _close(int file)
{
    (void)file;
    errno = ENOSYS;
    return -1;
}

int _getpid(void)
{
    errno = ENOSYS;
    return -1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

off_t _lseek(int file, off_t offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    errno = ENOSYS;
    return -1;
}
