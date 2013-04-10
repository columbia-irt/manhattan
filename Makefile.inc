CROSS_PREFIX	 = 
CC		 = $(CROSS_PREFIX)gcc
LD		 = $(CROSS_PREFIX)ld
AR		 = $(CROSS_PREFIX)ar
RANLIB		 = $(CROSS_PREFIX)ranlib

MAKE		 = make
INCDIR		 = $(TOPDIR)/include
LIBDIR		 = $(TOPDIR)/lib
PREFIX		?= /usr/local

CFLAGS		+= -g -O2 -Wall -Wextra -I$(INCDIR) -I$(LIBDIR)
LDFLAGS		+= -L$(LIBDIR)
ARFLAGS		 = rcs
