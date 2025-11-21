#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define ITERATIONS 100  // Number of loops for benchmarking
#define XLEN 32         // Target Architecture (RV32) / Data Width
//#define XLEN 64       // Target Architecture (RV64) / Data Width 

typedef uint32_t reg_t;
#define PRIreg PRIx32

static inline uint32_t get_cycles(void) {
    uint32_t cycles;
    asm volatile ("rdcycle %0" : "=r"(cycles));
    return cycles;
}

volatile reg_t g_src1 = 0x89ABCDEF;
volatile reg_t g_src2 = 0x0F0F0F0F;
volatile reg_t g_res  = 0;

// --- Zbkb (Bit-manipulation for Cryptography) ---

__attribute__((noinline)) reg_t emu_rol(reg_t rs1, reg_t rs2) {
    reg_t shamt = rs2 & (XLEN - 1);
    return (rs1 << shamt) | (rs1 >> (XLEN - shamt));
}

__attribute__((noinline)) reg_t emu_ror(reg_t rs1, reg_t rs2) {
    reg_t shamt = rs2 & (XLEN - 1);
    return (rs1 >> shamt) | (rs1 << (XLEN - shamt));
}

__attribute__((noinline)) reg_t emu_rori(reg_t rs1, reg_t imm) {
    reg_t shamt = imm & (XLEN - 1);
    return (rs1 >> shamt) | (rs1 << (XLEN - shamt));
}

__attribute__((noinline)) reg_t emu_andn(reg_t rs1, reg_t rs2) {
    return rs1 & (~rs2);
}

__attribute__((noinline)) reg_t emu_orn(reg_t rs1, reg_t rs2) {
    return rs1 | (~rs2);
}

__attribute__((noinline)) reg_t emu_xnor(reg_t rs1, reg_t rs2) {
    return ~(rs1 ^ rs2);
}

__attribute__((noinline)) reg_t emu_pack(reg_t rs1, reg_t rs2) {
    return (rs2 << 16) | (rs1 & 0xFFFF);
}

__attribute__((noinline)) reg_t emu_packh(reg_t rs1, reg_t rs2) {
    return ((rs2 & 0xFF) << 8) | (rs1 & 0xFF);
}

__attribute__((noinline)) reg_t emu_brev8(reg_t rs1) {
    reg_t x = rs1;
    x = ((x & 0x55555555) << 1) | ((x & 0xAAAAAAAA) >> 1);
    x = ((x & 0x33333333) << 2) | ((x & 0xCCCCCCCC) >> 2);
    x = ((x & 0x0F0F0F0F) << 4) | ((x & 0xF0F0F0F0) >> 4);
    return x;
}

__attribute__((noinline)) reg_t emu_rev8(reg_t rs1) {
    reg_t x = rs1;
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) |
           ((x & 0xFF0000) >> 8) | ((x >> 24) & 0xFF);
}

__attribute__((noinline)) reg_t emu_zip(reg_t rs1) {
    reg_t x = rs1;
    reg_t out = 0;
    for (int i = 0; i < 16; i++) {
        if ((x >> i) & 1) out |= (1U << (2 * i));
        if ((x >> (i + 16)) & 1) out |= (1U << (2 * i + 1));
    }
    return out;
}

__attribute__((noinline)) reg_t emu_unzip(reg_t rs1) {
    reg_t x = rs1;
    reg_t out = 0;
    for (int i = 0; i < 16; i++) {
        if ((x >> (2 * i)) & 1) out |= (1U << i);
        if ((x >> (2 * i + 1)) & 1) out |= (1U << (i + 16));
    }
    return out;
}

// --- Zbkc (Carry-less Multiply) ---

static uint64_t clmul64(uint32_t rs1, uint32_t rs2) {
    uint64_t output = 0;
    for (int i = 0; i < 32; i++) {
        if ((rs2 >> i) & 1) {
            output ^= ((uint64_t)rs1 << i);
        }
    }
    return output;
}

