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

run: $(PROG)
	./$(PROG) /tmp/log

$(RES)/libcommon.a: $(RES)/common.o
	$(AR) rc $@ $^
#
#$(RES)/common.o: $(SRC)/common.c
#	mkdir -p $(RES)
#	$(CC) $(CFLAGS) -c $^ -o $@
#
#$(RES)/logger.o: $(SRC)/logger.c
#	mkdir -p $(RES)
#	$(CC) $(CFLAGS) -c $(SRC)/logger.c -o $@
#
#$(RES)/temperature.o: $(SRC)/temperature.c
#	mkdir -p $(RES)
#	$(CC) $(CFLAGS) -c $(SRC)/temperature.c -o $@
#
#$(RES)/watchdog.o: $(SRC)/watchdog.c
#	mkdir -p $(RES)
#	$(CC) $(CFLAGS) -c $(SRC)/watchdog.c -o $@
#
#$(RES)/main.o: $(SRC)/main.c
#	mkdir -p $(RES)
#	$(CC) $(CFLAGS) -c $(SRC)/main.c -o $@
$(RES)/%.o: $(SRC)/%.c
	mkdir -p $(RES)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROG): $(OBJS) $(RES)/libcommon.a
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@ -lrt -L$(RES) -lcommon

clean:
	rm -rf $(BIN)
	rm -rf $(RES)
