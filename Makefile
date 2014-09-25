# Makefile
# 
# CSCI 4273 Fall 2014
#
# Programming Assignment 1: Chat Program
# 
# Author: Christopher Jordan
# 
# Updated: 09/23/2014

CHAT= chatServer chatClient chatCoordinator
CFLG=-O3 -Wall
CLEAN=rm -f $(EX) *.o *.a

# Main target
chat: $(CHAT)

# Generic compile rules
.c.o:
	gcc -c $(CFLG) $<
.cpp.o:
	g++ -c  $(CFLG) $<

#  Generic compile and link
%:%.c
	gcc -Wall -O3 -o $@ $^

#  Clean
clean:
	rm -f $(CHAT) *.o *.a