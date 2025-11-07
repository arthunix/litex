include ../include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

CFLAGS += -DWOLFSSL_USER_SETTINGS -Wno-cpp

WOLFSSL_SRC = $(wildcard $(LIBWOLFSSL_DIRECTORY)/../../../../../libwolfssl/wolfcrypt/src/*.c)
WOLFSSL_SRC := $(filter-out %bio.c %misc.c %evp.c, $(WOLFSSL_SRC))
WOLFSSL_SRC := $(sort $(WOLFSSL_SRC))

OBJECTS := $(WOLFSSL_SRC:.c=.o)

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
