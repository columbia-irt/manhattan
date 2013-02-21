CROSS_PREFIX	 = 
CC		 = $(CROSS_PREFIX)gcc
LD		 = $(CROSS_PREFIX)ld
AR		 = $(CROSS_PREFIX)ar
RANLIB		 = $(CROSS_PREFIX)ranlib

MAKE		 = make
EXEDIR		 = $(TOPDIR)/src
LIBDIR		 = $(TOPDIR)/lib
PREFIX		?= /usr/local

CFLAGS		+= -g -O2 -Wall -Wextra -I$(EXEDIR) -I$(LIBDIR)
LDFLAGS		+= -L$(LIBDIR)
ARFLAGS		 = rcs
