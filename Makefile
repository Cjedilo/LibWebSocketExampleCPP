CC=clang++
CFLAGS=-g -c -Wall -std=c++11 -I/usr/local/include
LDFLAGS=$(shell pkg-config --libs libwebsockets)

SOURCES=$(shell find . -name "*.cpp" | tr "\\n" " ")
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=cppServer

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -pthread -o $@

.PHONY: $(WEB_FRAMEWORK)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	find . -name "*.o" -exec rm {} \;
	rm -f $(EXECUTABLE)

