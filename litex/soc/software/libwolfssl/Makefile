include ../include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

OBJECTS=aes.o arc4.o ascon.o asm.o asn.o blake2b.o blake2s.o camellia.o chacha20_poly1305.o chacha.o cmac.o coding.o 
OBJECTS+=compress.o cpuid.o cryptocb.o curve25519.o curve448.o des3.o dh.o dilithium.o dsa.o ecc.o ecc_fp.o eccsi.o
OBJECTS+=ed25519.o ed448.o error.o evp.o ext_lms.o ext_mlkem.o ext_xmss.o falcon.o fe_448.o fe_low_mem.o fe_operations.o
OBJECTS+=ge_448.o ge_low_mem.o ge_operations.o hash.o hmac.o hpke.o integer.o kdf.o logging.o md2.o md4.o md5.o memory.o
OBJECTS+=misc.o pkcs12.o pkcs7.o poly1305.o pwdbased.o random.o rc2.o ripemd.o rsa.o sakke.o sha256.o sha3.o sha512.o
OBJECTS+=sha.o signature.o siphash.o sm2.o sm3.o sm4.o sphincs.o sp_arm32.o sp_arm64.o sp_armthumb.o sp_c32.o sp_c64.o
OBJECTS+=sp_cortexm.o sp_dsp32.o sp_int.o sp_sm2_c32.o sp_sm2_c64.o sp_sm2_cortexm.o sp_sm2_x86_64.o sp_x86_64.o srp.o
OBJECTS+=tfm.o wc_dsp.o wc_encrypt.o wc_lms.o wc_lms_impl.o wc_mlkem.o wc_mlkem_poly.o wc_pkcs11.o wc_port.o wc_xmss.o 
OBJECTS+=wc_xmss_impl.o wolfevent.o wolfmath.o

all: libwolfssl.a

libwolfssl.a: $(OBJECTS)
	$(AR) crs libwolfssl.a $(OBJECTS)

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

%.o: $(LIBWOLFSSL_DIRECTORY)/../../../../../libwolfssl/wolfcrypt/src/%.c
	$(compile)

%.o: %.S
	$(assemble)

.PHONY: all clean

clean:
	$(RM) $(OBJECTS) libwolfssl.a .*~ *~
