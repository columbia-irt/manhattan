# Makefile for test programs

export IN_SUBDIR = 1

PWD	= $(shell pwd)
TOP	= $(PWD)
SHELL	= /bin/sh
prefix	= /usr/local
exec_prefix=${prefix}
srcdir	= .
sbindir	= ${exec_prefix}/sbin
libdir	= ${exec_prefix}/lib
sysconfdir = ${prefix}/etc
MAKE		= make
CC              = gcc
C++		= g++
OPT	= -O2 -DHAVE_CONFIG_H -DSYSCONFDIR=\"$(sysconfdir)\"
CFLAGS  = -shared -fPIC $(DEBUG) $(INCLUDES)
CFLAGS  = $(DEBUG) $(OPT) $(INCLUDES)
#CFLAGS	+= -Dconnect=sine_connect -Dbind=sine_bind -Dgetsockname=sine_getsockname -Dgetpeername=sine_getpeername -Daccept=sine_accept -Drresvport=sine_rresvport -Dbindresvport=sine_bindresvport -Dgethostbyname=sine_gethostbyname -Dgethostbyname2=sine_gethostbyname2 -Dsendto=sine_sendto -Drecvfrom=sine_recvfrom -Drecvfrom=sine_recvfrom -Dwrite=sine_write -Dwritev=sine_writev -Dlisten=sine_listen -Dsend=sine_send -Dsendmsg=sine_sendmsg -Dread=sine_read -Dreadv=sine_readv -Drecv=sine_recv -Drecvmsg=sine_recvmsg -Dgetaddrinfo=sine_getaddrinfo -Dgetipnodebyname=sine_getipnodebyname

WARN		= 
# -Wall -Wno-unused
DEBUG		= -g -DNDEBUG

default: all

# end of $RCSFile$
LOADLIBES =  -lpthread -lcrypt -lcurl

INCLUDES        = -I. -I$(TOP)/protocolMgr -I$(TOP)/connectionMgr -I$(TOP)/policyMgr -I$(TOP)/networkMgr -I$(TOP)/locationMgr -I$(TOP)/srelay/srelay-0.4.8b5

OBJECTS = ./protocolMgr/sine_socket.o ./connectionMgr/connectionMgr.o ./policyMgr/PolicyModel.o ./policyMgr/PolicyListener.o ./policyMgr/PolicyController.o ./networkMgr/nm.o ./locationMgr/lm.o ./securityMgr/sm.o
OBJECTS	+= ./srelay/srelay-0.4.8b5/init.o ./srelay/srelay-0.4.8b5/readconf.o ./srelay/srelay-0.4.8b5/util.o ./srelay/srelay-0.4.8b5/socks.o ./srelay/srelay-0.4.8b5/relay.o ./srelay/srelay-0.4.8b5/main.o ./srelay/srelay-0.4.8b5/auth-pwd.o ./srelay/srelay-0.4.8b5/get-bind.o

TARGETS = sined

SRCS = ./protocolMgr/sine_socket.c ./connectionMgr/connectionMgr.c ./policyMgr/PolicyModel.cpp ./policyMgr/PolicyListener.cpp ./policyMgr/PolicyController.cpp ./networkMgr/nm.cpp ./locationMgr/lm.cpp ./securityMgr/sm.cpp ./srelay/srelay-0.4.8b5/init.c ./srelay/srelay-0.4.8b5/readconf.c ./srelay/srelay-0.4.8b5/util.c ./srelay/srelay-0.4.8b5/socks.c ./srelay/srelay-0.4.8b5/relay.c ./srelay/srelay-0.4.8b5/main.c ./srelay/srelay-0.4.8b5/auth-pwd.c ./srelay/srelay-0.4.8b5/get-bind.c

all: $(TARGETS)

$(TARGETS): $(OBJECTS)
	$(C++) $(LDFLAGS) $(OBJECTS) $(LOADLIBES) -o $@

clean:
	rm -f $(OBJECTS) $(TARGETS) *_tcp_kern* *~ core

depend:
	makedepend $(PROF) $(DEBUG) $(LOADLIBES) $(INCLUDES) I.. $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.

#socks.o: ../middleware/protocolMgr/mptcpMgr.h
#socks.o: ../middleware/policyMgr/policy.h

