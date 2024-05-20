obj-m += my_timer.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	sudo insmod my_timer.ko

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	sudo rmmod my_timer
