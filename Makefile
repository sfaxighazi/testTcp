#Compiler
CC=g++

#Compiler flags
CFLAGS=-Wall

#Linker flags
LDFLAGS=-lpthread

all:
	$(CC) $(CFLAGS) *.cpp *.h -o tcpcpp $(LDFLAGS)
clean:
	rm tcpcpp
