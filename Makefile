CC=g++
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(CPP_FILES:.cpp=.o)

all: engine

clean:
	rm -rf *.o 

%.o: %.cpp
	$(CC) -Wall -O2 $< -c -o $@

engine: $(OBJ_FILES)
	$(CC) *.o -o $@
