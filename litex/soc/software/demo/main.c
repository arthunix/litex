// This file is Copyright (c) 2020 Florent Kermarrec <florent@enjoy-digital.fr>
// License: BSD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/time.h>
#include <sys/times.h>

#include <irq.h>
#include <libbase/uart.h>
#include <libbase/console.h>
#include <generated/csr.h>

/*-----------------------------------------------------------------------*/
/* Uart                                                                  */
/*-----------------------------------------------------------------------*/

static char *readstr(void)
{
	char c[2];
	static char s[64];
	static int ptr = 0;

	if(readchar_nonblock()) {
		c[0] = getchar();
		c[1] = 0;
		switch(c[0]) {
			case 0x7f:
			case 0x08:
				if(ptr > 0) {
					ptr--;
					fputs("\x08 \x08", stdout);
				}
				break;
			case 0x07:
				break;
			case '\r':
			case '\n':
				s[ptr] = 0x00;
				fputs("\n", stdout);
				ptr = 0;
				return s;
			default:
				if(ptr >= (sizeof(s) - 1))
					break;
				fputs(c, stdout);
				s[ptr] = c[0];
				ptr++;
				break;
		}
	}

	return NULL;
}

static char *get_token(char **str)
{
	char *c, *d;

	c = (char *)strchr(*str, ' ');
	if(c == NULL) {
		d = *str;
		*str = *str+strlen(*str);
		return d;
	}
	*c = 0;
	d = *str;
	*str = c+1;
	return d;
}

static void prompt(void)
{
	printf("\e[92;1mlitex-demo-app\e[0m> ");
}

/*-----------------------------------------------------------------------*/
/* Help                                                                  */
/*-----------------------------------------------------------------------*/

static void help(void)
{
	puts("\nLiteX minimal demo app built "__DATE__" "__TIME__"\n");
	puts("Available commands:");
	puts("help                 - Show this command");
	puts("reboot               - Reboot CPU");
#ifdef CSR_LEDS_BASE
	puts("led                  - Led demo");
#endif
	puts("donut                - Spinning Donut demo");
	puts("helloc               - Hello C");
#ifdef WITH_CXX
	puts("hellocpp             - Hello C++");
#endif
	puts("test_time_syscalls   - Run Test for Time Syscalls, times, gettimeofday, current_time");
	puts("test_memory_syscalls - Test Dynamic Memory Allocation: malloc, calloc, realloc and free");
	puts("wolfssl_test         - Run Wolfssl Tests");
	puts("wolfssl_benchmark    - Run Wolfssl Benchmarks");
}

/*-----------------------------------------------------------------------*/
/* Commands                                                              */
/*-----------------------------------------------------------------------*/

static void reboot_cmd(void)
{
	ctrl_reset_write(1);
}

#ifdef CSR_LEDS_BASE
static void led_cmd(void)
{
	int i;
	printf("Led demo...\n");

	printf("Counter mode...\n");
	for(i=0; i<32; i++) {
		leds_out_write(i);
		busy_wait(100);
	}

	printf("Shift mode...\n");
	for(i=0; i<4; i++) {
		leds_out_write(1<<i);
		busy_wait(200);
	}
	for(i=0; i<4; i++) {
		leds_out_write(1<<(3-i));
		busy_wait(200);
	}

	printf("Dance mode...\n");
	for(i=0; i<4; i++) {
		leds_out_write(0x55);
		busy_wait(200);
		leds_out_write(0xaa);
		busy_wait(200);
	}
}
#endif

extern void donut(void);

static void donut_cmd(void)
{
	printf("Donut demo...\n");
	donut();
}

extern void helloc(void);

static void helloc_cmd(void)
{
	printf("Hello C demo...\n");
	helloc();
}

#ifdef WITH_CXX
extern void hellocpp(void);

static void hellocpp_cmd(void)
{
	printf("Hello C++ demo...\n");
	hellocpp();
}
#endif

