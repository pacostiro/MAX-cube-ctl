# define the C compiler to use
CC = gcc
# define the Java compiler to use
JAVAC = javac
JAVAH = javah
#JAVA_HOME = /usr/lib/jvm/default-java
# define any compile-time flags
CFLAGS = -Wall -g -fPIC
LFLAGS = -ldl
LDFLAGS = -shared
INCLUDES = -I./src/maxproto
#INCLUDES += -I./src/maxctl -I$(JAVA_HOME)/include/ -I$(JAVA_HOME)/include/linux

RM = rm -f

PARSEY = src/maxctl/parse.y
PARSER = src/maxctl/parse.c
LIBSRCS = src/maxproto/max.c src/maxproto/base64.c src/maxproto/maxmsg.c
LIBSRCS += src/maxctl/maxctl.c src/maxctl/max_parser.c $(PARSER)
LIBOBJS = $(LIBSRCS:.c=.o)
SRCS = $(LIBSRCS)
SRCS += src/maxctl/maxctl_main.c
OBJS = $(SRCS:.c=.o)
#JNISRCS = java/MaxCtlJNI.java
JNIOBJS = $(JNISRCS:.java=.o)
DEPS = $(SRCS:.c=.d)

MAIN = maxctl
SHAREDLIB = libmaxctl.so

.PHONY: clean

all: $(MAIN) $(SHAREDLIB)
	@echo  Build OK!

$(MAIN): $(OBJS)
	$(CC) -o $(MAIN) $(OBJS) $(LFLAGS)

$(SHAREDLIB): $(LIBOBJS) $(JNIOBJS)
	$(CC) ${LDFLAGS} -o $(SHAREDLIB) $(LIBOBJS) $(JNIOBJS)

%.d: %.c
	@set -e; rm -f $@; \
        $(CC) -MM $(CFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($(*F)\)\.o[ :]*,$(@D)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c  -o $*.o

$(PARSER): $(PARSEY)
	yacc -p max -o $(PARSER) $(PARSEY)

#$(JNIOBJS): $(JNIOBJS:.o=.h) $(JNIOBJS:.o=.c)
#	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c  -o $*.o

#$(JNIOBJS:.o=.h):$(JNIOBJS:.o=.java)
#	$(JAVAC) $*.java
#	$(JAVAH) -classpath $(*D) -d $(*D) $(*F)

clean:
	$(RM) $(OBJS) $(DEPS) $(JNISRCS:.java=.class) $(JNISRCS:.java=.h) $(JNIOBJS) $(PARSER) \
	 *~ $(SHAREDLIB) $(MAIN)

-include $(DEPS)
