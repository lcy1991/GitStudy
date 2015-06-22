ARCH:=$(shell uname -m)
BIT32:=i686
BIT64:=x86_64
OBJ=                   \
	AHandler.o         \
	ALooper.o          \
	ALooperRoster.o    \
	AMessage.o         \
	AString.o
CC=g++
CFLAGS=-Wno-multichar

ifeq ($(ARCH),$(BIT64))
FLAG64=-fPIC
endif

INCLUDES= -I./include/
LDLIBS= -lpthread
libstagefright_foundation.so:$(OBJ)
	$(CC) -shared  $(FLAG64) $(LDLIBS) $(OBJ) -o libstagefright_foundation.so 	
$(OBJ): %.o: %.cpp
	$(CC) -c $(FLAG64) $(CFLAGS) $(INCLUDES) $< -o $@
.PHONY:clean
clean:
	rm -f *.o libstagefright_foundation.so