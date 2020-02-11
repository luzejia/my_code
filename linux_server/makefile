SrcFiles=$(wildcard *.cpp)
ObjFiles=$(patsubst %.cpp,%.o,$(SrcFiles))
CC=g++
MY_PATH=-I./include 
MY_FLAG=-lpthread -lsqlite3
all:my_server

my_server:$(ObjFiles)
	$(CC) -o $@ $(MY_PATH) $(ObjFiles) $(MY_FLAG)

%.o:%.c
	$(CC) -c $< $(MY_PATH) -o $@

.PHONY:clean all

clean:
	-@rm -f *.o
	-@rm -f my_server
