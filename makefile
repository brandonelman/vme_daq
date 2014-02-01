CC=gcc
CFLAGS = -L/usr/local/lib/CAEN -g -Iinclude/ -I/usr/local/include/CAEN -Wall -DVERSION=\"$(GIT_VERSION)\"
OBJS = lib/V1729.o 
LIBS = -lm -lCAENVME 
GIT_VERSION := $(shell git describe --abbrev=8 --dirty --always)

all: $(OBJS)
	$(CC)	-o bin/daq  $(OBJS)  $(CFLAGS) src/main.c $(LIBS)

lib/V1729.o: include/V1729.h  src/V1729.c
	$(CC) $(CFLAGS)   -c src/V1729.c -o lib/V1729.o $(LIBS)

volt_up: include/V6521M.h src/V6521M.c src/volt_up.c
	$(CC) $(CFLAGS) src/volt_up.c src/V6521M.c -o bin/volt_up $(LIBS) 

volt_down: include/V6521M.h src/V6521M.c src/volt_down.c
	$(CC) $(CFLAGS) src/volt_down.c src/V6521M.c -o bin/volt_down $(LIBS) 

clean:
	rm -f bin/test $(OBJECTS)
