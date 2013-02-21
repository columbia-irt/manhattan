TOPDIR = .
include $(TOPDIR)/Makefile.inc

EXE = (EXEDIR)/sined
OBJLIBS = (LIBDIR)/libsine.a
TARGET = $(EXE)
CFLAGS = -DNDEBUG
export CFLAGS

all: $(TARGET)

$(EXE): $(OBJLIBS)
	$(MAKE) -C $(EXEDIR)

$(OBJLIBS):
	$(MAKE) -C $(LIBDIR)

install: install_exe install_lib

install_exe:
	$(MAKE) -C $(EXEDIR) install

install_lib:
	$(MAKE) -C $(LIBDIR) install

clean: clean_exe clean_lib

clean_exe:
	$(MAKE) -C $(EXEDIR) clean

clean_lib:
	$(MAKE) -C $(LIBDIR) clean
