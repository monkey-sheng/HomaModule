.PHONY: module clean server.c server.o client.c client.o zcserver.c zcserver.o zcserver_splice.c zcserver_splice.o zcclient.c zcclient.o zcclient_splice.c zcclient_splice.o
obj-m += server.o client.o zcserver.o zcclient.o zcserver_splice.o zcclient_splice.o
PROG = module
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all: $(PROG)
module:
	make -I/home/vagrant/HomaModule0 -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	(rm -rf .*.cmd *.o *.mod.c *.ko *.mod modules.order Module.symvers)
