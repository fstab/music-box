OBJS = \
	audio_output.o \
	log_context_state.o \
	device_name_list.o

all: $(OBJS)

%.o: %.c
	gcc -m64 -I../.. -g -Wall -c $<

clean:
	rm -f $(OBJS)
