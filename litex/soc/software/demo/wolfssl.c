/* main.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <wolfssl/user_settings.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfcrypt/benchmark/benchmark.h>
#include <wolfcrypt/test/test.h>

/* wolfCrypt_Init/wolfCrypt_Cleanup */
#include <wolfssl/wolfcrypt/wc_port.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* RNG CODE */
/* TODO: Implement real RNG */
static unsigned int gCounter;
unsigned int hw_rand(void)
{
    /* #warning Must implement your own random source */

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

int main(void)
{
    int ret;
    long clk_Hz = 50000000; /* default */

    printf("Actual Clock %dMHz\n", (int)(clk_Hz/1000000));

    if ((ret = wolfCrypt_Init()) != 0) {
        printf("wolfCrypt_Init failed %d\n", ret);
        return -1;
    }

#ifndef NO_CRYPT_TEST
    printf("\nwolfCrypt Test Started\n");
    wolfcrypt_test(NULL);
    printf("\nwolfCrypt Test Completed\n");
#endif

#ifndef NO_CRYPT_BENCHMARK
    printf("\nBenchmark Test Started\n");
    benchmark_test(NULL);
    printf("\nBenchmark Test Completed\n");
#endif

    if ((ret = wolfCrypt_Cleanup()) != 0) {
        printf("wolfCrypt_Cleanup failed %d\n", ret);
        return -1;
    }
    return 0;
}
