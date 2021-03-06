#
# uxmhf-rpi3 top-level Makefile
# 
# author: amit vasudevan (amitvasudevan@acm.org)
#
#


check_defined = \
    $(strip $(foreach 1,$1, \
        $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
    $(if $(value $1),, \
        $(error Error: Set $1$(if $2, ($2))$(if $(value @), \
                to OS kernel image file [required by target `$@'])))


export UXMHFRPI3_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

TOOLPREFIX ?= arm-linux-gnueabihf-

INCS = -I$(UXMHFRPI3_DIR)/libs/libxmhfc/include
INCS += -I$(UXMHFRPI3_DIR)/libs/libxmhfcrypto/include
INCS += -I$(UXMHFRPI3_DIR)/libs/libutpm/include
#INCS += -I$(UXMHFRPI3_DIR)/include

CFLAGS_DEFS =  $(INCS) -I./include -I../include -D__DEBUG_SERIAL__ 
CFLAGS = $(CFLAGS_DEFS) -c -Wall -nostdlib -nostartfiles -ffreestanding -mcpu=cortex-a53
#CFLAGS = -I./include -I../include -D__DEBUG_SERIAL__ -c -Wall -nostdlib -nostartfiles -ffreestanding -march=armv8-a+crc
CCLIB = $(shell $(TOOLPREFIX)gcc --print-lib)

LIBS = $(UXMHFRPI3_DIR)/libs/libutpm/libutpm.a
LIBS += $(UXMHFRPI3_DIR)/libs/libxmhfcrypto/libxmhfcrypto.a
LIBS += $(UXMHFRPI3_DIR)/libs/libxmhfc/libxmhfc.a


# tools
export AR=ar
export CP=cp
export CCERT=$(TOOLPREFIX)gcc
export CC=$(TOOLPREFIX)gcc
export CCERT_FLAGS=$(CFLAGS)
export RM=rm

export TOOLPREFIX
export CFLAGS_DEFS
export CFLAGS
export CCLIB
export LIBS

.PHONY: all
all: checkoskrnlimg 
	cd libs/libxmhfc && make -w all
	cd libs/libxmhfcrypto && make -w all
	cd libs/libutpm && make -w all
	cd core && make -w all
	cd bootstrap && make -w all
	cat ./bootstrap/bootstrap.bin ./oskrnlimg.tmp ./core/core.bin > uxmhf-rpi3.img

.PHONY: checkoskrnlimg
checkoskrnlimg:
	@:$(call check_defined, OSKRNLIMG)
	$(eval OSKRNLIMG_SIZE := $(shell stat -L -c %s $(OSKRNLIMG)))
	@echo OSKRNLIMG_SIZE=$(OSKRNLIMG_SIZE)
	$(eval OSKRNLIMG_ENDADDR := $(shell echo "$(OSKRNLIMG_SIZE) + 32768" | bc))
	@echo OSKRNLIMG_ENDADDR=$(OSKRNLIMG_ENDADDR)
	$(eval OSKRNLIMG_PADSIZE := $(shell echo "16777216 -( $(OSKRNLIMG_ENDADDR) % 16777216)" | bc))
	@echo OSKRNLIMG_PADSIZE=$(OSKRNLIMG_PADSIZE)
	$(eval OSKRNLIMG_FINALSIZE := $(shell echo "$(OSKRNLIMG_SIZE) + $(OSKRNLIMG_PADSIZE) " | bc))
	@echo OSKRNLIMG_FINALSIZE=$(OSKRNLIMG_FINALSIZE)
	#$(eval CORE_STARTADDR := $(shell echo "$(OSKRNLIMG_FINALSIZE)" | bc))
	#$(eval CORE_STARTADDR_HEX=$(shell printf "0x%08x" $(CORE_STARTADDR)))
	#@echo $(CORE_STARTADDR)
	#@echo $(CORE_STARTADDR_HEX)
	cp $(OSKRNLIMG) ./oskrnlimg.tmp
	truncate --size=$(OSKRNLIMG_FINALSIZE) ./oskrnlimg.tmp 
	#./configure --with-core-startaddr=$(CORE_STARTADDR_HEX) --with-core-size=0xC00000
	./configure --with-core-startaddr=0x1000000 --with-core-size=0xC00000


.PHONY: clean
clean:
	cd core && make -w clean
	cd bootstrap && make -w clean
	rm -f *.o
	rm -f *.img 
	rm -f *.elf
	rm -f *.tmp
	rm -f *.bin