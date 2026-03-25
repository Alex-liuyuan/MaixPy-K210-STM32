#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>

static void semihost_writec(char ch)
{
    __asm volatile(
        "mov r0, #0x03  \n"
        "mov r1, %0     \n"
        "bkpt 0xAB      \n"
        :
        : "r" (&ch)
        : "r0", "r1", "memory");
}

int _write(int file, const char *ptr, int len)
{
    if ((file != 1 && file != 2) || ptr == 0 || len < 0)
    {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < len; ++i)
    {
        semihost_writec(ptr[i]);
    }
    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

int _close(int file)
{
    (void)file;
    return 0;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    if (st == 0)
    {
        errno = EINVAL;
        return -1;
    }

    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    return (file == 0 || file == 1 || file == 2) ? 1 : 0;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

void _exit(int status)
{
    (void)status;
    while (1) {}
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}
