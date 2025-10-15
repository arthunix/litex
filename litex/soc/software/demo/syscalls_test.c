#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>

/* Simple busy loop for visible time differences */
static void delay_loop(volatile unsigned long loops)
{
    while (loops--)
        ;
}

/* Test time-related syscalls (integer only) */
static void test_time_functions(void)
{
    struct timeval tv1, tv2;
    struct tms tms_buf;
    clock_t c1, c2;

    printf("\n=== Time Syscalls Test (integer) ===\n");

    /* gettimeofday */
    gettimeofday(&tv1, NULL);
    delay_loop(5000000);
    gettimeofday(&tv2, NULL);

    long delta_us = (tv2.tv_sec - tv1.tv_sec) * 1000000L +
                    (tv2.tv_usec - tv1.tv_usec);
    long delta_s  = delta_us / 1000000L;

    printf("gettimeofday Δ = %ld s\n", delta_s);

    /* times */
    c1 = times(&tms_buf);
    delay_loop(5000000);
    c2 = times(&tms_buf);
    printf("times Δticks = %ld\n", (long)(c2 - c1));

    /* clock */
    c1 = clock();
    delay_loop(5000000);
    c2 = clock();
    long delta_ticks = (long)(c2 - c1);
    printf("clock Δticks = %ld (per sec: %ld)\n", delta_ticks, (long)CLOCKS_PER_SEC);

    printf("Time functions OK!\n");
}

/* Test dynamic memory allocation */
static void test_memory_functions(void)
{
    printf("\n=== Memory Allocation Test ===\n");

    size_t size = 1024 * 1024;  /* 1 MB */
    printf("Allocating %lu bytes...\n", (unsigned long)size);

    void *ptr = malloc(size);
    if (!ptr) {
        printf("malloc FAILED!\n");
        return;
    }
    printf("malloc OK at %p\n", ptr);

    /* Write a pattern */
    memset(ptr, 0xAA, size);
    printf("Memory filled with 0xAA\n");

    /* Reallocate to larger size */
    size_t new_size = size;  /* 1.5 MB */
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        printf("realloc FAILED!\n");
        free(ptr);
        return;
    }
    printf("realloc OK (new size %lu, new ptr %p)\n",
           (unsigned long)new_size, new_ptr);

    /* Verify contents still intact */
    unsigned char *p = (unsigned char *)new_ptr;
    int ok = 1;
    for (size_t i = 0; i < 1024; i++) {
        if (p[i] != 0xAA) {
            ok = 0;
            break;
        }
    }
    printf("Memory verify: %s\n", ok ? "OK" : "CORRUPTED");

    /* calloc test */
    void *zero_ptr = calloc(256, 1024);  /* 256 KB */
    if (!zero_ptr)
        printf("calloc FAILED!\n");
    else {
        unsigned char *z = (unsigned char *)zero_ptr;
        ok = 1;
        for (size_t i = 0; i < 16; i++) {
            if (z[i] != 0) { ok = 0; break; }
        }
        printf("calloc verify: %s\n", ok ? "OK" : "CORRUPTED");
        free(zero_ptr);
    }

    /* Free everything */
    free(new_ptr);
    printf("free OK — all allocations released.\n");
}

int main(void)
{
    test_time_functions();
    test_memory_functions();
    return 0;
}