CC:= gcc
CFLAGS = -g -Wall# -g for debug, -O2 for optimise and -Wall additional messages
LFLAGS = -pthread
SOURCES = count.c main.c
OBJECTS = count.o main.o
EXECUTABLE = run
.PHONY: clean

all: build link clean

build: $(SOURCES)

link: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LFLAGS) -o $@ $(OBJECTS)

$(SOURCES):
	$(CC) $(CFLAGS) -o $@ -c $< -MD

clean:
	rm -rf *.d *.o

-include *.d
