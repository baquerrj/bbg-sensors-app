#  
#  =================================================================================
#     @file     Makefile
#     @brief    
#  
#   Makefile for BeagleBone Green Temperature and Light Application
#  
#     @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
#  
#     @internal
#        Created:  04/01/2019
#       Revision:  none
#       Compiler:  gcc
#   Organization:  University of Colorado: Boulder
#  
#   This source code is released for free distribution under the terms of the
#   GNU General Public License as published by the Free Software Foundation.
#  =================================================================================
#  



CFLAGS = -Wall -g3 -pthread -I./inc
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

$(RES)/%.o: $(SRC)/%.c
	mkdir -p $(RES)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROG): $(OBJS) $(RES)/libcommon.a
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@ -lrt -lmraa -lm -L$(RES) -lcommon

clean:
	rm -rf $(BIN)
	rm -rf $(RES)
