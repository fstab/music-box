OBJS = \
	track.o \
	bstdfile.o \
	mad_decoder.o

all: $(OBJS)

%.o: %.c
	gcc -m64 -I../.. -g -Wall -c $<

clean:
	rm -f $(OBJS)
