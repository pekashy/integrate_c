CC:= gcc
CFLAGS = -g -pg -fprofile-arcs -ftest-coverage -Wall# -g for debug, -O2 for optimise and -Wall additional messages
LFLAGS = -pthread -lm --coverage
SOURCES = count.c main.c
OBJECTS = count.o main.o
EXECUTABLE = run
.PHONY: clean

all: build link clean

build: $(SOURCES)

link: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LFLAGS)

$(SOURCES):
	$(CC) $(CFLAGS) -o $@ -c $< -MD

clean:
	rm -rf *.d *.o

-include *.d
