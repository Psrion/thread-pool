
TARGET = sql_threads_pool
CC = g++
INCLUDE = ./include/sql_threads_pool.h

SRC = $(wildcard ./src/*.cpp)
OBJ = $(patsubst ./src/%.cpp, ./obj/%.o, $(SRC))
OBJ_DIR = $(PWD)/obj/
LFLAG = -lpthread 
LDEBUG = -g -Wall

$(TARGET):$(OBJ) 
	@echo
	$(CC) $(LDEBUG) $^ -o $@ $(LFLAG)
	@echo

$(OBJ):$(SRC)
	@echo
	$(CC) -c $< -o $@ $(LFLAG)


.PHONY:clean

clean:
	@echo 
	rm -rf $(TARGET) $(OBJ_DIR)/*
	@echo

