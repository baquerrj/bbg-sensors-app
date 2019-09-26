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



CFLAGS	:= -Wall -g3 -pthread -I./inc
BIN 	:= ./bin
SRC 	:= ./src
RES		:= ./res
SRCS  	:= $(wildcard $(SRC)/*.c)
OBJS  	:= $(patsubst $(SRC)/%.c, $(RES)/%.o, $(SRCS))
OBJS_CROSS  := $(patsubst $(SRC)/%.c, $(RES)/%_bbg.o, $(SRCS))

CC	:= /usr/bin/gcc
AR 	:= /usr/bin/ar
CC_CROSS := arm-linux-gnueabihf-gcc
AR_CROSS := arm-linux-gnueabihf-ar


PROG		:= $(BIN)/project1.out
PROG_CROSS 	:= $(BIN)/project1_bbg.out

.PHONY: default
default: cross

.PHONY: clean
clean:
	rm -rf $(BIN)
	rm -rf $(RES)

.PHONY: all
all: pre-build cross normal

.PHONY: pre-build
pre-build:
	@mkdir -p $(BIN)
	@mkdir -p $(RES)

.PHONY: cross
cross: $(PROG_CROSS)
	
$(PROG_CROSS): $(OBJS_CROSS)
	@mkdir -p $(BIN)
	$(CC_CROSS) $(CFLAGS) $^ -o $@ -lrt -L/home/baquerrj/mraa/build/src -lmraa -lm
	@#scp $(PROG_CROSS) baquerrj@beaglebone:~/

$(RES)/%_bbg.o: $(SRC)/%.c
	@mkdir -p $(RES)
	$(CC_CROSS) $(CFLAGS) -c $< -o $@

normal: $(PROG)

$(RES)/%.o: $(SRC)/%.c
	@mkdir -p $(RES)
	$(CC) $(CFLAGS) -c $< -o $@

$(PROG): $(OBJS)
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@ -lrt -lmraa -lm


