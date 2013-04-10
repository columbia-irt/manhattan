TOPDIR = ..
include $(TOPDIR)/Makefile.inc

LIBS += -lvlc

OBJECTS = vlc.o
SOURCES = $(patsubst %.o, %.c, $(OBJECTS)) $(patsubst %.o, %.h, $(OBJECTS))

TARGET = ssmd

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

install: all
	install -d $(PREFIX)/bin/
	install $(TARGET) $(PREFIX)/bin/

clean:
	rm -f $(OBJECTS) $(TARGET)
