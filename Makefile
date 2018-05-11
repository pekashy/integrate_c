CC:= gcc
CFLAGS = -g -pg -std=c99 -fprofile-arcs -ftest-coverage -Wall# -g for debug, -O2 for optimise and -Wall additional messages
LFLAGS = -pthread -lm --coverage
SOURCES =main.c
OBJECTS =main.o
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
