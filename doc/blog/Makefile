BLOGPOSTS = \
	01-what-is-it-about.html \
	02-example-code-for-playing-mp3s.html \
	03-a-simple-scriptable-audio-player.html \
	04-pulseaudio-and-the-native-instruments-audi2dj-soundcard.html \
	05-architectural-overview.html \
	06-minimal-functionality.html

all: $(BLOGPOSTS)

%.html: %.md
	markdown --html4tags $< > $@

clean:
	rm -r $(BLOGPOSTS)
