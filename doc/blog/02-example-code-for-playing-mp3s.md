music box 2 / example code for playing mp3s
===========================================

As a starting point, I looked at open source libraries that might be a good
starting point for implementing my own software.

This post is about [madlld][1] and [pacat][2], which are command line tools
with tutorial-like source codes that can be combined to play mp3s.

audio output
------------

My favorite Linux distribution is Ubuntu, and Ubuntu, as of now, uses
[PulseAudio][3] to play audio. I have read about [JACK][4] as a real-time,
low-latency alternative, but as long as I don't run into trouble, I'll give
[PulseAudio][3] a try.

[PulseAudio][3] comes with an application called [pacat][2], which is a simple
command line tool to play back audio streams. [pacat][2] lets you play around
with different latency settings, which might become helpful later.

[pacat][2] doesn't need to be downloaded, it's included in the Ubuntu standard
setup. This is how to get the source

	git clone git://anongit.freedesktop.org/pulseaudio/pulseaudio
	cat src/utils/pacat.c

mp3 decoder
-----------

There are many mp3 decoder libraries. One of the most widespread seems to be
[mad][5], the MPEG audio decoder. The project doesn't seem very active right
now, but I think this is more because it's done, not because people don't use
it anymore.

There is a simple command line application using the [mad][5] library, which is
called [madlld][1]: The MPEG Audio Decoder Low-Level Demonstration. This is
kind of a tutorial source code that can be used to decode an mp3 file and
output the sound data into a file.

[madlld][1] is not included in Ubuntu, so you need to download and compile it.

	tar xfz madlld-1.1p1.tar.gz
	cd madlld-1.1p1
	make

putting the tools together
--------------------------

This is how to play an mp3 in the command line

	./madlld < test1.mp3 > out.dat
	/usr/bin/pacat --format s16be out.dat

initial commit on github
------------------------

I started a [music box github project][6] where I will put my source code. As
a start, I pushed the unmodified source code of [pacat][2] and [madlld][1].
The initial version is tagged [v0.1][7].

As the next step, I will build the mp3 player software from that.

Are [mad][5] and [PulseAudio][3] a good choice?
-----------------------------------------------

If you are reading this and have any experience with implementing audio
software for Linux, please tell me your thoughts about choosing
[mad][5] and [PulseAudio][3]. I'll keep you informed about my success or
failure in this blog.

[1]: http://www.bsd-dk.dk/~elrond/audio/madlld
[2]: http://manpages.ubuntu.com/manpages/natty/man1/pacat.1.html
[3]: http://www.pulseaudio.org
[4]: http://jackaudio.org
[5]: http://www.underbit.com/products/mad
[6]: http://github.com/fstab/music-box
[7]: https://github.com/fstab/music-box/tree/v0.1
