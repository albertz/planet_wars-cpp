CC=g++

all: engine

clean:
	rm -rf *.o 

%.o: %.cpp
	$(CC) -Wall -O2 $< -c

engine: *.o
	$(CC) *.o -o $@
