CFLAGS=-g -O2 -Wall -Wextra -Isrc $(OPTFLAGS)
LIBS=-lpthread $(OPTLIBS)
PREFIX?=/usr/local

SOURCES=$(wildcard src/**/*.c src/*.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c, %, $(TEST_SRC))

TARGET=bin/sined
A_TARGET=build/libsined.a
SO_TARGET=$(patsubst %.a,%.so, $(A_TARGET))

# The Target Build
all:	$(TARGET) $(SO_TARGET) tests

release: CFLAGS += -DNDEBUG
release: all

$(TARGET): $(A_TARGET)
	$(CC) -o $@ $(A_TARGET) $(LIBS)

$(SO_TARGET):	CFLAGS += -rdynamic
$(SO_TARGET):	$(A_TARGET) $(OBJECTS)
	$(CC) -shared -o $@ $(OBJECTS)

$(A_TARGET):	CFLAGS += -fPIC
$(A_TARGET):	$(OBJECTS)
	@mkdir -p build
	@mkdir -p bin
	ar rcs $@ $(OBJECTS)
	ranlib $@

# The Unit Tests

.PHONY:	tests
tests:  CFLAGS += -ldl $(LIBS)
tests:	$(TESTS)
	sh ./tests/runtests.sh

valgrind:
	VALGRIND="valgrind --log-file=/tmp/valgrind-%p.log" $(MAKE)

# The Cleaner

clean:
	rm -rf bin build $(OBJECTS) $(TESTS)
	rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf 'find . -name "*.dSYM" -print'

#The Install

install:	all
	install -d $(DESTDIR)/$(PREFIX)/bin/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/bin/

#The Checker

BADFUNCS='[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)'
check:
	@echo Files with potentially dangerous functions.
	@egrep $(BADFUNCS) $(SOURCES) || true
