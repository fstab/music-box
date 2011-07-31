all: music-box

music-box: objs
	gcc -m64 -g -Wall -o music-box \
		./libmbx/config/config.o \
		./libmbx/core/controller.o \
		./libmbx/out/audio_output.o \
		./libmbx/out/log_context_state.o \
		./libmbx/out/device_name_list.o \
		./libmbx/common/mbx_errno.o \
		./libmbx/common/log.o \
		./libmbx/common/xmalloc.o \
		./libmbx/mp3lib/mad_decoder.o \
		./libmbx/mp3lib/track.o \
		./libmbx/mp3lib/bstdfile.o \
		./shell/shell.o \
		./shell/main.o \
		-lpulse -lmad -lreadline
objs:
	$(MAKE) -C libmbx/common
	$(MAKE) -C libmbx/config
	$(MAKE) -C libmbx/core
	$(MAKE) -C libmbx/mp3lib
	$(MAKE) -C libmbx/out
	$(MAKE) -C shell

clean:
	$(MAKE) -C libmbx/common clean
	$(MAKE) -C libmbx/core clean
	$(MAKE) -C libmbx/mp3lib clean
	$(MAKE) -C libmbx/out clean
	$(MAKE) -C shell clean
