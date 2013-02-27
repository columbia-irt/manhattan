TOPDIR = ..
include $(TOPDIR)/Makefile.inc

OBJECTS = libsine.o
SOURCES = $(patsubst %.o, %.c, $(OBJECTS))
HEADERS = sine.h

TARGET = libsine.a
SO_TARGET = $(patsubst %.a, %.so, $(TARGET))

default: $(TARGET)

all: $(TARGET) $(SO_TARGET)

$(TARGET): CFLAGS += -fPIC
$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	$(RANLIB) $@

$(SO_TARGET): CFLAGS += -rdynamic
$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

install: all
	install -d $(PREFIX)/lib/
	install -d $(PREFIX)/include/
	install $(TARGET) $(PREFIX)/lib/
	install $(SO_TARGET) $(PREFIX)/lib/
	install $(HEADERS) $(PREFIX)/include/

clean:
	rm -f $(OBJECTS) $(TARGET) $(SO_TARGET)
