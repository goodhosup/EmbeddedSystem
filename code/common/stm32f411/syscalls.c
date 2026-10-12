/*
 * newlib(nano)용 최소 시스템 호출 (14장)
 *
 * printf 같은 표준 라이브러리 함수는 결국 _write()를 부른다.
 * 여기서 _write()가 __io_putchar()를 부르게 해 두고, 각 예제가 __io_putchar()를
 * UART로 보내는 함수로 만든다.
 *
 * 공통 Makefile에서 USE_NEWLIB=1일 때만 이 파일을 링크한다.
 */
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>

extern uint32_t end;        /* 링커 스크립트: .bss의 끝 = 힙의 시작 */
extern uint32_t _estack;    /* 스택의 꼭대기 */

#define STACK_RESERVE 1024u /* 힙이 스택에 이만큼은 남겨 둔다 */

__attribute__((weak)) int __io_putchar(int ch)
{
    (void)ch;
    return ch;
}

__attribute__((weak)) int __io_getchar(void)
{
    return -1;
}

int _write(int fd, const char *buf, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) {
        __io_putchar(buf[i]);
    }
    return len;
}

int _read(int fd, char *buf, int len)
{
    (void)fd;
    int n = 0;
    while (n < len) {
        int c = __io_getchar();
        if (c < 0) {
            break;
        }
        buf[n++] = (char)c;
    }
    return n;
}

void *_sbrk(int incr)
{
    static uint8_t *heap_end;
    if (heap_end == 0) {
        heap_end = (uint8_t *)&end;
    }
    uint8_t *prev = heap_end;
    if ((uintptr_t)(heap_end + incr) > (uintptr_t)&_estack - STACK_RESERVE) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_end += incr;
    return prev;
}

int _close(int fd)               { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd)              { (void)fd; return 1; }
int _lseek(int fd, int ptr, int dir) { (void)fd; (void)ptr; (void)dir; return 0; }
int _getpid(void)                { return 1; }
int _kill(int pid, int sig)      { (void)pid; (void)sig; errno = EINVAL; return -1; }
void _exit(int status)           { (void)status; for (;;) { } }
