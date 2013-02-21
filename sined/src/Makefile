TOPDIR = ..
include $(TOPDIR)/Makefile.inc

LIBS += -lpthread -lxml2 -lsine

OBJECTS = sined.o conmgr.o netmgr.o locmgr.o pe.o socks.o
SOURCES = $(patsubst %.o, %.c, $(OBJECTS)) $(patsubst %.o, %.h, $(OBJECTS))

TARGET = sined

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

install: all
	install -d $(PREFIX)/bin/
	install $(TARGET) $(PREFIX)/bin/

clean:
	rm -f $(OBJECTS) $(TARGET)
