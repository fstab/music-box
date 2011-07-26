music box 4 / PulseAudio and the Native Instruments Audi2DJ sound card
======================================================================

![Audio2DJ][1]

For my music box, I need an audio interface with two stereo outputs: One for
the headphones, and one for the speakers. I decided on the
[Native Instruments Audio2DJ][2] sound card.

This post is about how to get the Audio2DJ sound card running with
[PulseAudio][3].

Subdevices and PulseAudio
-------------------------

The Audio2DJ appears as a single sound card with two subdevices. You can see
this by running the command `aplay -l`

	**** List of PLAYBACK Hardware Devices ****
	card 0: Intel [HDA Intel], device 0: ALC275 Analog [ALC275 Analog]
	  Subdevices: 1/1
	  Subdevice #0: subdevice #0
	card 0: Intel [HDA Intel], device 3: HDMI 0 [HDMI 0]
	  Subdevices: 1/1
	  Subdevice #0: subdevice #0
	card 1: Audio2DJ [Audio 2 DJ], device 0: Audio 2 DJ [Audio 2 DJ]
	  Subdevices: 2/2
	  Subdevice #0: subdevice #0
	  Subdevice #1: subdevice #1

In my case, there are two sound cards: `card 0` is the built-in sound card in
my laptop, `card 1` is the Audio2DJ.

[aplay][4] is an [ALSA][5] command that lists low level information about the
underlying hardware. The problem is that PulseAudio will not automatically
configure the subdevices. You need to tell PulseAudio about these subdevices
manually.

Step 1: Make sure the subdevices work with ALSA
-----------------------------------------------

Native Instruments [provides ALSA drivers][6] for all of its audio interfaces.
The drivers are already configured on my Ubuntu system, and I can use ALSA's
aplay command to play sound on the subdevices:

	aplay -D plughw:1,0,0 test.wav
	aplay -D plughw:1,0,1 test.wav

The first command would play the file on output A, the second command would
play it on output B.

The first digit in `plughw:1,0,0` is the card number. For me, it is `1` as
Audio2DJ is card number `1` on my system. If Audio2DJ is card number `2` on
your system, you will need to use `plughw:2,0,0` and `plughw:2,0,1`.

Step 2: Configure the subdevices as ALSA pcm devices
----------------------------------------------------

Before you can use the sound card with PulseAudio, you need to configure the
devices as ALSA pcm devices. ALSA can be configured in the user's home
directory in `~/.adoundrc`. I used the following configuration:

	pcm.!default {
	        type hw
	        card 0
	}

	ctl.!default {
	        type hw
	        card 0
	}
	pcm.Audio2DJ_A {
	        type route
	        slave {
	                pcm "plughw:1,0,0"
	                channels 2
	        }
	        ttable.0.0 1
	        ttable.1.1 1
	}

	pcm.Audio2DJ_B {
	        type route
	        slave {
	                pcm "plughw:1,0,1"
	                channels 2
	        }
	        ttable.0.0 1
	        ttable.1.1 1
	}

The `~/.asoundrc` file is read each time you start an audio application. As
PulseAudio is started when you log in, you need to log out and log in again to
make PulseAudio learn about the new configuration.

Step 3: Configure the subdevices as PulseAudio sinks
----------------------------------------------------

A sink in PulseAudio terminology is an output where you can send audio data to.

PulseAudio ships with the command [pacmd][7] that you can use to reconfigure
the PulseAudio sound server during run time. Pacmd launches a shell. In order
to see the sinks, type `list-sinks` in the pacmd shell.

	Welcome to PulseAudio! Use "help" for usage information.
	>>> list-sinks
	2 sink(s) available.
	...

Without any further configuration, the Audio2DJ sound card will show as a
single sink. On my system, I see one sink for my built-in sound card and one
sink for Audio2DJ. Within the pacmd shell, you can use `load-module` to
configure ALSA devices as additional sinks.

	Welcome to PulseAudio! Use "help" for usage information.
	>>> list-sinks
	2 sink(s) available.
	...
	>>> load-module module-alsa-sink device=Audio2DJ_A
	>>> load-module module-alsa-sink device=Audio2DJ_B
	>>> list-sinks
	4 sink(s) available.
	...

The previously configured sinks are numbered `0` and `1`, the new sinks are `2`
and `3`. Pacmd has a `load-sample` and a `play-sample` command that you can use
to test the new sinks:

	Welcome to PulseAudio! Use "help" for usage information.
	>>> list-sinks
	2 sink(s) available.
	...
	>>> load-module module-alsa-sink device=Audio2DJ_A
	>>> load-module module-alsa-sink device=Audio2DJ_B
	>>> list-sinks
	4 sink(s) available.
	...
	>>> load-sample x /tmp/test.wav
	>>> play-sample x 2
	Playing on sink input #2
	>>> play-sample x 3
	Playing on sink input #3

Voil&agrave;: The first `play-sample` command will play the sound on Audio2DJ
output A, the second one will play it on output B.

Step 4: Make the configuration permanent
----------------------------------------

In order to make the configuration permanent, you can add the `load-module`
commands to your `/etc/pulse/default.pa` file.

[1]: img/Audio2DJ.jpg
[2]: http://www.native-instruments.com
[3]: http://www.pulseaudio.org
[4]: http://alsa.opensrc.org/Aplay
[5]: http://www.alsa-project.org
[6]: http://www.native-instruments.com/knowledge/questions/773/Native+Instruments+hardware+under+Linux
[7]: http://en.wikibooks.org/wiki/Configuring_Sound_on_Linux/Pulse_Audio/Testing
