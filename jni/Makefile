OBJ=                   \
	AHandler.o         \
	ALooper.o          \
	ALooperRoster.o    \
	AMessage.o         \
	AString.o
CC=g++
CFLAGS=-Wno-multichar
INCLUDES= -I./include/
LDLIBS= -lpthread
libstagefright_foundation.so:$(OBJ)
	$(CC) -shared $(LDLIBS) $(OBJ) -o libstagefright_foundation.so 	
$(OBJ): %.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
.PHONY:clean
clean:
	rm -f *.o libstagefright_foundation.so