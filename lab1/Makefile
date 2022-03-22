BASE = $(shell pwd)
KDIR = ~/linux-kernel/linux-wsl
BUILD = $(BASE)/build
SRC = $(BASE)/src
BUILD_MAKEFILE = $(BUILD)/Makefile

MOD_NAME = char_dev

$(BUILD)/$(MOD_NAME).ko: $(BUILD_MAKEFILE)
	make -C $(KDIR) M=$(BUILD) src=$(SRC) modules
	
$(BUILD):
	mkdir -p "$@"
	
$(BUILD_MAKEFILE): $(BUILD)
	touch "$@"
	
clean:
	make -C $(KDIR) M=$(BUILD) src=$(SRC) clean
	
load: $(BUILD)/$(MOD_NAME).ko
	sudo insmod $(BUILD)/$(MOD_NAME).ko
	lsmod

unload:
	sudo rmmod $(MOD_NAME)
	
reload: unload clean load


	
