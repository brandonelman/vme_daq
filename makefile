CC=g++
CFLAGS = -g -Iinclude/ -Wall
OBJS = lib/V1729.o lib/V812.o
LIBS = -lm -lCAENVME -lCAENComm

all: $(OBJS)
	$(CC)	-o bin/adc_spectrum  $(OBJS)  $(CFLAGS) src/main.c $(LIBS)

lib/V1729.o: include/V1729.h  src/V1729.c
	$(CC) $(CFLAGS)   -c src/V1729.c -o lib/V1729.o $(LIBS)

lib/V812.o: include/V812.h  include/V1729.h src/V812.c
	$(CC) $(CFLAGS) -c src/V812.c -o lib/V812.o $(LIBS)

clean:
	rm -f bin/test $(OBJECTS)
