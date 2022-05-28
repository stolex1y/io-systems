BASE = $(shell pwd)
KDIR_WSL = ~/linux-kernel/linux-wsl
KDIR = /lib/modules/$(shell uname -r)/build
BUILD = $(BASE)/build
SRC = $(BASE)/src
BUILD_MAKEFILE = $(BUILD)/Makefile
MOD_NAME = vni

dest_port = 12345
link = eth0
debug = 1
vni_addr = 192.168.1.10

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
	
netload: build
	sudo dmesg -c
	sudo insmod $(BUILD)/$(MOD_NAME).ko dest_port_filter=$(dest_port) debug=$(debug) link=$(link)
	lsmod
	dmesg
	sudo ifconfig vni0 $(vni_addr)
		
	
# wsl
build-wsl: $(BUILD_MAKEFILE)
	make -C $(KDIR_WSL) M=$(BUILD) src=$(SRC) modules
	
clean-wsl:
	make -C $(KDIR_WSL) M=$(BUILD) src=$(SRC) clean	
	
load-wsl: build-wsl
	sudo insmod $(BUILD)/$(MOD_NAME).ko
	lsmod
	
reload-wsl: unload clean-wsl load-wsl

netload-wsl: build-wsl
	sudo dmesg -c
	sudo insmod $(BUILD)/$(MOD_NAME).ko dest_port_filter=$(dest_port) debug=$(debug) link=$(link)
	lsmod
	dmesg
	sudo ifconfig vni0 $(vni_addr)