__attribute__((noinline)) reg_t emu_clmul(reg_t rs1, reg_t rs2) {
    return (reg_t)clmul64(rs1, rs2);
}

__attribute__((noinline)) reg_t emu_clmulh(reg_t rs1, reg_t rs2) {
    return (reg_t)(clmul64(rs1, rs2) >> 32);
}

// --- Zbkx (Crossbar Permutation) ---

__attribute__((noinline)) reg_t emu_xperm8(reg_t rs1, reg_t rs2) {
    // rs1 is indices, rs2 is table
    reg_t out = 0;
    for (int i = 0; i < 4; i++) { // 4 bytes in 32-bit word
        uint8_t idx = (rs1 >> (i * 8)) & 0xFF;
        if (idx < 4) { // Only indices 0-3 are valid for 32-bit reg
            uint8_t val = (rs2 >> (idx * 8)) & 0xFF;
            out |= ((uint32_t)val << (i * 8));
        }
    }
    return out;
}

__attribute__((noinline)) reg_t emu_xperm4(reg_t rs1, reg_t rs2) {
    // rs1 is indices (nibbles), rs2 is table (nibbles)
    reg_t out = 0;
    for (int i = 0; i < 8; i++) { // 8 nibbles in 32-bit word
        uint8_t idx = (rs1 >> (i * 4)) & 0xF;
        if (idx < 8) { // Only indices 0-7 are valid for 32-bit reg
            uint8_t val = (rs2 >> (idx * 4)) & 0xF;
            out |= ((uint32_t)val << (i * 4));
        }
    }
    return out;
}

void report(const char* name, int passed, reg_t sw_res, reg_t hw_res, uint32_t t_sw, uint32_t t_hw) {
    if (t_hw == 0) t_hw = 1;
    uint32_t ratio = (t_sw * 100) / t_hw;
    uint32_t int_part = ratio / 100;
    uint32_t dec_part = ratio % 100;

    if (passed) {
        printf("[PASS] %-7s | Res: 0x%08" PRIreg " | SW: %5lu | HW: %5lu | Speedup: %lu.%02lux\n", 
               name, hw_res, t_sw, t_hw, int_part, dec_part);
    } else {
        printf("[FAIL] %-7s | Exp: 0x%08" PRIreg " | Got: 0x%08" PRIreg " | SW: %5lu | HW: %5lu\n", 
               name, sw_res, hw_res, t_sw, t_hw);
    }
}

#define TEST_BIN(NAME, EXT, ASM_OP, FUNC) do { \
    reg_t sw_res, hw_res; \
    uint32_t t0, t_sw, t_hw; \
    int i; \
    sw_res = FUNC(g_src1, g_src2); \
    asm volatile (".option push\n.option arch, +" #EXT "\n" ASM_OP "\n.option pop" \
                  : "=r"(hw_res) : "r"(g_src1), "r"(g_src2)); \
    int pass = (sw_res == hw_res); \
    t0 = get_cycles(); \
    for(i=0; i<ITERATIONS; i++) g_res = FUNC(g_src1, g_src2); \
    t_sw = get_cycles() - t0; \
    t0 = get_cycles(); \
    for(i=0; i<ITERATIONS; i++) { \
        asm volatile (".option push\n.option arch, +" #EXT "\n" ASM_OP "\n.option pop" \
                      : "=r"(g_res) : "r"(g_src1), "r"(g_src2)); \
    } \
    t_hw = get_cycles() - t0; \
    report(NAME, pass, sw_res, hw_res, t_sw, t_hw); \
} while(0)