/*-----------------------------------------------------------------------*/
/* Time Syscalls Test                                                    */
/*-----------------------------------------------------------------------*/

static void delay_loop(volatile unsigned long loops)
{
    while (loops--)
        ;
}

#define DELAY_LOOP_IT 100000000

#ifndef CPU_CLOCK_HZ
#define CPU_CLOCK_HZ 50000000UL
#endif

static void test_time_syscalls(void)
{
    struct timeval tv1, tv2;
    struct tms tms_buf;
    clock_t c1, c2;

	printf("\n--======== Time Syscalls Test! =========--\n");
    printf("\nRunning Time Syscalls Test with freq.: %ld and: %ld of iterations.\n", (long)CPU_CLOCK_HZ, (long)DELAY_LOOP_IT);

    gettimeofday(&tv1, NULL);
    delay_loop(DELAY_LOOP_IT);
    gettimeofday(&tv2, NULL);

    long delta_us = (tv2.tv_sec - tv1.tv_sec) * 1000000L + (tv2.tv_usec - tv1.tv_usec);
    long delta_s  = delta_us / 1000000L;

    printf("gettimeofday Δ = %ld s\n", delta_s);

    c1 = times(&tms_buf);
    delay_loop(DELAY_LOOP_IT);
    c2 = times(&tms_buf);

    printf("times Δ cpu ticks = %ld\n", (long)(c2 - c1));

    c1 = clock();
    delay_loop(DELAY_LOOP_IT);
    c2 = clock();

    printf("clock Δ cpu ticks = %ld (per sec: %ld)\n", (long)(c2 - c1), (long)CLOCKS_PER_SEC);

    printf("Time functions OK!\n");
}

/*-----------------------------------------------------------------------*/
/* Dynamic Allocation Test                                               */
/*-----------------------------------------------------------------------*/

static void test_memory_syscalls(void)
{
	size_t buffer_size = 512;  /* 1 MB */

	printf("\n--======== Dynamic Memory Test! ========--\n");
    printf("Allocating buffer of %lu bytes...\n", (unsigned long)buffer_size);

    void *ptr = malloc(buffer_size);
    if (!ptr) {
        printf("malloc FAILED!\n");
        return;
    } else {
		printf("malloc OK at %p\n", ptr);/* Write a pattern */
		memset(ptr, 0xAA, buffer_size);
		printf("Memory filled with 0xAA\n");
	}

    size_t new_buffer_size = buffer_size*2;  /* 1.5 MB */
    void *new_ptr = realloc(ptr, new_buffer_size);
    if (!new_ptr) {
        printf("realloc FAILED!\n");
        free(ptr);
        return;
    } else {
		printf("realloc OK (new size %lu, new ptr %p)\n", (unsigned long)new_buffer_size, new_ptr);
	}

    unsigned char *p = (unsigned char *)new_ptr;
    int ok = 1;
    for (size_t i = 0; i < buffer_size; i++) {
        if (p[i] != 0xAA) {
            ok = 0;
            break;
        }
    }
    printf("Memory verify: %s\n", ok ? "OK" : "CORRUPTED");

    void *zero_ptr = calloc(1, new_buffer_size);
    if (!zero_ptr)
        printf("calloc FAILED!\n");
    else {
        unsigned char *z = (unsigned char *)zero_ptr;
        ok = 1;
        for (size_t i = 0; i < 16; i++) {
            if (z[i] != 0) { ok = 0; break; }
        }

		if (!ok) {
			free(ptr);
			return;
		} else {
			printf("calloc OK (size %lu, ptr %p)\n", (unsigned long)new_buffer_size, zero_ptr);
		}

		printf("calloc verify: %s\n", ok ? "OK" : "CORRUPTED");

        free(zero_ptr);
    }

    /* Free everything */
    free(new_ptr);
    printf("free OK — all allocations released.\n");
}

/*-----------------------------------------------------------------------*/
/* Wolfssl Tests / Benchmarks                                            */
/*-----------------------------------------------------------------------*/

