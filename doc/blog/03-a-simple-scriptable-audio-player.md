music box 3 / a simple scriptable audio player
==============================================

As an intermediate step towards my music box, I implemented a
_[simple scriptable audio player][1]_.

The idea is to have a simple command line interface to try everything out, and
to replace the command line with a MIDI interface later.

The first version i committed on [github][1] is very limited. However, you can
download it, compile it, and use it to play several mp3s at the same time.

Compiling
---------

The code uses the following libraries. You need to install the corresponding
development packages:

  * **libmad:** The [MPEG Audio Decoder][2]. The Ubuntu development package is
    [`libmad0-dev`][3].
  * **libpulse:** The [PulseAudio][4] library. The Ubuntu development package
    is [`libpulse-dev`][5].
  * **libreadline:** The [GNU Readline library][6]. The Ubuntu development
    package is [`libreadline-dev`][7].

There is a simple `Makefile` included. In order to compile `music-box`, you
just need to:

	git clone https://github.com/fstab/music-box.git
	cd music-box/src
	make

Running
-------

The `Makefile` will create an executable `./music-box`. When you run it in the
command line, you will see a prompt where you can type commands. The following
commands are supported so far:

  * `load <file.mp3> as <var>`<br/>
    Load an mp3 file and store it in variable `<var>`.<br/>
    For file names with spaces, type the file name in "double quotes".
  * `play <var>`<br/>
    Start playing the file loaded as `<var>`
  * `pause <var>`<br/>
    Pause the file loaded as `<var>`
  * `resume <var>`<br/>
    If `<var>` is paused, resume playing `<var>`
  * `wait for <var>`<br/>
    wait until file `<var>` is done playing
  * `unload <var>`<br/>
    Unload the mp3 file and free the variable `<var>`
  * `sleep <seconds>`<br/>
    sleep for `<seconds>` seconds
  * `quit`<br/>
    quit the application

As this is a test program, it will produce a lot of debug messages. You can
start `./music-box` with a command line parameter to redirect debug logging
into a log file:

	./music-box -l out.log

Finally, you can put your commands into a script and use `./music-box` to
execute the script. In that case, you should use the command line parameter
`-q` to suppress user messages written to stdout.

	cat script.txt | ./music-box -q -l out.log

Sample Session
--------------

The following is a sample script that will play `test1.mp3`, wait three
seconds, and then mix in `test2.mp3`.

	load test1.mp3 as x
	load test2.mp3 as y
	play x
	sleep 3
	play y
	wait x
	wait y
	quit

Next Steps
----------

The current implementation is very basic. Most importantly: It does not
implement any latency requirements. When PulseAudio is not configured with any
latency settings, it will use a very high latency, like 2 seconds.

Among many other things, the next steps are:

  * Implement latency requirements
  * Adjust the volume of the tracks when mixing them together
  * Add support for an additional headphone output
  * etc etc

[1]: https://github.com/fstab/music-box
[2]: http://www.underbit.com/products/mad
[3]: http://packages.ubuntu.com/libmad0-dev
[4]: http://www.pulseaudio.org
[5]: http://packages.ubuntu.com/libpulse-dev
[6]: http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html
[7]: http://packages.ubuntu.com/libreadline-dev
