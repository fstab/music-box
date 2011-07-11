OBJS = \
	audio_producer.o \
	audio_consumer.o \
	controller.o \
	mad_decoder.o \
	bstdfile.o \
	shell.o \
	log.o \
	error_codes.o

all: music-box

music-box: main.c $(OBJS)
	gcc -g -Wall -lmad -lpulse -lreadline -o music-box main.c $(OBJS)

%.o: %.c %.h
	gcc -g -Wall -c $<

clean:
	rm -f $(OBJS)