#include <wolfssl/user_settings.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfcrypt/benchmark/benchmark.h>
#include <wolfcrypt/test/test.h>
#include <wolfssl/wolfcrypt/wc_port.h>

static unsigned int gCounter;
unsigned int hw_rand(void)
{
    return ++gCounter;
}

unsigned int my_rng_seed_gen(void)
{
    return hw_rand();
}

int my_rng_gen_block(unsigned char* output, unsigned int sz)
{
    uint32_t i = 0;
    uint32_t randReturnSize = sizeof(CUSTOM_RAND_TYPE);

    while (i < sz)
    {
        /* If not aligned or there is odd/remainder */
        if((i + randReturnSize) > sz ||
            ((uint32_t)&output[i] % randReturnSize) != 0 ) {
            /* Single byte at a time */
            output[i++] = (unsigned char)my_rng_seed_gen();
        }
        else {
            /* Use native 8, 16, 32 or 64 copy instruction */
            *((CUSTOM_RAND_TYPE*)&output[i]) = my_rng_seed_gen();
            i += randReturnSize;
        }
    }

    return 0;
}

int run_wolfssl_tests(void)
{
	int ret;
	printf("\n--======== Wolfssl Tests ========--\n");

	printf("Running Wolfssl Init...\n");
    if ((ret = wolfCrypt_Init()) != 0) {
        printf("wolfCrypt_Init failed %d\n", ret);
        return -1;
    }

    printf("\nwolfCrypt Test Started\n");
    wolfcrypt_test(NULL);
    printf("\nwolfCrypt Test Completed\n");

	printf("Running Wolfssl Cleanup...\n");
    if ((ret = wolfCrypt_Cleanup()) != 0) {
        printf("wolfCrypt_Cleanup failed %d\n", ret);
        return -1;
    }
    return 0;
}

int run_wolfssl_benchmark(void)
{
	int ret;
	printf("\n--======== Wolfssl Benchmark ========--\n");

	printf("Running Wolfssl Init...\n");
    if ((ret = wolfCrypt_Init()) != 0) {
        printf("wolfCrypt_Init failed %d\n", ret);
        return -1;
    }

	printf("\nBenchmark Test Started\n");
    benchmark_test(NULL);
    printf("\nBenchmark Test Completed\n");

	printf("Running Wolfssl Cleanup...\n");
    if ((ret = wolfCrypt_Cleanup()) != 0) {
        printf("wolfCrypt_Cleanup failed %d\n", ret);
        return -1;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Console service / Main                                                */
/*-----------------------------------------------------------------------*/

static void console_service(void)
{
	char *str;
	char *token;

	str = readstr();
	if(str == NULL) return;
	token = get_token(&str);
	if(strcmp(token, "help") == 0)
		help();
	else if(strcmp(token, "reboot") == 0)
		reboot_cmd();
#ifdef CSR_LEDS_BASE
	else if(strcmp(token, "led") == 0)
		led_cmd();
#endif
	else if(strcmp(token, "donut") == 0)
		donut_cmd();
	else if(strcmp(token, "helloc") == 0)
		helloc_cmd();
#ifdef WITH_CXX
	else if(strcmp(token, "hellocpp") == 0)
		hellocpp_cmd();
#endif
	else if(strcmp(token, "test_time_syscalls") == 0)
		test_time_syscalls();
	else if(strcmp(token, "test_memory_syscalls") == 0)
		test_memory_syscalls();
	else if(strcmp(token, "wolfssl_test") == 0)
		run_wolfssl_tests();
	else if(strcmp(token, "wolfssl_benchmark") == 0)
		run_wolfssl_benchmark();
	prompt();
}

int main(void)
{
#ifdef CONFIG_CPU_HAS_INTERRUPT
	irq_setmask(0);
	irq_setie(1);
#endif
	uart_init();

	help();
	prompt();

	while(1) {
		console_service();
	}

	return 0;
}
