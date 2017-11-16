obj-m += led_mod.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	
clean:
	make -c /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
