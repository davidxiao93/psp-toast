TARGET = toast
OBJS = import.o main.o apihook.o draw.o screenshot.o exports.o sceCtrl_driver.o include/sysconhk.o

BUILD_PRX=1
USE_KERNEL_LIBC=1
USE_KERNEL_LIBS=1

CFLAGS = -Os -G0 -Wall -fno-strict-aliasing -fno-builtin-printf -D_PSPSLIM -D_ENGLISH_ONLY
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

INCDIR = include
LIBDIR = lib
LDFLAGS = -nostdlib -nodefaultlibs
LIBS = -lpspsystemctrl_kernel -lpsprtc -lpsppower_driver -lgcc

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

