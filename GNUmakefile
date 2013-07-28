CC=gcc
CFLAGS = -Iinclude/ 
OBJS = lib/V1729.o lib/V812.o
LIBS = -lm -lCAENVME

all: $(OBJS)
	$(CC)	-o bin/test $(OBJS) $(LIBS) $(CFLAGS) src/main.c

lib/V1729.o: include/V1729.h include/CAENVMElib.h src/V1729.c
	$(CC) $(CFLAGS) -c src/V1729.c -o lib/V1729.o

lib/V812.o: include/V812.h include/CAENVMElib.h include/V1729.h src/V812.c
	$(CC) $(CFLAGS) -c src/V812.c -o lib/V812.o

clean:
	rm -f bin/test $(OBJECTS)
