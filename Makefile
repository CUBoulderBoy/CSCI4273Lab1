ALL=echoServer echoClient chatServer chatClient chatCoordinator
ECHO= echoServer echoClient
CHAT= chatServer chatClient chatCoordinator
CFLG=-O3 -Wall
CLEAN=rm -f $(EX) *.o *.a

# Main target
all: $(ALL)
echo: $(ECHO)
chat: $(CHAT)

# Generic compile rules
.c.o:
	gcc -c $(CFLG) $<
.cpp.o:
	g++ -c $(CFLG) $<

#  Generic compile and link
%:%.c
	gcc -Wall -O3 -o $@ $^

#  Clean
clean:
	rm -f $(ALL) *.o *.a