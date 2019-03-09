# Copyright (C) 2017, Chris Simmonds (chris@2net.co.uk)
#
# If cross-compiling, CC must point to your cross compiler, for example:
# make CC=arm-linux-gnueabihf-gcc

CFLAGS = -Wall -g -pthread -I./inc
BIN 	:= ./bin
SRC 	:= ./src
RES	:= ./res
SRCS  := $(wildcard $(SRC)/*.c)
OBJS  := $(patsubst $(SRC)/%.c, $(RES)/%.o, $(SRCS))
PROG  := ./bin/prog

all: $(PROG)

$(RES)/%.o: $(SRC)/%.c
	mkdir -p $(RES)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROG): $(OBJS)
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@ -lrt

clean:
	rm -rf $(BIN)
	rm -rf $(RES)
