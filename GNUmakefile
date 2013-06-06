TARGET = vme_daq
SHELL  = /bin/bash
.SUFFIXES: .c .o .h .so 
VPATH  = lib:src:include:
vpath %.c src:

DESTDIR  = bin
BUILDDIR = lib
INCDIR   = include
SRCDIR   = src
CXX      = gcc

CFLAGS += -Iinclude 

LDLIBS += -lm -lCAENVME 

OBJS := V1729.o 

HEADERS := $(OBJS:%.o=$(INCDIR)/%.h)
LIBS    := $(OBJS:%.o=$(BUILDDIR)/%.o)

default : main 

main: $(BUILDDIR)/$(OBJS) 
	$(CXX) -o $(DESTDIR)/$(TARGET) src/main.c $(CFLAGS) $(LDLIBS) $(LIBS)

$(BUILDDIR)/$(OBJS) : $(BUILDDIR)/%.o : %.h
	$(CXX) -c -o $@ $(@:$(BUILDDIR)/%.o=$(SRCDIR)/%.c) $(CFLAGS)

doc: 
	doxygen doc/InSANE_Doxyfile #	doxygen doc/Doxyfile_insaneweb
	cd ..
	@echo "HTML Documentation created"

.PHONY : clean doc 


clean:
	rm -f a.out
	rm -f bin/$(TARGET)
	rm -f lib/*.o
