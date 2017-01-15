# define the C compiler to use
CC = gcc
# define any compile-time flags
CFLAGS = -Wall -g -fPIC
LFLAGS = -ldl
LDFLAGS = -shared
INCLUDES = -I./src/maxproto

RM = rm -f

PARSEY = src/maxctl/parse.y
PARSER = src/maxctl/parse.c
LIBSRCS = src/maxproto/max.c src/maxproto/base64.c src/maxproto/maxmsg.c
LIBSRCS += src/maxctl/maxctl.c src/maxctl/max_parser.c $(PARSER)
LIBOBJS = $(LIBSRCS:.c=.o)
SRCS = $(LIBSRCS)
SRCS += src/maxctl/maxctl_main.c
OBJS = $(SRCS:.c=.o)

MAIN = maxctl
SHAREDLIB = libmaxctl.so

.PHONY: clean

all: $(MAIN) $(SHAREDLIB)
	@echo  Build OK!

$(MAIN): $(OBJS)
	$(CC) -o $(MAIN) $(OBJS) $(LFLAGS)

$(SHAREDLIB): $(LIBOBJS)
	$(CC) ${LDFLAGS} -o $(SHAREDLIB) $(LIBOBJS)

%.d: %.c
	@set -e; rm -f $@; \
        $(CC) -MM $(CFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($(*F)\)\.o[ :]*,$(@D)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c  -o $*.o

$(PARSER): $(PARSEY)
	yacc -p max -o $(PARSER) $(PARSEY)

clean:
	$(RM) $(OBJS) $(SRCS:.c=.d) $(PARSER) \
	 *~ $(SHAREDLIB) $(MAIN)

-include $(SRCS:.c=.d)
