music box 6 / music box library - minimal functionality
=======================================================

I didn't work a lot on the music box during summer. However, it's November now,
and gray and cold outside. There are no more excuses.

The virtual controller
----------------------

I restructured the source code of the [music box library][1], and I want to
use this post to explain how the library is structured internally.

Firstly, I decided to keep the software as simple as possible.

The core of the music box library is a virtual controller, as shown
in the figure below.

![music box library - minimal functionality][2]

The virtual controller provides two decks that can load and play MP3 files.
Moreover, there are 16 slots for sound samples, like an [air horn][3].

Deck
----

Each deck can load a single MP3 file at a time. A deck will have the following
functionality:

  * play / resume
  * pause
  * volume control
  * forward / rewind
  * pre-listen on headphones

Output to the speakers
----------------------

Both decks constantly produce an audio output stream that is sent to the
speakers. If a deck is paused, the audio output stream is all zeros, which
means that the deck produces silence.

The audio output of the decks is adjusted with respect to

  * the crossfader
  * the deck's volume control

When the crossfader is on the left, the audio output of deck B is multiplied
by zero, i.e. deck B is silent. When the crossfader is on the right,
the audio output of deck A is multiplied by zero, i.e. deck A is silent.

Moreover, the deck's crossfader is used to adjust the output.

Output to the headphones
------------------------

When headphone output is enabled for a deck, a copy of the deck's audio output
stream is sent to the headphones. If headphone output is enabled on both decks,
both decks will be heard on the headphones.

For the headphone output, only the decks' volume controls are relevant, the
crossfader has no effect on the headphone output.

Master Volume
-------------

The Audio2DJ interface provides hardware volume controls for the speaker's
and the headphone's output, so there will be no additional master volume for
the speakers and the headphones.

Slots for sound samples
-----------------------

The slots for sound samples have very limited functionality. Each slot
can load an MP3 file, and there will be a play button that plays the entire
file from the beginning to the end.

Current state
-------------

The current state of the software is as follows:

  * configure speaker and headphone output device: _ok_
  * load MP3 into deck / sample slot: _ok_
  * play/resume MP3: _ok_
  * play both decks at the same time: _ok_
  * configure latency: _to do_
  * pause: _to do_
  * crossfade: _to do_
  * volume control: _to do_
  * forward / rewind: _to do_
  * pre-listen on headphones: _to do_

[1]: https://github.com/fstab/music-box
[2]: img/music-box-minimal-functionality.png
[3]: http://www.schlachthofbronx.net/home%20+%20airhorn.html
