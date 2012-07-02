CD=cd
CP=cp
GCC=gcc
CPP=g++
BJAM=b2
SED=sed
PWD=$(shell pwd)

all: socks hip mip6d mih lm

socks:
	$(MAKE) -C ./middleware/

hip:
	$(MAKE) -C ./protocols/openhip/

mip6d:
	$(MAKE) -C ./protocols/umip/

mih:
	$(CD) ./protocols/mih/odtone; $(BJAM) -q linkflags=-lpthread

lm: ./sine/middleware/locationMgr/locationMgr.cpp
	$(CPP) ./middleware/locationMgr/locationMgr.cpp -o ./middleware/locationMgr/lm -lpthread 

install:
	./middleware/kill_sined
	$(MAKE) -C ./protocols/openhip/ install
	hitgen
	$(MAKE) -C ./protocols/umip/ install
	$(CP) ./protocols/umip/mip6d.conf /usr/local/etc/mip6d.conf
	#$(CP) ./protocols/mih/mih /usr/local/sbin/mih
	$(SED) 's/SINE_ROOT=/SINE_ROOT=$(subst /,\/,$(PWD))/' <./protocols/mih/mih >/usr/local/sbin/mih
	chmod +x /usr/local/sbin/mih
	$(CP) ./middleware/locationMgr/lm /usr/local/sbin/lm
	$(CP) ./middleware/sined /usr/local/sbin/sined
	$(CP) ./middleware/kill_sined /usr/local/sbin/kill_sined
	$(CP) ./middleware/sine_policy.conf /etc/sine_policy.conf
	$(CP) ./middleware/srelay.conf /etc/srelay.conf

clean:
	$(MAKE) -C ./middleware/ clean
	$(MAKE) -C ./protocols/openhip/ clean
	$(MAKE) -C ./protocols/umip/ clean
	$(CD) ./protocols/mih/odtone; $(BJAM) clean
