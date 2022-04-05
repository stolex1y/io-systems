BASE = $(shell pwd)
KDIR_WSL = ~/linux-kernel/linux-wsl
KDIR = /lib/modules/$(shell uname -r)/build
BUILD = $(BASE)/build
SRC = $(BASE)/src
BUILD_MAKEFILE = $(BUILD)/Makefile
MOD_NAME = blk_dev

default: build

$(BUILD):
	mkdir -p "$@"
	
$(BUILD_MAKEFILE): $(BUILD)
	touch "$@"	
	
load: build
	sudo insmod $(BUILD)/$(MOD_NAME).ko
	lsmod

unload:
	sudo rmmod $(MOD_NAME)
	
reload: unload clean load

build: $(BUILD_MAKEFILE)
	make -C $(KDIR) M=$(BUILD) src=$(SRC) modules
	
clean:
	make -C $(KDIR) M=$(BUILD) src=$(SRC) clean
	
# wsl
build-wsl: $(BUILD_MAKEFILE)
	make -C $(KDIR_WSL) M=$(BUILD) src=$(SRC) modules
	
clean-wsl:
	make -C $(KDIR_WSL) M=$(BUILD) src=$(SRC) clean	
	
load-wsl: build-wsl
	sudo insmod $(BUILD)/$(MOD_NAME).ko
	lsmod
	
reload-wsl: unload clean-wsl load-wsl
	


	
