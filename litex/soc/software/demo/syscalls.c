#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>

#ifndef CPU_CLOCK_HZ
#define CPU_CLOCK_HZ 1000000UL
#endif

#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define rdtime() read_csr(time)
#define rdcycle() read_csr(cycle)
#define rdinstret() read_csr(instret)

/* times(): CPU time since boot */
clock_t times(struct tms *buf) {

    clock_t ticks = (clock_t)read_csr(cycle);

    if (buf != NULL) {
        buf->tms_utime  = ticks;
        buf->tms_stime  = 0;
        buf->tms_cutime = 0;
        buf->tms_cstime = 0;
    }

    return ticks;
}

/* gettimeofday(): real elapsed time since boot */
int gettimeofday(struct timeval *tv, void *tz)
{
    if (!tv) {
        errno = EINVAL;
        return -1;
    }

    uint64_t cycles = read_csr(cycle);
    uint64_t usec = (cycles * 1000000ULL) / CPU_CLOCK_HZ;

    tv->tv_sec  = usec / 1000000ULL;
    tv->tv_usec = usec % 1000000ULL;
    return 0;
}