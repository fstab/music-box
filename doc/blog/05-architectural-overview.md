music box 5 / architectural overview
====================================

In this post, I will present an overview of the target architecture of the
music box software. This post describes the future components of the software,
not its current state on [github][5]. The aim of this post is to give an
overview of the components I am planning to implement myself, the third party
components I will use, and how these components will work together.

![music box architectural overview][1]

As shown in the picture above, the software can be divided into three
layers:

  1. music box front-ends
  2. music box library
  3. audio output

Music Box Front-Ends
--------------------

The front-end layer provides the user interfaces. My goal is to implement at
least two interfaces:

  1. A command shell interface for scripting, testing, and debugging.
  2. A MIDI controller interface that will be built into the hardware version
     of my music box.

I already started some work on the shell-based interface. The sources can
be found on [github][5].

There are a lot of ideas for more front-ends to be implemented in the future,
like a remote control application for smpartphones. However, my primary goal
is to build the hardware box, so my focus is on the MIDI controller.

Music Box Library
-----------------

The music box library will be the core of the music box software. The basic
functionality of the music box library includes:

  * managing MP3 files
  * playing sounds (sending a stream to the audio output)
  * providing a virtual crossfader
  * implementing volume control
  * providing a way to pre-listen songs with the headphones

Moreover, there are ideas for advanced features that might also be implemented
in the music box library. Some of these ideas are:

  * time-stretching
  * pitch-shifting
  * filters (low-pass, ...)
  * audio effects (delay, ...)
  * etc.

My first commits on [github][5] include some of the basic functionality
(load mp3, play mp3), but there is no well-defined interface between the
font-end layer and the music box library yet. Moreover, much of the basic
functionality is still missing.

Audio Output
------------

On the lowest layer, there is the audio output. Currently, I'm using
[PulseAudio][2] to play audio, but it should be possible to replace PulseAudio
with any other audio system, like [JACK][3], or [ALSA][4].

In order to implement a loose coupling between the audio output and the
music box library, I only use very basic functionality of the audio output:

  * Play sound on the headphones
  * Play sound on the speakers

In particular, the music box library will _not_ use any of PulseAudio's mixing
capabilities. Mixing several audio streams is done in the music box library.

The code in my first [github][5] commits is directly implemented against the
PulseAudio API. In the future, I should have a look at the APIs of other audio
systems, and define an abstraction layer for audio output. That way, porting
the music box library to another sound system should become easy.


[1]: img/music-box-overview.png
[2]: http://www.pulseaudio.org
[3]: http://jackaudio.org
[4]: http://www.alsa-project.org
[5]: https://github.com/fstab/music-box
