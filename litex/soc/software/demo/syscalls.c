#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <stdint.h>

#ifndef CPU_CLOCK_HZ
#define CPU_CLOCK_HZ 1000000UL  /* adjust or include csr.h */
#endif

#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define rdtime() read_csr(time)
#define rdcycle() read_csr(cycle)
#define rdinstret() read_csr(instret)

/* gettimeofday(): real elapsed time since boot */
int gettimeofday(struct timeval *tv, void *tz)
{
    if (!tv)
        return -1;

    uint64_t cycles = rdcycle();
    uint64_t usec = (cycles * 1000000ULL) / CPU_CLOCK_HZ;

    tv->tv_sec  = usec / 1000000ULL;
    tv->tv_usec = usec % 1000000ULL;
    return 0;
}

/* clock_gettime(): high-resolution version */
int clock_gettime(int clk_id, struct timespec *tp)
{
    if (!tp)
        return -1;

    uint64_t cycles = rdcycle();
    uint64_t nsec = (cycles * 1000000000ULL) / CPU_CLOCK_HZ;

    tp->tv_sec  = nsec / 1000000000ULL;
    tp->tv_nsec = nsec % 1000000000ULL;
    return 0;
}

/* times(): CPU time since boot */
clock_t times(struct tms *buf)
{
    uint64_t ticks = (clock_t)(rdcycle() / CPU_CLOCK_HZ);

    if (buf) {
        buf->tms_utime  = ticks;
        buf->tms_stime  = 0;
        buf->tms_cutime = 0;
        buf->tms_cstime = 0;
    }
    return ticks;
}
