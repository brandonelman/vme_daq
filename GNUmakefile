CC=g++
CFLAGS = -Iinclude/ -g
OBJS = lib/V1729.o lib/V812.o
LIBS = -lm -lCAENVME

all: $(OBJS)
	$(CC)	-o bin/adc_spectrum  $(OBJS) $(LIBS) $(CFLAGS) src/main.c `root-config --cflags --libs`

lib/V1729.o: include/V1729.h  src/V1729.c
	$(CC) $(CFLAGS) -c src/V1729.c -o lib/V1729.o

lib/V812.o: include/V812.h  include/V1729.h src/V812.c
	$(CC) $(CFLAGS) -c src/V812.c -o lib/V812.o

clean:
	rm -f bin/test $(OBJECTS)
