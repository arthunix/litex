#include <stdio.h>
#include <stdint.h>

static inline uint32_t simd_add(uint32_t a, uint32_t b) {
    uint32_t r;
    asm volatile(
        ".insn r 0x33, 0x0, 0x03, %0, %1, %2"
        : "=r"(r)
        : "r"(a), "r"(b)
    );
    return r;
}

uint32_t simd_add_reference(uint32_t a, uint32_t b) {
    uint32_t r = 0;
    r |= ((a & 0x000000FF) + (b & 0x000000FF)) & 0xFF;
    r |= (((a & 0x0000FF00) + (b & 0x0000FF00)) & 0xFF00);
    r |= (((a & 0x00FF0000) + (b & 0x00FF0000)) & 0xFF0000);
    r |= (((a & 0xFF000000) + (b & 0xFF000000)) & 0xFF000000);
    return r;
}

void test(uint32_t a, uint32_t b) {
    uint32_t hw = simd_add(a, b);
    uint32_t sw = simd_add_reference(a, b);

    printf("Running SimdAdd on: A = 0x%08X, B = 0x%08X\n", (unsigned)a, (unsigned)b);

    if(hw == sw) {
        printf("simd_add test successful!! -> Result = 0x%08X, Expected = 0x%08X\n\n", (unsigned)hw, (unsigned)sw);
    } else {
        printf("simd_add test error!! -> Result = 0x%08X, Expected = 0x%08X\n\n", (unsigned)hw, (unsigned)sw);
    }
}

void simd_add_test(void);
void simd_add_test(void) {
    printf("--== Custom SimdAdd Instruction VexRiscv Plugin ==--\n\n");

    test(0x11223344, 0x01010101);
    test(0xAABBCCDD, 0x11223344);
    test(0xFFFFFFFF, 0x01010101);
    test(0x00000000, 0xFF00FF00);
}
