# define the C compiler to use
CC = gcc
# define any compile-time flags
CFLAGS = -Wall -g

INCLUDES += -I./src/maxproto

PARSEY = src/maxctl/parse.y
PARSER = src/maxctl/parse.c
SRCS = src/maxproto/max.c src/maxproto/base64.c src/maxproto/maxmsg.c
SRCS += src/maxctl/maxctl.c $(PARSER)

OBJS = $(SRCS:.c=.o)

MAIN = maxctl

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all: parser $(MAIN)
	@echo  Build OK!

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

parser:
	yacc -p max -o $(PARSER) $(PARSEY)

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it

