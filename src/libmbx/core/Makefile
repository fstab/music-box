OBJS = \
	controller.o

all: $(OBJS)

%.o: %.c
	gcc -m64 -I../.. -g -Wall -c $<

clean:
	rm -f $(OBJS)