#define TEST_UNI(NAME, EXT, ASM_OP, FUNC) do { \
    reg_t sw_res, hw_res; \
    uint32_t t0, t_sw, t_hw; \
    int i; \
    sw_res = FUNC(g_src1); \
    asm volatile (".option push\n.option arch, +" #EXT "\n" ASM_OP "\n.option pop" \
                  : "=r"(hw_res) : "r"(g_src1)); \
    int pass = (sw_res == hw_res); \
    t0 = get_cycles(); \
    for(i=0; i<ITERATIONS; i++) g_res = FUNC(g_src1); \
    t_sw = get_cycles() - t0; \
    t0 = get_cycles(); \
    for(i=0; i<ITERATIONS; i++) { \
        asm volatile (".option push\n.option arch, +" #EXT "\n" ASM_OP "\n.option pop" \
                      : "=r"(g_res) : "r"(g_src1)); \
    } \
    t_hw = get_cycles() - t0; \
    report(NAME, pass, sw_res, hw_res, t_sw, t_hw); \
} while(0)

#define TEST_IMM(NAME, EXT, ASM_OP, FUNC, IMM_VAL) do { \
    reg_t sw_res, hw_res; \
    uint32_t t0, t_sw, t_hw; \
    int i; \
    sw_res = FUNC(g_src1, IMM_VAL); \
    asm volatile (".option push\n.option arch, +" #EXT "\n" ASM_OP "\n.option pop" \
                  : "=r"(hw_res) : "r"(g_src1)); \
    int pass = (sw_res == hw_res); \
    t0 = get_cycles(); \
    for(i=0; i<ITERATIONS; i++) g_res = FUNC(g_src1, IMM_VAL); \
    t_sw = get_cycles() - t0; \
    t0 = get_cycles(); \
    for(i=0; i<ITERATIONS; i++) { \
        asm volatile (".option push\n.option arch, +" #EXT "\n" ASM_OP "\n.option pop" \
                      : "=r"(g_res) : "r"(g_src1)); \
    } \
    t_hw = get_cycles() - t0; \
    report(NAME, pass, sw_res, hw_res, t_sw, t_hw); \
} while(0)

void zbkb_test_benchmark(void) {
    printf("--------------------------------------------");
    printf("\nZbk(b/c/x) Plugin VexRiscv: Verify & Benchmark (N=%d iterations)\n", ITERATIONS);
    printf("Test Input 1: 0x%08" PRIreg "\n", g_src1);
    printf("Test Input 2: 0x%08" PRIreg "\n", g_src2);
    printf("--------------------------------------------\n");

    // --- Zbkb (Bit-manipulation for Cryptography) ---
    TEST_BIN("ANDN",  zbkb, "andn %0, %1, %2",    emu_andn);
    TEST_BIN("ORN",   zbkb, "orn %0, %1, %2",     emu_orn);
    TEST_BIN("XNOR",  zbkb, "xnor %0, %1, %2",    emu_xnor);
    TEST_BIN("ROL",   zbkb, "rol %0, %1, %2",     emu_rol);
    TEST_BIN("ROR",   zbkb, "ror %0, %1, %2",     emu_ror);
    TEST_IMM("RORI",  zbkb, "rori %0, %1, 4",     emu_rori, 4);
    TEST_UNI("REV8",  zbkb, "rev8 %0, %1",        emu_rev8);
    TEST_BIN("PACK",  zbkb, "pack %0, %1, %2",    emu_pack);
    TEST_BIN("PACKH", zbkb, "packh %0, %1, %2",   emu_packh);
    TEST_UNI("BREV8", zbkb, "brev8 %0, %1",       emu_brev8);
    TEST_UNI("ZIP",   zbkb, "zip %0, %1",         emu_zip);
    TEST_UNI("UNZIP", zbkb, "unzip %0, %1",       emu_unzip);

    // --- Zbkc (Carry-less Multiply) ---
    TEST_BIN("CLMUL",  zbkc, "clmul %0, %1, %2",  emu_clmul);
    TEST_BIN("CLMULH", zbkc, "clmulh %0, %1, %2", emu_clmulh);

    // --- Zbkx (Crossbar Permutation) ---
    TEST_BIN("XPERM8", zbkx, "xperm8 %0, %1, %2", emu_xperm8);
    TEST_BIN("XPERM4", zbkx, "xperm4 %0, %1, %2", emu_xperm4);

    printf("----------------------------------------------------------------------\n");
}